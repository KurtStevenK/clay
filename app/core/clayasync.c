/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayasync.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clayasync.h"
#include "claycancelable.h"
#include "claywaitable.h"


/* ClayAsync represents an asynchronous task.  Both the public and the
 * protected interfaces are intentionally minimal at this point, to keep things
 * simple.  They may be extended in the future as needed.
 *
 * ClayAsync implements the ClayWaitable and ClayCancelable interfaces.
 *
 * Upon creation, a ClayAsync object is in the "running" state.  Once the task
 * is complete (and before the object's destruction), it should be transitioned
 * to the "stopped" state, using either 'clay_async_finish()' or
 * 'clay_async_abort()'.
 *
 * Similarly, upon creation, a ClayAsync object is said to be "unsynced".  It
 * becomes synced once the execution of any of the completion callbacks added
 * through 'clay_async_add_callback()' had started, or after a successful call
 * to one of the 'clay_waitable_wait()' family of functions.
 *
 * Note that certain ClayAsync functions may only be called during a certain
 * state, on a certain thread, or depending on whether or not the object is
 * synced, as detailed for each function.  When referring to threads, the "main
 * thread" is the thread running the main loop, or any thread whose execution
 * is synchronized with the main thread, and the "async thread" is the thread
 * calling 'clay_async_finish()' or 'clay_async_abort()' (which may also be the
 * main thread), or any thread whose execution is synchronized with the async
 * thread.
 */


/* #define TIME_ASYNC_OPS */


enum
{
  WAITING,
  LAST_SIGNAL
};


typedef struct _ClayAsyncCallbackInfo ClayAsyncCallbackInfo;


struct _ClayAsyncCallbackInfo
{
  ClayAsync         *async;
  ClayAsyncCallback  callback;
  gpointer           data;
  gpointer           gobject;
};

struct _ClayAsyncPrivate
{
  GMutex         mutex;
  GCond          cond;

  GQueue         callbacks;

  gpointer       result;
  GDestroyNotify result_destroy_func;

  guint          idle_id;

  gboolean       stopped;
  gboolean       finished;
  gboolean       synced;
  gboolean       canceled;

#ifdef TIME_ASYNC_OPS
  guint64        start_time;
#endif
};


/*  local function prototypes  */

static void       clay_async_waitable_iface_init   (ClayWaitableInterface   *iface);

static void       clay_async_cancelable_iface_init (ClayCancelableInterface *iface);

static void       clay_async_finalize              (GObject                 *object);

static void       clay_async_wait                  (ClayWaitable            *waitable);
static gboolean   clay_async_try_wait              (ClayWaitable            *waitable);
static gboolean   clay_async_wait_until            (ClayWaitable            *waitable,
                                                    gint64                   end_time);

static void       clay_async_cancel                (ClayCancelable          *cancelable);

static gboolean   clay_async_idle                  (ClayAsync               *async);

static void       clay_async_callback_weak_notify  (ClayAsyncCallbackInfo   *callback_info,
                                                    GObject                 *gobject);

static void       clay_async_stop                  (ClayAsync               *async);
static void       clay_async_run_callbacks         (ClayAsync               *async);


G_DEFINE_TYPE_WITH_CODE (ClayAsync, clay_async, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayAsync)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_WAITABLE,
                                                clay_async_waitable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CANCELABLE,
                                                clay_async_cancelable_iface_init))

#define parent_class clay_async_parent_class

static guint async_signals[LAST_SIGNAL] = { 0 };


/*  local variables  */

static volatile gint clay_async_n_running = 0;


/*  private functions  */


static void
clay_async_class_init (ClayAsyncClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  async_signals[WAITING] =
    g_signal_new ("waiting",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayAsyncClass, waiting),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize = clay_async_finalize;
}

static void
clay_async_waitable_iface_init (ClayWaitableInterface *iface)
{
  iface->wait       = clay_async_wait;
  iface->try_wait   = clay_async_try_wait;
  iface->wait_until = clay_async_wait_until;
}

