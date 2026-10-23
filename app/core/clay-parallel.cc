/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-parallel.c
 * Copyright (C) 2018 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

extern "C"
{

#include "core-types.h"

#include "config/claygeglconfig.h"

#include "clay.h"
#include "clay-parallel.h"
#include "clayasync.h"
#include "claycancelable.h"


#define CLAY_PARALLEL_MAX_THREADS           64
#define CLAY_PARALLEL_RUN_ASYNC_MAX_THREADS  1


typedef struct
{
  ClayAsync        *async;
  gint              priority;
  ClayRunAsyncFunc  func;
  gpointer          user_data;
  GDestroyNotify    user_data_destroy_func;
} ClayParallelRunAsyncTask;

typedef struct
{
  GThread   *thread;

  gboolean   quit;

  ClayAsync *current_async;
} ClayParallelRunAsyncThread;


/*  local function prototypes  */

static void                       clay_parallel_notify_num_processors   (ClayGeglConfig             *config);

static void                       clay_parallel_set_n_threads           (gint                        n_threads,
                                                                         gboolean                    finish_tasks);

static void                       clay_parallel_run_async_set_n_threads (gint                        n_threads,
                                                                         gboolean                    finish_tasks);
static gpointer                   clay_parallel_run_async_thread_func   (ClayParallelRunAsyncThread *thread);
static void                       clay_parallel_run_async_enqueue_task  (ClayParallelRunAsyncTask   *task);
static ClayParallelRunAsyncTask * clay_parallel_run_async_dequeue_task  (void);
static gboolean                   clay_parallel_run_async_execute_task  (ClayParallelRunAsyncTask   *task);
static void                       clay_parallel_run_async_abort_task    (ClayParallelRunAsyncTask   *task);
static void                       clay_parallel_run_async_cancel        (ClayAsync                  *async);
static void                       clay_parallel_run_async_waiting       (ClayAsync                  *async);


/*  local variables  */

static gint                       clay_parallel_run_async_n_threads = 0;
static ClayParallelRunAsyncThread clay_parallel_run_async_threads[CLAY_PARALLEL_RUN_ASYNC_MAX_THREADS];

static GMutex                     clay_parallel_run_async_mutex;
static GCond                      clay_parallel_run_async_cond;
static GQueue                     clay_parallel_run_async_queue = G_QUEUE_INIT;


/*  public functions  */


void
clay_parallel_init (Clay *clay)
{
  ClayGeglConfig *config;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  config = CLAY_GEGL_CONFIG (clay->config);

  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (clay_parallel_notify_num_processors),
                    NULL);

  clay_parallel_notify_num_processors (config);
}

void
clay_parallel_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_signal_handlers_disconnect_by_func (clay->config,
                                        (gpointer) clay_parallel_notify_num_processors,
                                        NULL);

  /* stop all threads */
  clay_parallel_set_n_threads (0, /* finish_tasks = */ FALSE);
}

ClayAsync *
clay_parallel_run_async (ClayRunAsyncFunc func,
                         gpointer         user_data)
{
  return clay_parallel_run_async_full (0, func, user_data, NULL);
}

ClayAsync *
clay_parallel_run_async_full (gint             priority,
                              ClayRunAsyncFunc func,
                              gpointer         user_data,
                              GDestroyNotify   user_data_destroy_func)
{
  ClayAsync                *async;
  ClayParallelRunAsyncTask *task;

  g_return_val_if_fail (func != NULL, NULL);

  async = clay_async_new ();

  task = g_slice_new (ClayParallelRunAsyncTask);

  task->async                  = CLAY_ASYNC (g_object_ref (async));
  task->priority               = priority;
  task->func                   = func;
  task->user_data              = user_data;
  task->user_data_destroy_func = user_data_destroy_func;

  if (clay_parallel_run_async_n_threads > 0)
    {
      g_signal_connect_after (async, "cancel",
                              G_CALLBACK (clay_parallel_run_async_cancel),
                              NULL);
      g_signal_connect_after (async, "waiting",
                              G_CALLBACK (clay_parallel_run_async_waiting),
                              NULL);

      g_mutex_lock (&clay_parallel_run_async_mutex);

      clay_parallel_run_async_enqueue_task (task);

      g_cond_signal (&clay_parallel_run_async_cond);

      g_mutex_unlock (&clay_parallel_run_async_mutex);
    }
  else
    {
      while (clay_parallel_run_async_execute_task (task));
    }

  return async;
}

