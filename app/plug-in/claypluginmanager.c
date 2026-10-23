/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * claypluginmanager.c
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "plug-in-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/clay-filter-history.h"
#include "core/clay-memsize.h"
#include "core/claymarshal.h"

#include "pdb/claypdb.h"

#include "clayenvirontable.h"
#include "clayinterpreterdb.h"
#include "clayplugin.h"
#include "clayplugindebug.h"
#include "clayplugindef.h"
#include "claypluginmanager.h"
#include "claypluginmanager-data.h"
#include "claypluginmanager-help-domain.h"
#include "claypluginshm.h"
#include "claytemporaryprocedure.h"

#include "clay-intl.h"


enum
{
  PLUG_IN_OPENED,
  PLUG_IN_CLOSED,
  LAST_SIGNAL
};


static void     clay_plug_in_manager_finalize       (GObject    *object);

static gint64   clay_plug_in_manager_get_memsize    (ClayObject *object,
                                                     gint64     *gui_size);


static void     clay_plug_in_manager_close_waitpid  (GPid        pid,
                                                     gint        status,
                                                     ClayPlugIn *plug_in);
static void     clay_plug_in_manager_cleanup_zombie (ClayPlugIn *plug_in,
                                                     gpointer    value,
                                                     gpointer    user_data);


G_DEFINE_TYPE (ClayPlugInManager, clay_plug_in_manager, CLAY_TYPE_OBJECT)

#define parent_class clay_plug_in_manager_parent_class

static guint manager_signals[LAST_SIGNAL] = { 0, };


static void
clay_plug_in_manager_class_init (ClayPlugInManagerClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  manager_signals[PLUG_IN_OPENED] =
    g_signal_new ("plug-in-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayPlugInManagerClass,
                                   plug_in_opened),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PLUG_IN);

  manager_signals[PLUG_IN_CLOSED] =
    g_signal_new ("plug-in-closed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayPlugInManagerClass,
                                   plug_in_closed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_PLUG_IN);

  object_class->finalize         = clay_plug_in_manager_finalize;

  clay_object_class->get_memsize = clay_plug_in_manager_get_memsize;
}

static void
clay_plug_in_manager_init (ClayPlugInManager *manager)
{
  manager->zombie_plug_ins = g_hash_table_new_full (g_direct_hash, NULL,
                                                    g_object_unref, NULL);
}

