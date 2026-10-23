/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-proc.c
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

#include "core/clayparamspecs.h"

#include "pdb/claypdberror.h"

#include "clayplugin.h"
#include "clayplugin-proc.h"
#include "clayplugindef.h"
#include "claypluginmanager.h"
#include "claypluginmanager-file.h"
#include "claypluginprocedure.h"

#include "clay-intl.h"


/*  local function prototypes  */

static ClayPlugInProcedure * clay_plug_in_proc_find (ClayPlugIn  *plug_in,
                                                     const gchar *proc_name);


/*  public functions  */

gboolean
clay_plug_in_set_proc_image_types (ClayPlugIn   *plug_in,
                                   const gchar  *proc_name,
                                   const gchar  *image_types,
                                   GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register images types "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_image_types (proc, image_types);

  return TRUE;
}

gboolean
clay_plug_in_set_proc_sensitivity_mask (ClayPlugIn   *plug_in,
                                        const gchar  *proc_name,
                                        gint          sensitivity_mask,
                                        GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the sensitivity mask \"%x\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   sensitivity_mask, proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_sensitivity_mask (proc, sensitivity_mask);

  return TRUE;
}

gboolean
clay_plug_in_set_proc_menu_label (ClayPlugIn   *plug_in,
                                  const gchar  *proc_name,
                                  const gchar  *menu_label,
                                  GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_label != NULL && strlen (menu_label), FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the menu label \"%s\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   menu_label, proc_name);

      return FALSE;
    }

  return clay_plug_in_procedure_set_menu_label (proc, menu_label, error);
}

gboolean
clay_plug_in_add_proc_menu_path (ClayPlugIn   *plug_in,
                                 const gchar  *proc_name,
                                 const gchar  *menu_path,
                                 GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the menu item \"%s\" "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   menu_path, proc_name);

      return FALSE;
    }

  return clay_plug_in_procedure_add_menu_path (proc, menu_path, error);
}

gboolean
clay_plug_in_set_proc_icon (ClayPlugIn    *plug_in,
                            const gchar   *proc_name,
                            ClayIconType   type,
                            const guint8  *data,
                            gint           data_length,
                            GError       **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to set the icon "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  return clay_plug_in_procedure_set_icon (proc, type, data, data_length,
                                          error);
}

gboolean
clay_plug_in_set_proc_help (ClayPlugIn   *plug_in,
                            const gchar  *proc_name,
                            const gchar  *blurb,
                            const gchar  *help,
                            const gchar  *help_id,
                            GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register help "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_procedure_set_help (CLAY_PROCEDURE (proc),
                           blurb, help, help_id);

  return TRUE;
}

gboolean
clay_plug_in_set_proc_attribution (ClayPlugIn   *plug_in,
                                   const gchar  *proc_name,
                                   const gchar  *authors,
                                   const gchar  *copyright,
                                   const gchar  *date,
                                   GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the attribution "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_procedure_set_attribution (CLAY_PROCEDURE (proc),
                                  authors, copyright, date);

  return TRUE;
}

static inline gboolean
CLAY_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == CLAY_TYPE_RUN_MODE);
}

static inline gboolean
clay_plug_in_is_file_spec (GParamSpec *pspec)
{
  /* This will work both for ClayParamSpecFile specs and
   * GParamSpecObject with a GFile value.
   */
  return (G_IS_PARAM_SPEC_OBJECT (pspec) &&
          pspec->value_type == G_TYPE_FILE);
}