ClayAsync *
clay_parallel_run_async_independent (ClayRunAsyncFunc func,
                                     gpointer         user_data)
{
  return clay_parallel_run_async_independent_full (0, func, user_data);
}

ClayAsync *
clay_parallel_run_async_independent_full (gint             priority,
                                          ClayRunAsyncFunc func,
                                          gpointer         user_data)
{
  ClayAsync                *async;
  ClayParallelRunAsyncTask *task;
  GThread                  *thread;

  g_return_val_if_fail (func != NULL, NULL);

  async = clay_async_new ();

  task = g_slice_new0 (ClayParallelRunAsyncTask);

  task->async     = CLAY_ASYNC (g_object_ref (async));
  task->priority  = priority;
  task->func      = func;
  task->user_data = user_data;

  thread = g_thread_new (
    "async-ind",
    [] (gpointer data) -> gpointer
    {
      ClayParallelRunAsyncTask *task = (ClayParallelRunAsyncTask *) data;

      /* adjust the thread's priority */
#if defined (G_OS_WIN32)
      if (task->priority < 0)
        {
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL);
        }
      else if (task->priority > 0)
        {
          SetThreadPriority (GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN);
        }
#elif defined (HAVE_UNISTD_H) && defined (__gnu_linux__)
      if (task->priority)
        {
          nice (task->priority);
        }
#endif

      while (clay_parallel_run_async_execute_task (task));

      return NULL;
    },
    task);

  clay_async_add_callback (async,
                           [] (ClayAsync *async,
                               gpointer   thread)
                           {
                             g_thread_join ((GThread *) thread);
                           },
                           thread);

  return async;
}


/*  private functions  */


static void
clay_parallel_notify_num_processors (ClayGeglConfig *config)
{
  clay_parallel_set_n_threads (config->num_processors,
                               /* finish_tasks = */ TRUE);
}

static void
clay_parallel_set_n_threads (gint     n_threads,
                             gboolean finish_tasks)
{
  clay_parallel_run_async_set_n_threads (n_threads, finish_tasks);
}

static void
clay_parallel_run_async_set_n_threads (gint     n_threads,
                                       gboolean finish_tasks)
{
  gint i;

  n_threads = CLAMP (n_threads, 0, CLAY_PARALLEL_RUN_ASYNC_MAX_THREADS);

  if (n_threads > clay_parallel_run_async_n_threads) /* need more threads */
    {
      for (i = clay_parallel_run_async_n_threads; i < n_threads; i++)
        {
          ClayParallelRunAsyncThread *thread =
            &clay_parallel_run_async_threads[i];

          thread->quit = FALSE;

          thread->thread = g_thread_new (
            "async",
            (GThreadFunc) clay_parallel_run_async_thread_func,
            thread);
        }
    }
  else if (n_threads < clay_parallel_run_async_n_threads) /* need less threads */
    {
      g_mutex_lock (&clay_parallel_run_async_mutex);

      for (i = n_threads; i < clay_parallel_run_async_n_threads; i++)
        {
          ClayParallelRunAsyncThread *thread =
            &clay_parallel_run_async_threads[i];

          thread->quit = TRUE;

          if (thread->current_async && ! finish_tasks)
            clay_cancelable_cancel (CLAY_CANCELABLE (thread->current_async));
        }

      g_cond_broadcast (&clay_parallel_run_async_cond);

      g_mutex_unlock (&clay_parallel_run_async_mutex);

      for (i = n_threads; i < clay_parallel_run_async_n_threads; i++)
        {
          ClayParallelRunAsyncThread *thread =
            &clay_parallel_run_async_threads[i];

          g_thread_join (thread->thread);
        }
    }

  clay_parallel_run_async_n_threads = n_threads;

  if (n_threads == 0)
    {
      ClayParallelRunAsyncTask *task;

      /* finish remaining tasks */
      while ((task = clay_parallel_run_async_dequeue_task ()))
        {
          if (finish_tasks)
            while (clay_parallel_run_async_execute_task (task));
          else
            clay_parallel_run_async_abort_task (task);
        }
    }
}