static void
clay_plug_in_manager_finalize (GObject *object)
{
  ClayPlugInManager *manager = CLAY_PLUG_IN_MANAGER (object);

  g_clear_pointer (&manager->load_procs,     g_slist_free);
  g_clear_pointer (&manager->save_procs,     g_slist_free);
  g_clear_pointer (&manager->export_procs,   g_slist_free);
  g_clear_pointer (&manager->raw_load_procs, g_slist_free);
  g_clear_pointer (&manager->batch_procs,    g_slist_free);

  g_clear_pointer (&manager->display_load_procs,     g_slist_free);
  g_clear_pointer (&manager->display_save_procs,     g_slist_free);
  g_clear_pointer (&manager->display_export_procs,   g_slist_free);
  g_clear_pointer (&manager->display_raw_load_procs, g_slist_free);

  if (manager->plug_in_procedures)
    {
      g_slist_free_full (manager->plug_in_procedures,
                         (GDestroyNotify) g_object_unref);
      manager->plug_in_procedures = NULL;
    }

  if (manager->plug_in_defs)
    {
      g_slist_free_full (manager->plug_in_defs,
                         (GDestroyNotify) g_object_unref);
      manager->plug_in_defs = NULL;
    }

  g_clear_object (&manager->environ_table);
  g_clear_object (&manager->interpreter_db);

  g_clear_pointer (&manager->debug, clay_plug_in_debug_free);

  clay_plug_in_manager_help_domain_exit (manager);
  clay_plug_in_manager_data_free (manager);

  /* Best case scenario is that if we have any plug-in process not
   * returned yet, we'll be able to property close the PID by processing
   * pending events.
   */
  while (g_hash_table_size (manager->zombie_plug_ins) > 0 &&
         g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
  /* If that was not enough, just remove the source. */
  g_hash_table_foreach (manager->zombie_plug_ins,
                        (GHFunc) clay_plug_in_manager_cleanup_zombie,
                        NULL);
  /* Finally free the ClayPlugIn object, otherwise we will leak
   * ClayContext and get annoying and harder-to-diagnose messages on
   * stderr.
   */
  g_hash_table_destroy (manager->zombie_plug_ins);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_plug_in_manager_get_memsize (ClayObject *object,
                                  gint64     *gui_size)
{
  ClayPlugInManager *manager = CLAY_PLUG_IN_MANAGER (object);
  gint64             memsize = 0;

  memsize += clay_g_slist_get_memsize_foreach (manager->plug_in_defs,
                                               (ClayMemsizeFunc)
                                               clay_object_get_memsize,
                                               gui_size);

  memsize += clay_g_slist_get_memsize (manager->plug_in_procedures, 0);
  memsize += clay_g_slist_get_memsize (manager->load_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->save_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->export_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->raw_load_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->batch_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->display_load_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->display_save_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->display_export_procs, 0);
  memsize += clay_g_slist_get_memsize (manager->display_raw_load_procs, 0);

  memsize += clay_g_slist_get_memsize (manager->help_domains,   0 /* FIXME */);

  memsize += clay_g_slist_get_memsize_foreach (manager->open_plug_ins,
                                               (ClayMemsizeFunc)
                                               clay_object_get_memsize,
                                               gui_size);
  memsize += clay_g_slist_get_memsize (manager->plug_in_stack, 0);

  memsize += 0; /* FIXME manager->shm */
  memsize += /* FIXME */ clay_g_object_get_memsize (G_OBJECT (manager->interpreter_db));
  memsize += /* FIXME */ clay_g_object_get_memsize (G_OBJECT (manager->environ_table));
  memsize += 0; /* FIXME manager->plug_in_debug */
  memsize += clay_g_list_get_memsize (manager->data_list, 0 /* FIXME */);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

ClayPlugInManager *
clay_plug_in_manager_new (Clay *clay)
{
  ClayPlugInManager *manager;

  manager = g_object_new (CLAY_TYPE_PLUG_IN_MANAGER, NULL);

  manager->clay           = clay;
  manager->interpreter_db = clay_interpreter_db_new (clay->be_verbose);
  manager->environ_table  = clay_environ_table_new (clay->be_verbose);

  return manager;
}

void
clay_plug_in_manager_initialize (ClayPlugInManager  *manager,
                                 ClayInitStatusFunc  status_callback)
{
  ClayCoreConfig *config;
  const gchar    *path_str;
  GList          *path;

  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (status_callback != NULL);

  config = manager->clay->config;

  status_callback (NULL, _("Plug-in Interpreters"), 0.8);

  path_str = g_getenv ("CLAY_TESTING_INTERPRETER_DIRS");
  if (! path_str)
    path_str = config->interpreter_path;

  path = clay_config_path_expand_to_files (path_str, NULL);
  clay_interpreter_db_load (manager->interpreter_db, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  status_callback (NULL, _("Plug-in Environment"), 0.9);

  path_str = g_getenv ("CLAY_TESTING_ENVIRON_DIRS");
  if (! path_str)
    path_str = config->environ_path;
  path = clay_config_path_expand_to_files (path_str, NULL);
  clay_environ_table_load (manager->environ_table, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /*  allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (manager->clay->use_shm)
    manager->shm = clay_plug_in_shm_new ();

  manager->debug = clay_plug_in_debug_new ();
}

void
clay_plug_in_manager_exit (ClayPlugInManager *manager)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));

  while (manager->open_plug_ins)
    clay_plug_in_close (manager->open_plug_ins->data, TRUE);

  /*  need to detach from shared memory, we can't rely on exit()
   *  cleaning up behind us (see bug #609026)
   */
  if (manager->shm)
    {
      clay_plug_in_shm_free (manager->shm);
      manager->shm = NULL;
    }
}

void
clay_plug_in_manager_add_procedure (ClayPlugInManager   *manager,
                                    ClayPlugInProcedure *procedure)
{
  GSList *list;

  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (procedure));

  for (list = manager->plug_in_procedures; list; list = list->next)
    {
      ClayPlugInProcedure *tmp_proc = list->data;

      if (strcmp (clay_object_get_name (procedure),
                  clay_object_get_name (tmp_proc)) == 0)
        {
          GSList *list2;

          list->data = g_object_ref (procedure);

          g_printerr ("Removing duplicate PDB procedure '%s' "
                      "registered by '%s'\n",
                      clay_object_get_name (tmp_proc),
                      clay_file_get_utf8_name (tmp_proc->file));

          /* search the plugin list to see if any plugins had references to
           * the tmp_proc.
           */
          for (list2 = manager->plug_in_defs; list2; list2 = list2->next)
            {
              ClayPlugInDef *plug_in_def = list2->data;

              if (g_slist_find (plug_in_def->procedures, tmp_proc))
                clay_plug_in_def_remove_procedure (plug_in_def, tmp_proc);
            }

          /* also remove it from the lists of load, save and export procs */
          manager->load_procs             = g_slist_remove (manager->load_procs,             tmp_proc);
          manager->save_procs             = g_slist_remove (manager->save_procs,             tmp_proc);
          manager->export_procs           = g_slist_remove (manager->export_procs,           tmp_proc);
          manager->raw_load_procs         = g_slist_remove (manager->raw_load_procs,         tmp_proc);
          manager->batch_procs            = g_slist_remove (manager->batch_procs,            tmp_proc);
          manager->display_load_procs     = g_slist_remove (manager->display_load_procs,     tmp_proc);
          manager->display_save_procs     = g_slist_remove (manager->display_save_procs,     tmp_proc);
          manager->display_export_procs   = g_slist_remove (manager->display_export_procs,   tmp_proc);
          manager->display_raw_load_procs = g_slist_remove (manager->display_raw_load_procs, tmp_proc);

          /* and from the history */
          clay_filter_history_remove (manager->clay, CLAY_PROCEDURE (tmp_proc));

          g_object_unref (tmp_proc);

          return;
        }
    }

  manager->plug_in_procedures = g_slist_prepend (manager->plug_in_procedures,
                                                 g_object_ref (procedure));
}

void
clay_plug_in_manager_add_batch_procedure (ClayPlugInManager   *manager,
                                          ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  if (! g_slist_find (manager->batch_procs, proc))
    manager->batch_procs = g_slist_prepend (manager->batch_procs, proc);
}

GSList *
clay_plug_in_manager_get_batch_procedures (ClayPlugInManager *manager)
{
  g_return_val_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager), NULL);

  return manager->batch_procs;
}

void
clay_plug_in_manager_add_temp_proc (ClayPlugInManager      *manager,
                                    ClayTemporaryProcedure *procedure)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_TEMPORARY_PROCEDURE (procedure));

  clay_pdb_register_procedure (manager->clay->pdb, CLAY_PROCEDURE (procedure));

  manager->plug_in_procedures = g_slist_prepend (manager->plug_in_procedures,
                                                 g_object_ref (procedure));
}