static void
clay_async_cancelable_iface_init (ClayCancelableInterface *iface)
{
  iface->cancel = clay_async_cancel;
}

static void
clay_async_init (ClayAsync *async)
{
  async->priv = clay_async_get_instance_private (async);

  g_mutex_init (&async->priv->mutex);
  g_cond_init  (&async->priv->cond);

  g_queue_init (&async->priv->callbacks);

  g_atomic_int_inc (&clay_async_n_running);

#ifdef TIME_ASYNC_OPS
  async->priv->start_time = g_get_monotonic_time ();
#endif
}

static void
clay_async_finalize (GObject *object)
{
  ClayAsync *async = CLAY_ASYNC (object);

  g_warn_if_fail (async->priv->stopped);
  g_warn_if_fail (async->priv->idle_id == 0);
  g_warn_if_fail (g_queue_is_empty (&async->priv->callbacks));

  if (async->priv->finished &&
      async->priv->result   &&
      async->priv->result_destroy_func)
    {
      async->priv->result_destroy_func (async->priv->result);

      async->priv->result = NULL;
    }

  g_cond_clear  (&async->priv->cond);
  g_mutex_clear (&async->priv->mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* waits for 'waitable' to transition to the "stopped" state.  if 'waitable' is
 * already stopped, returns immediately.
 *
 * after the call, all callbacks previously added through
 * 'clay_async_add_callback()' are guaranteed to have been called.
 *
 * may only be called on the main thread.
 */
static void
clay_async_wait (ClayWaitable *waitable)
{
  ClayAsync *async = CLAY_ASYNC (waitable);

  g_mutex_lock (&async->priv->mutex);

  if (! async->priv->stopped)
    {
      g_signal_emit (async, async_signals[WAITING], 0);

      while (! async->priv->stopped)
        g_cond_wait (&async->priv->cond, &async->priv->mutex);
    }

  g_mutex_unlock (&async->priv->mutex);

  clay_async_run_callbacks (async);
}

/* same as 'clay_async_wait()', but returns immediately if 'waitable' is not in
 * the "stopped" state.
 *
 * returns TRUE if 'waitable' has transitioned to the "stopped" state, or FALSE
 * otherwise.
 */
static gboolean
clay_async_try_wait (ClayWaitable *waitable)
{
  ClayAsync *async = CLAY_ASYNC (waitable);

  g_mutex_lock (&async->priv->mutex);

  if (! async->priv->stopped)
    {
      g_mutex_unlock (&async->priv->mutex);

      return FALSE;
    }

  g_mutex_unlock (&async->priv->mutex);

  clay_async_run_callbacks (async);

  return TRUE;
}

/* same as 'clay_async_wait()', taking an additional 'end_time' parameter,
 * specifying the maximal monotonic time until which to wait for 'waitable' to
 * stop.
 *
 * returns TRUE if 'waitable' has transitioned to the "stopped" state, or FALSE
 * if the wait was interrupted before the transition.
 */
static gboolean
clay_async_wait_until (ClayWaitable *waitable,
                       gint64        end_time)
{
  ClayAsync *async = CLAY_ASYNC (waitable);

  g_mutex_lock (&async->priv->mutex);

  if (! async->priv->stopped)
    {
      g_signal_emit (async, async_signals[WAITING], 0);

      while (! async->priv->stopped)
        {
          if (! g_cond_wait_until (&async->priv->cond, &async->priv->mutex,
                                   end_time))
            {
              g_mutex_unlock (&async->priv->mutex);

              return FALSE;
            }
        }
    }

  g_mutex_unlock (&async->priv->mutex);

  clay_async_run_callbacks (async);

  return TRUE;
}

/* requests the cancellation of the task managed by 'cancelable'.
 *
 * note that 'clay_async_cancel()' doesn't directly cause 'cancelable' to be
 * stopped, nor synced.  furthermore, 'cancelable' may still complete
 * successfully even when cancellation has been requested.
 *
 * may only be called on the main thread.
 */
static void
clay_async_cancel (ClayCancelable *cancelable)
{
  ClayAsync *async = CLAY_ASYNC (cancelable);

  async->priv->canceled = TRUE;
}

static gboolean
clay_async_idle (ClayAsync *async)
{
  clay_waitable_wait (CLAY_WAITABLE (async));

  return G_SOURCE_REMOVE;
}

static void
clay_async_callback_weak_notify (ClayAsyncCallbackInfo *callback_info,
                                 GObject               *gobject)
{
  ClayAsync *async       = callback_info->async;
  gboolean   unref_async = FALSE;

  g_mutex_lock (&async->priv->mutex);

  g_queue_remove (&async->priv->callbacks, callback_info);

  g_slice_free (ClayAsyncCallbackInfo, callback_info);

  if (g_queue_is_empty (&async->priv->callbacks) && async->priv->idle_id)
    {
      g_source_remove (async->priv->idle_id);
      async->priv->idle_id = 0;

      unref_async = TRUE;
    }

  g_mutex_unlock (&async->priv->mutex);

  if (unref_async)
    g_object_unref (async);
}

static void
clay_async_stop (ClayAsync *async)
{
#ifdef TIME_ASYNC_OPS
  {
    guint64 time = g_get_monotonic_time ();

    g_printerr ("Asynchronous operation took %g seconds%s\n",
                (time - async->priv->start_time) / 1000000.0,
                async->priv->finished ? "" : " (aborted)");
  }
#endif

  g_atomic_int_dec_and_test (&clay_async_n_running);

  if (! g_queue_is_empty (&async->priv->callbacks))
    {
      g_object_ref (async);

      async->priv->idle_id = g_idle_add_full (G_PRIORITY_DEFAULT,
                                              (GSourceFunc) clay_async_idle,
                                              async, NULL);
    }

  async->priv->stopped = TRUE;

  g_cond_broadcast (&async->priv->cond);
}

static void
clay_async_run_callbacks (ClayAsync *async)
{
  ClayAsyncCallbackInfo *callback_info;
  gboolean               unref_async = FALSE;

  if (async->priv->idle_id)
    {
      g_source_remove (async->priv->idle_id);
      async->priv->idle_id = 0;

      unref_async = TRUE;
    }

  async->priv->synced = TRUE;

  while ((callback_info = g_queue_pop_head (&async->priv->callbacks)))
    {
      if (callback_info->gobject)
        {
          g_object_ref (callback_info->gobject);

          g_object_weak_unref (callback_info->gobject,
                               (GWeakNotify) clay_async_callback_weak_notify,
                               callback_info);
        }

      callback_info->callback (async, callback_info->data);

      if (callback_info->gobject)
        g_object_unref (callback_info->gobject);

      g_slice_free (ClayAsyncCallbackInfo, callback_info);
    }

  if (unref_async)
    g_object_unref (async);
}


/*  public functions  */


/* creates a new ClayAsync object, initially unsynced and placed in the
 * "running" state.
 */
ClayAsync *
clay_async_new (void)
{
  return g_object_new (CLAY_TYPE_ASYNC,
                       NULL);
}

/* checks if 'async' is synced.
 *
 * may only be called on the main thread.
 */
gboolean
clay_async_is_synced (ClayAsync *async)
{
  g_return_val_if_fail (CLAY_IS_ASYNC (async), FALSE);

  return async->priv->synced;
}

/* registers a callback to be called when 'async' transitions to the "stopped"
 * state.  if 'async' is already stopped, the callback may be called directly.
 *
 * callbacks are called in the order in which they were added.  'async' is
 * guaranteed to be kept alive, even without an external reference, between the
 * point where it was stopped, and until all callbacks added while 'async' was
 * externally referenced have been called.
 *
 * the callback is guaranteed to be called on the main thread.
 *
 * may only be called on the main thread.
 */
void
clay_async_add_callback (ClayAsync         *async,
                         ClayAsyncCallback  callback,
                         gpointer           data)
{
  ClayAsyncCallbackInfo *callback_info;

  g_return_if_fail (CLAY_IS_ASYNC (async));
  g_return_if_fail (callback != NULL);

  g_mutex_lock (&async->priv->mutex);

  if (async->priv->stopped && g_queue_is_empty (&async->priv->callbacks))
    {
      async->priv->synced = TRUE;

      g_mutex_unlock (&async->priv->mutex);

      callback (async, data);

      return;
    }

  callback_info           = g_slice_new0 (ClayAsyncCallbackInfo);
  callback_info->async    = async;
  callback_info->callback = callback;
  callback_info->data     = data;

  g_queue_push_tail (&async->priv->callbacks, callback_info);

  g_mutex_unlock (&async->priv->mutex);
}

/* same as 'clay_async_add_callback()', however, takes an additional 'gobject'
 * argument, which should be a GObject.
 *
 * 'gobject' is guaranteed to remain alive for the duration of the callback.
 *
 * When 'gobject' is destroyed, the callback is automatically removed.
 */
void
clay_async_add_callback_for_object (ClayAsync         *async,
                                    ClayAsyncCallback  callback,
                                    gpointer           data,
                                    gpointer           gobject)
{
  ClayAsyncCallbackInfo *callback_info;

  g_return_if_fail (CLAY_IS_ASYNC (async));
  g_return_if_fail (callback != NULL);
  g_return_if_fail (G_IS_OBJECT (gobject));

  g_mutex_lock (&async->priv->mutex);

  if (async->priv->stopped && g_queue_is_empty (&async->priv->callbacks))
    {
      async->priv->synced = TRUE;

      g_mutex_unlock (&async->priv->mutex);

      g_object_ref (gobject);

      callback (async, data);

      g_object_unref (gobject);

      return;
    }

  callback_info           = g_slice_new0 (ClayAsyncCallbackInfo);
  callback_info->async    = async;
  callback_info->callback = callback;
  callback_info->data     = data;
  callback_info->gobject  = gobject;

  g_queue_push_tail (&async->priv->callbacks, callback_info);

  g_object_weak_ref (gobject,
                     (GWeakNotify) clay_async_callback_weak_notify,
                     callback_info);

  g_mutex_unlock (&async->priv->mutex);
}

/* removes all callbacks previously registered through
 * 'clay_async_add_callback()', matching 'callback' and 'data', which hasn't
 * been called yet.
 *
 * may only be called on the main thread.
 */
void
clay_async_remove_callback (ClayAsync         *async,
                            ClayAsyncCallback  callback,
                            gpointer           data)
{
  GList    *iter;
  gboolean  unref_async = FALSE;

  g_return_if_fail (CLAY_IS_ASYNC (async));
  g_return_if_fail (callback != NULL);

  g_mutex_lock (&async->priv->mutex);

  iter = g_queue_peek_head_link (&async->priv->callbacks);

  while (iter)
    {
      ClayAsyncCallbackInfo *callback_info = iter->data;
      GList                 *next          = g_list_next (iter);

      if (callback_info->callback == callback &&
          callback_info->data     == data)
        {
          if (callback_info->gobject)
            {
              g_object_weak_unref (
                callback_info->gobject,
                (GWeakNotify) clay_async_callback_weak_notify,
                callback_info);
            }

          g_queue_delete_link (&async->priv->callbacks, iter);

          g_slice_free (ClayAsyncCallbackInfo, callback_info);
        }

      iter = next;
    }

  if (g_queue_is_empty (&async->priv->callbacks) && async->priv->idle_id)
    {
      g_source_remove (async->priv->idle_id);
      async->priv->idle_id = 0;

      unref_async = TRUE;
    }

  g_mutex_unlock (&async->priv->mutex);

  if (unref_async)
    g_object_unref (async);
}

/* checks if 'async' is in the "stopped" state.
 *
 * may only be called on the async thread.
 */
gboolean
clay_async_is_stopped (ClayAsync *async)
{
  g_return_val_if_fail (CLAY_IS_ASYNC (async), FALSE);

  return async->priv->stopped;
}

/* transitions 'async' to the "stopped" state, indicating that the task
 * completed normally, possibly providing a result.
 *
 * 'async' shall be in the "running" state.
 *
 * may only be called on the async thread.
 */
void
clay_async_finish (ClayAsync *async,
                   gpointer   result)
{
  clay_async_finish_full (async, result, NULL);
}

/* same as 'clay_async_finish()', taking an additional GDestroyNotify function,
 * used for freeing the result when 'async' is destroyed.
 */
void
clay_async_finish_full (ClayAsync      *async,
                        gpointer        result,
                        GDestroyNotify  result_destroy_func)
{
  g_return_if_fail (CLAY_IS_ASYNC (async));
  g_return_if_fail (! async->priv->stopped);

  g_mutex_lock (&async->priv->mutex);

  async->priv->finished            = TRUE;
  async->priv->result              = result;
  async->priv->result_destroy_func = result_destroy_func;

  clay_async_stop (async);

  g_mutex_unlock (&async->priv->mutex);
}

/* checks if 'async' completed normally, using 'clay_async_finish()' (in
 * contrast to 'clay_async_abort()').
 *
 * 'async' shall be in the "stopped" state.
 *
 * may only be called on the async thread, or on the main thread when 'async'
 * is synced.
 */
gboolean
clay_async_is_finished (ClayAsync *async)
{
  g_return_val_if_fail (CLAY_IS_ASYNC (async), FALSE);
  g_return_val_if_fail (async->priv->stopped, FALSE);

  return async->priv->finished;
}

/* returns the result of 'async', as passed to 'clay_async_finish()'.
 *
 * 'async' shall be in the "stopped" state, and should have completed normally.
 *
 * may only be called on the async thread, or on the main thread when 'async'
 * is synced.
 */
gpointer
clay_async_get_result (ClayAsync *async)
{
  g_return_val_if_fail (CLAY_IS_ASYNC (async), NULL);
  g_return_val_if_fail (async->priv->stopped, NULL);
  g_return_val_if_fail (async->priv->finished, NULL);

  return async->priv->result;
}

/* transitions 'async' to the "stopped" state, indicating that the task
 * was stopped before completion (normally, in response to a
 * 'clay_cancelable_cancel()' call).
 *
 * 'async' shall be in the "running" state.
 *
 * may only be called on the async thread.
 */
void
clay_async_abort (ClayAsync *async)
{
  g_return_if_fail (CLAY_IS_ASYNC (async));
  g_return_if_fail (! async->priv->stopped);

  g_mutex_lock (&async->priv->mutex);

  clay_async_stop (async);

  g_mutex_unlock (&async->priv->mutex);
}

/* checks if cancellation of 'async' has been requested.
 *
 * note that a return value of TRUE only indicates that
 * 'clay_cancelable_cancel()' has been called for 'async', and not that 'async'
 * is stopped, or, if it is stopped, that it was aborted.
 */
gboolean
clay_async_is_canceled (ClayAsync *async)
{
  g_return_val_if_fail (CLAY_IS_ASYNC (async), FALSE);

  return async->priv->canceled;
}

/* a convenience function, canceling 'async' and waiting for it to stop.
 *
 * may only be called on the main thread.
 */
void
clay_async_cancel_and_wait (ClayAsync *async)
{
  g_return_if_fail (CLAY_IS_ASYNC (async));

  clay_cancelable_cancel (CLAY_CANCELABLE (async));
  clay_waitable_wait (CLAY_WAITABLE (async));
}


/*  public functions (stats)  */


gint
clay_async_get_n_running (void)
{
  return clay_async_n_running;
}
