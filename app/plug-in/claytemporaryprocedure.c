/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytemporaryprocedure.c
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

#include "clayplugin.h"
#define __YES_I_NEED_CLAY_PLUG_IN_MANAGER_CALL__
#include "claypluginmanager-call.h"
#include "claytemporaryprocedure.h"

#include "clay-intl.h"


static void             clay_temporary_procedure_finalize (GObject        *object);

static ClayValueArray * clay_temporary_procedure_execute  (ClayProcedure  *procedure,
                                                           Clay           *clay,
                                                           ClayContext    *context,
                                                           ClayProgress   *progress,
                                                           ClayValueArray *args,
                                                           GError        **error);
static void        clay_temporary_procedure_execute_async (ClayProcedure  *procedure,
                                                           Clay           *clay,
                                                           ClayContext    *context,
                                                           ClayProgress   *progress,
                                                           ClayValueArray *args,
                                                           ClayDisplay    *display);

static GFile     * clay_temporary_procedure_get_file      (ClayPlugInProcedure *procedure);


G_DEFINE_TYPE (ClayTemporaryProcedure, clay_temporary_procedure,
               CLAY_TYPE_PLUG_IN_PROCEDURE)

#define parent_class clay_temporary_procedure_parent_class


static void
clay_temporary_procedure_class_init (ClayTemporaryProcedureClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  ClayProcedureClass       *proc_class   = CLAY_PROCEDURE_CLASS (klass);
  ClayPlugInProcedureClass *plug_class   = CLAY_PLUG_IN_PROCEDURE_CLASS (klass);

  object_class->finalize    = clay_temporary_procedure_finalize;

  proc_class->execute       = clay_temporary_procedure_execute;
  proc_class->execute_async = clay_temporary_procedure_execute_async;

  plug_class->get_file      = clay_temporary_procedure_get_file;
}

static void
clay_temporary_procedure_init (ClayTemporaryProcedure *proc)
{
  CLAY_PROCEDURE (proc)->proc_type = CLAY_PDB_PROC_TYPE_TEMPORARY;
}

static void
clay_temporary_procedure_finalize (GObject *object)
{
  /* ClayTemporaryProcedure *proc = CLAY_TEMPORARY_PROCEDURE (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static ClayValueArray *
clay_temporary_procedure_execute (ClayProcedure   *procedure,
                                  Clay            *clay,
                                  ClayContext     *context,
                                  ClayProgress    *progress,
                                  ClayValueArray  *args,
                                  GError         **error)
{
  return clay_plug_in_manager_call_run_temp (clay->plug_in_manager,
                                             context, progress,
                                             CLAY_TEMPORARY_PROCEDURE (procedure),
                                             args);
}

static void
clay_temporary_procedure_execute_async (ClayProcedure  *procedure,
                                        Clay           *clay,
                                        ClayContext    *context,
                                        ClayProgress   *progress,
                                        ClayValueArray *args,
                                        ClayDisplay    *display)
{
  ClayTemporaryProcedure *temp_procedure = CLAY_TEMPORARY_PROCEDURE (procedure);
  ClayValueArray         *return_vals;

  return_vals = clay_plug_in_manager_call_run_temp (clay->plug_in_manager,
                                                    context, progress,
                                                    temp_procedure,
                                                    args);

  if (return_vals)
    {
      ClayPlugInProcedure *proc = CLAY_PLUG_IN_PROCEDURE (procedure);

      clay_plug_in_procedure_handle_return_values (proc,
                                                   clay, progress,
                                                   return_vals);
      clay_value_array_unref (return_vals);
    }
}

static GFile *
clay_temporary_procedure_get_file (ClayPlugInProcedure *procedure)
{
  return CLAY_TEMPORARY_PROCEDURE (procedure)->plug_in->file;
}


/*  public functions  */

ClayProcedure *
clay_temporary_procedure_new (ClayPlugIn *plug_in)
{
  ClayTemporaryProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), NULL);

  proc = g_object_new (CLAY_TYPE_TEMPORARY_PROCEDURE, NULL);

  proc->plug_in = plug_in;

  CLAY_PLUG_IN_PROCEDURE (proc)->file = g_file_new_for_path ("none");

  return CLAY_PROCEDURE (proc);
}