void
clay_plug_in_manager_remove_temp_proc (ClayPlugInManager      *manager,
                                       ClayTemporaryProcedure *procedure)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_TEMPORARY_PROCEDURE (procedure));

  manager->plug_in_procedures = g_slist_remove (manager->plug_in_procedures,
                                                procedure);

  clay_filter_history_remove (manager->clay,
                              CLAY_PROCEDURE (procedure));

  clay_pdb_unregister_procedure (manager->clay->pdb,
                                 CLAY_PROCEDURE (procedure));

  g_object_unref (procedure);
}

void
clay_plug_in_manager_add_open_plug_in (ClayPlugInManager *manager,
                                       ClayPlugIn        *plug_in)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  manager->open_plug_ins = g_slist_prepend (manager->open_plug_ins,
                                            g_object_ref (plug_in));

  g_signal_emit (manager, manager_signals[PLUG_IN_OPENED], 0,
                 plug_in);
}

void
clay_plug_in_manager_remove_open_plug_in (ClayPlugInManager *manager,
                                          ClayPlugIn        *plug_in)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  manager->open_plug_ins = g_slist_remove (manager->open_plug_ins, plug_in);

  g_signal_emit (manager, manager_signals[PLUG_IN_CLOSED], 0,
                 plug_in);

  g_object_unref (plug_in);
}

void
clay_plug_in_manager_watch_zombie_plug_in (ClayPlugInManager *manager,
                                           ClayPlugIn        *plug_in)
{
  guint source_id = 0;

  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  source_id = g_child_watch_add_full (G_PRIORITY_LOW, plug_in->pid,
                                      (GChildWatchFunc) clay_plug_in_manager_close_waitpid,
                                      plug_in, NULL);
  g_hash_table_insert (manager->zombie_plug_ins,
                       g_object_ref (plug_in),
                       GUINT_TO_POINTER (source_id));
}

void
clay_plug_in_manager_plug_in_push (ClayPlugInManager *manager,
                                   ClayPlugIn        *plug_in)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  manager->current_plug_in = plug_in;

  manager->plug_in_stack = g_slist_prepend (manager->plug_in_stack,
                                            manager->current_plug_in);
}

void
clay_plug_in_manager_plug_in_pop (ClayPlugInManager *manager)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_MANAGER (manager));

  if (manager->current_plug_in)
    manager->plug_in_stack = g_slist_remove (manager->plug_in_stack,
                                             manager->plug_in_stack->data);

  if (manager->plug_in_stack)
    manager->current_plug_in = manager->plug_in_stack->data;
  else
    manager->current_plug_in = NULL;
}


/* Private functions */

static void
clay_plug_in_manager_close_waitpid (GPid        pid,
                                    gint        status,
                                    ClayPlugIn *plug_in)
{
  GError *error = NULL;

  if (plug_in->manager->clay->be_verbose &&
      ! g_spawn_check_wait_status (status, &error))
    {
      g_printerr ("Process for plug-in '%s' terminated with error: %s\n",
                  clay_object_get_name (plug_in),
                  error->message);
    }
  g_clear_error (&error);

  g_spawn_close_pid (pid);
  plug_in->pid = 0;

  g_hash_table_remove (plug_in->manager->zombie_plug_ins, plug_in);
}

static void
clay_plug_in_manager_cleanup_zombie (ClayPlugIn *plug_in,
                                     gpointer    value,
                                     gpointer    user_data)
{
  guint source_id = GPOINTER_TO_UINT (value);

  /* Do not try to wait forever for plug-in process when exiting the
   * application. We don't want them to block the main process forever
   * waiting. Leaving a zombie plug-in process is better.
   * Remove the source watch-PID and leave a stderr message for info.
   */
  g_printerr ("Zombie plug-in: the process of plug-in '%s' never exited.\n",
              clay_object_get_name (CLAY_OBJECT (plug_in)));
  g_source_remove (source_id);

  g_spawn_close_pid (plug_in->pid);
  plug_in->pid = 0;
}