static gpointer
clay_parallel_run_async_thread_func (ClayParallelRunAsyncThread *thread)
{
  g_mutex_lock (&clay_parallel_run_async_mutex);

  while (TRUE)
    {
      ClayParallelRunAsyncTask *task;

      while (! thread->quit &&
             (task = clay_parallel_run_async_dequeue_task ()))
        {
          gboolean resume;

          thread->current_async = CLAY_ASYNC (g_object_ref (task->async));

          do
            {
              g_mutex_unlock (&clay_parallel_run_async_mutex);

              resume = clay_parallel_run_async_execute_task (task);

              g_mutex_lock (&clay_parallel_run_async_mutex);
            }
          while (resume &&
                 (g_queue_is_empty (&clay_parallel_run_async_queue) ||
                  task->priority <
                  ((ClayParallelRunAsyncTask *)
                     g_queue_peek_head (
                       &clay_parallel_run_async_queue))->priority));

          g_clear_object (&thread->current_async);

          if (resume)
            clay_parallel_run_async_enqueue_task (task);
        }

      if (thread->quit)
        break;

      g_cond_wait (&clay_parallel_run_async_cond,
                   &clay_parallel_run_async_mutex);
    }

  g_mutex_unlock (&clay_parallel_run_async_mutex);

  return NULL;
}

static void
clay_parallel_run_async_enqueue_task (ClayParallelRunAsyncTask *task)
{
  GList *link;
  GList *iter;

  if (clay_async_is_canceled (task->async))
    {
      clay_parallel_run_async_abort_task (task);

      return;
    }

  link       = g_list_alloc ();
  link->data = task;

  g_object_set_data (G_OBJECT (task->async),
                     "clay-parallel-run-async-link", link);

  for (iter = g_queue_peek_tail_link (&clay_parallel_run_async_queue);
       iter;
       iter = g_list_previous (iter))
    {
      ClayParallelRunAsyncTask *other_task =
        (ClayParallelRunAsyncTask *) iter->data;

      if (other_task->priority <= task->priority)
        break;
    }

  if (iter)
    {
      link->prev = iter;
      link->next = iter->next;

      iter->next = link;

      if (link->next)
        link->next->prev = link;
      else
        clay_parallel_run_async_queue.tail = link;

      clay_parallel_run_async_queue.length++;
    }
  else
    {
      g_queue_push_head_link (&clay_parallel_run_async_queue, link);
    }
}

static ClayParallelRunAsyncTask *
clay_parallel_run_async_dequeue_task (void)
{
  ClayParallelRunAsyncTask *task;

  task = (ClayParallelRunAsyncTask *) g_queue_pop_head (
                                        &clay_parallel_run_async_queue);

  if (task)
    {
      g_object_set_data (G_OBJECT (task->async),
                         "clay-parallel-run-async-link", NULL);
    }

  return task;
}

static gboolean
clay_parallel_run_async_execute_task (ClayParallelRunAsyncTask *task)
{
  if (clay_async_is_canceled (task->async))
    {
      clay_parallel_run_async_abort_task (task);

      return FALSE;
    }

  task->func (task->async, task->user_data);

  if (clay_async_is_stopped (task->async))
    {
      g_object_unref (task->async);

      g_slice_free (ClayParallelRunAsyncTask, task);

      return FALSE;
    }

  return TRUE;
}

static void
clay_parallel_run_async_abort_task (ClayParallelRunAsyncTask *task)
{
  if (task->user_data && task->user_data_destroy_func)
    task->user_data_destroy_func (task->user_data);

  clay_async_abort (task->async);

  g_object_unref (task->async);

  g_slice_free (ClayParallelRunAsyncTask, task);
}

static void
clay_parallel_run_async_cancel (ClayAsync *async)
{
  GList                    *link;
  ClayParallelRunAsyncTask *task = NULL;

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "clay-parallel-run-async-link");

  if (! link)
    return;

  g_mutex_lock (&clay_parallel_run_async_mutex);

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "clay-parallel-run-async-link");

  if (link)
    {
      g_object_set_data (G_OBJECT (async),
                         "clay-parallel-run-async-link", NULL);

      task = (ClayParallelRunAsyncTask *) link->data;

      g_queue_delete_link (&clay_parallel_run_async_queue, link);
    }

  g_mutex_unlock (&clay_parallel_run_async_mutex);

  if (task)
    clay_parallel_run_async_abort_task (task);
}

static void
clay_parallel_run_async_waiting (ClayAsync *async)
{
  GList *link;

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "clay-parallel-run-async-link");

  if (! link)
    return;

  g_mutex_lock (&clay_parallel_run_async_mutex);

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "clay-parallel-run-async-link");

  if (link)
    {
      ClayParallelRunAsyncTask *task = (ClayParallelRunAsyncTask *) link->data;

      task->priority = G_MININT;

      g_queue_unlink         (&clay_parallel_run_async_queue, link);
      g_queue_push_head_link (&clay_parallel_run_async_queue, link);
    }

  g_mutex_unlock (&clay_parallel_run_async_mutex);
}

} /* extern "C" */
