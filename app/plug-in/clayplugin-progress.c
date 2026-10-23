/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-progress.c
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

#include "libclaybase/claybase.h"

#include "plug-in-types.h"

#include "core/clay.h"
#include "core/claydisplay.h"
#include "core/clayparamspecs.h"
#include "core/claypdbprogress.h"
#include "core/clayprogress.h"

#include "pdb/claypdb.h"
#include "pdb/claypdberror.h"

#include "clayplugin.h"
#include "clayplugin-progress.h"
#include "claypluginmanager.h"
#include "claytemporaryprocedure.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_plug_in_progress_cancel_callback (ClayProgress *progress,
                                                     ClayPlugIn   *plug_in);


/*  public functions  */

gint
clay_plug_in_progress_attach (ClayProgress *progress)
{
  gint attach_count;

  g_return_val_if_fail (CLAY_IS_PROGRESS (progress), 0);

  attach_count =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (progress),
                                        "plug-in-progress-attach-count"));

  attach_count++;

  g_object_set_data (G_OBJECT (progress), "plug-in-progress-attach-count",
                     GINT_TO_POINTER (attach_count));

  return attach_count;
}

gint
clay_plug_in_progress_detach (ClayProgress *progress)
{
  gint attach_count;

  g_return_val_if_fail (CLAY_IS_PROGRESS (progress), 0);

  attach_count =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (progress),
                                        "plug-in-progress-attach-count"));

  attach_count--;

  g_object_set_data (G_OBJECT (progress), "plug-in-progress-attach-count",
                     GINT_TO_POINTER (attach_count));

  return attach_count;
}

void
clay_plug_in_progress_start (ClayPlugIn  *plug_in,
                             const gchar *message,
                             ClayDisplay *display)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (display == NULL || CLAY_IS_DISPLAY (display));

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress)
    {
      proc_frame->progress = clay_new_progress (plug_in->manager->clay,
                                                display);

      if (proc_frame->progress)
        {
          proc_frame->progress_created = TRUE;

          g_object_ref (proc_frame->progress);

          clay_plug_in_progress_attach (proc_frame->progress);
        }
    }

  if (proc_frame->progress)
    {
      if (! proc_frame->progress_cancel_id)
        {
          g_object_add_weak_pointer (G_OBJECT (proc_frame->progress),
                                     (gpointer) &proc_frame->progress);

          proc_frame->progress_cancel_id =
            g_signal_connect (proc_frame->progress, "cancel",
                              G_CALLBACK (clay_plug_in_progress_cancel_callback),
                              plug_in);
        }

      if (clay_progress_is_active (proc_frame->progress))
        {
          if (message)
            clay_progress_set_text_literal (proc_frame->progress, message);

          if (clay_progress_get_value (proc_frame->progress) > 0.0)
            clay_progress_set_value (proc_frame->progress, 0.0);
        }
      else
        {
          clay_progress_start (proc_frame->progress, TRUE,
                               "%s", message ? message : "");
        }
    }
}

void
clay_plug_in_progress_end (ClayPlugIn          *plug_in,
                           ClayPlugInProcFrame *proc_frame)
{
  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));
  g_return_if_fail (proc_frame != NULL);

  if (proc_frame->progress)
    {
      if (proc_frame->progress_cancel_id)
        {
          g_signal_handler_disconnect (proc_frame->progress,
                                       proc_frame->progress_cancel_id);
          proc_frame->progress_cancel_id = 0;

          g_object_remove_weak_pointer (G_OBJECT (proc_frame->progress),
                                        (gpointer) &proc_frame->progress);
        }

      if (clay_plug_in_progress_detach (proc_frame->progress) < 1 &&
          clay_progress_is_active (proc_frame->progress))
        {
          clay_progress_end (proc_frame->progress);
        }

      if (proc_frame->progress_created)
        {
          clay_free_progress (plug_in->manager->clay, proc_frame->progress);
          g_clear_object (&proc_frame->progress);
        }
    }
}