gboolean
clay_plug_in_set_file_proc_load_handler (ClayPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *extensions,
                                         const gchar  *meta_extensions,
                                         const gchar  *prefixes,
                                         const gchar  *magics,
                                         GError      **error)
{
  ClayPlugInProcedure *proc;
  ClayProcedure       *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as load handler.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = CLAY_PROCEDURE (proc);

  if (((procedure->num_args   < 2)                        ||
       (procedure->num_values < 1)                        ||
       ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
       ! clay_plug_in_is_file_spec   (procedure->args[1]) ||
       (! proc->generic_file_proc &&
        ! CLAY_IS_PARAM_SPEC_IMAGE (procedure->values[0]))))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as load handler which does not take the standard "
                   "load procedure arguments: "
                   "(ClayRunMode, GFile) -> (ClayImage)",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_file_proc (proc, extensions, meta_extensions, prefixes, magics);

  clay_plug_in_manager_add_load_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_save_handler (ClayPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *extensions,
                                         const gchar  *meta_extensions,
                                         const gchar  *prefixes,
                                         GError      **error)
{
  ClayPlugInProcedure *proc;
  ClayProcedure       *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as save handler.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = CLAY_PROCEDURE (proc);

  if ((procedure->num_args < 4)                                ||
      ! CLAY_IS_PARAM_SPEC_RUN_MODE       (procedure->args[0]) ||
      ! CLAY_IS_PARAM_SPEC_IMAGE          (procedure->args[1]) ||
      ! clay_plug_in_is_file_spec         (procedure->args[2]) ||
      ! CLAY_IS_PARAM_SPEC_EXPORT_OPTIONS (procedure->args[3]))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as save handler which does not take the standard "
                   "save procedure arguments:\n"
                   "(ClayRunMode, ClayImage, GFile, ClayExportOptions)",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_file_proc (proc, extensions, meta_extensions, prefixes, NULL);

  clay_plug_in_manager_add_save_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_priority (ClayPlugIn   *plug_in,
                                     const gchar  *proc_name,
                                     gint          priority,
                                     GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the priority "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_priority (proc, priority);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_mime_types (ClayPlugIn   *plug_in,
                                       const gchar  *proc_name,
                                       const gchar  *mime_types,
                                       GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register mime types "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_mime_types (proc, mime_types);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_handles_remote (ClayPlugIn   *plug_in,
                                           const gchar  *proc_name,
                                           GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles remote' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_handles_remote (proc);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_handles_raw (ClayPlugIn   *plug_in,
                                        const gchar  *proc_name,
                                        GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles raw' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (proc->handles_vector)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles raw' "
                   "for procedure \"%s\" which already 'handles vector'.\n"
                   "It cannot handle both.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }
  clay_plug_in_procedure_set_handles_raw (proc);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_handles_vector (ClayPlugIn   *plug_in,
                                           const gchar  *proc_name,
                                           GError      **error)
{
  ClayPlugInProcedure *proc;
  ClayProcedure       *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles vector' "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = CLAY_PROCEDURE (proc);

  if (procedure->num_args < 4                            ||
      procedure->num_values < 1                          ||
      ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
      ! clay_plug_in_is_file_spec   (procedure->args[1]) ||
      ! G_IS_PARAM_SPEC_INT         (procedure->args[2]) ||
      ! G_IS_PARAM_SPEC_INT         (procedure->args[3]) ||
      ! CLAY_IS_PARAM_SPEC_IMAGE (procedure->values[0]))
    {

      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a vector load procedure which does not take the "
                   "standard load procedure procedure arguments: "
                   "(ClayRunMode, file, int, int) -> (image)",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (proc->handles_raw)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register 'handles vector' "
                   "for procedure \"%s\" which already 'handles raw'.\n"
                   "It cannot handle both.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_handles_vector (proc);

  return TRUE;
}

gboolean
clay_plug_in_set_file_proc_thumb_loader (ClayPlugIn   *plug_in,
                                         const gchar  *proc_name,
                                         const gchar  *thumb_proc_name,
                                         GError      **error)
{
  ClayPlugInProcedure *proc;
  ClayPlugInProcedure *thumb_proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (thumb_proc_name != NULL, FALSE);

  proc       = clay_plug_in_proc_find (plug_in, proc_name);
  thumb_proc = clay_plug_in_proc_find (plug_in, thumb_proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register a thumbnail loader "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  if (! thumb_proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register a procedure \"%s\" "
                   "as thumbnail loader.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   thumb_proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_thumb_loader (proc, thumb_proc_name);

  return TRUE;
}

gboolean
clay_plug_in_set_batch_interpreter (ClayPlugIn   *plug_in,
                                    const gchar  *proc_name,
                                    const gchar  *interpreter_name,
                                    GError      **error)
{
  ClayPlugInProcedure *proc;
  ClayProcedure       *procedure;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a 'batch interpreter'.\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  procedure = CLAY_PROCEDURE (proc);

  if (procedure->num_args < 2                            ||
      ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
      ! G_IS_PARAM_SPEC_STRING   (procedure->args[1]))
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register procedure \"%s\" "
                   "as a batch interpreter which does not take the standard "
                   "batch interpreter procedure arguments: "
                   "(ClayRunMode, gchar *) -> ()",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  clay_plug_in_procedure_set_batch_interpreter (proc, interpreter_name);
  clay_plug_in_manager_add_batch_procedure (plug_in->manager, proc);

  return TRUE;
}

gboolean
clay_plug_in_set_proc_help_uri (ClayPlugIn   *plug_in,
                                const gchar  *proc_name,
                                const gchar  *uri,
                                GError      **error)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail (CLAY_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);

  proc = clay_plug_in_proc_find (plug_in, proc_name);

  if (! proc)
    {
      g_set_error (error, CLAY_PDB_ERROR, CLAY_PDB_ERROR_PROCEDURE_NOT_FOUND,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to register the help URI "
                   "for procedure \"%s\".\n"
                   "It has however not installed that procedure. "
                   "This is not allowed.",
                   clay_object_get_name (plug_in),
                   clay_file_get_utf8_name (plug_in->file),
                   proc_name);

      return FALSE;
    }

  return clay_plug_in_procedure_set_help_uri (proc, uri, error);
}


/*  private functions  */

static ClayPlugInProcedure *
clay_plug_in_proc_find (ClayPlugIn  *plug_in,
                        const gchar *proc_name)
{
  ClayPlugInProcedure *proc = NULL;

  if (plug_in->plug_in_def)
    proc = clay_plug_in_procedure_find (plug_in->plug_in_def->procedures,
                                        proc_name);

  if (! proc)
    proc = clay_plug_in_procedure_find (plug_in->temp_procedures, proc_name);

  return proc;
}