void
clay_plug_in_progress_set_text (ClayPlugIn  *plug_in,
                                const gchar *message)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    clay_progress_set_text_literal (proc_frame->progress, message);
}

void
clay_plug_in_progress_set_value (ClayPlugIn *plug_in,
                                 gdouble     percentage)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! clay_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      clay_plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && clay_progress_is_active (proc_frame->progress))
    clay_progress_set_value (proc_frame->progress, percentage);
}

void
clay_plug_in_progress_pulse (ClayPlugIn *plug_in)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_if_fail (CLAY_IS_PLUG_IN (plug_in));

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! clay_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      clay_plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && clay_progress_is_active (proc_frame->progress))
    clay_progress_pulse (proc_frame->progress);
}

GBytes *
clay_plug_in_progress_get_window_handle (ClayPlugIn *plug_in)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), 0);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    return clay_progress_get_window_handle (proc_frame->progress);
  else if (plug_in->display)
    return clay_get_display_window_handle (plug_in->manager->clay, plug_in->display);

  return 0;
}

gboolean
clay_plug_in_progress_install (ClayPlugIn  *plug_in,
                               const gchar *progress_callback)
{
  ClayPlugInProcFrame *proc_frame;
  ClayProcedure       *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  procedure = clay_pdb_lookup_procedure (plug_in->manager->clay->pdb,
                                         progress_callback);

  if (! CLAY_IS_TEMPORARY_PROCEDURE (procedure)                ||
      CLAY_TEMPORARY_PROCEDURE (procedure)->plug_in != plug_in ||
      procedure->num_args                           != 3       ||
      ! G_IS_PARAM_SPEC_INT    (procedure->args[0])            ||
      ! G_IS_PARAM_SPEC_STRING (procedure->args[1])            ||
      ! G_IS_PARAM_SPEC_DOUBLE (procedure->args[2]))
    {
      return FALSE;
    }

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    {
      clay_plug_in_progress_end (plug_in, proc_frame);

      g_clear_object (&proc_frame->progress);
    }

  proc_frame->progress = g_object_new (CLAY_TYPE_PDB_PROGRESS,
                                       "pdb",           plug_in->manager->clay->pdb,
                                       "context",       proc_frame->main_context,
                                       "callback-name", progress_callback,
                                       NULL);

  clay_plug_in_progress_attach (proc_frame->progress);

  return TRUE;
}

gboolean
clay_plug_in_progress_uninstall (ClayPlugIn  *plug_in,
                                 const gchar *progress_callback)
{
  ClayPlugInProcFrame *proc_frame;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  proc_frame = clay_plug_in_get_proc_frame (plug_in);

  if (CLAY_IS_PDB_PROGRESS (proc_frame->progress))
    {
      clay_plug_in_progress_end (plug_in, proc_frame);

      g_clear_object (&proc_frame->progress);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_plug_in_progress_cancel (ClayPlugIn  *plug_in,
                              const gchar *progress_callback)
{
  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  return FALSE;
}


/*  private functions  */

static ClayValueArray *
get_cancel_return_values (ClayProcedure *procedure)
{
  ClayValueArray *return_vals;
  GError         *error;

  error = g_error_new_literal (CLAY_PDB_ERROR, CLAY_PDB_ERROR_CANCELLED,
                               _("Cancelled"));
  return_vals = clay_procedure_get_return_values (procedure, FALSE, error);
  g_error_free (error);

  return return_vals;
}

static void
clay_plug_in_progress_cancel_callback (ClayProgress *progress,
                                       ClayPlugIn   *plug_in)
{
  ClayPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;
  GList               *list;

  if (proc_frame->main_loop)
    {
      proc_frame->return_vals =
        get_cancel_return_values (proc_frame->procedure);
    }

  for (list = plug_in->temp_proc_frames; list; list = g_list_next (list))
    {
      proc_frame = list->data;

      if (proc_frame->main_loop)
        {
          proc_frame->return_vals =
            get_cancel_return_values (proc_frame->procedure);
        }
    }

  clay_plug_in_close (plug_in, TRUE);
}
