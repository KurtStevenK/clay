/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginprocedure.c
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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"

#include "plug-in-types.h"

#include "gegl/clay-babl-compat.h"

#include "core/clay.h"
#include "core/clay-memsize.h"
#include "core/clay-utils.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayparamspecs.h"

#include "file/file-utils.h"

#define __YES_I_NEED_CLAY_PLUG_IN_MANAGER_CALL__
#include "claypluginmanager-call.h"

#include "claypluginerror.h"
#include "claypluginprocedure.h"
#include "plug-in-menu-path.h"

#include "clay-intl.h"


enum
{
  MENU_PATH_ADDED,
  LAST_SIGNAL
};


static void     clay_plug_in_procedure_finalize        (GObject        *object);

static gint64   clay_plug_in_procedure_get_memsize     (ClayObject     *object,
                                                        gint64         *gui_size);

static gchar  * clay_plug_in_procedure_get_description (ClayViewable   *viewable,
                                                        gchar         **tooltip);

static const gchar * clay_plug_in_procedure_get_label  (ClayProcedure  *procedure);
static const gchar * clay_plug_in_procedure_get_menu_label
                                                       (ClayProcedure  *procedure);
static const gchar * clay_plug_in_procedure_get_blurb  (ClayProcedure  *procedure);
static const gchar * clay_plug_in_procedure_get_help_id(ClayProcedure  *procedure);
static gboolean   clay_plug_in_procedure_get_sensitive (ClayProcedure  *procedure,
                                                        ClayObject     *object,
                                                        const gchar  **reason);
static ClayValueArray * clay_plug_in_procedure_execute (ClayProcedure  *procedure,
                                                        Clay           *clay,
                                                        ClayContext    *context,
                                                        ClayProgress   *progress,
                                                        ClayValueArray *args,
                                                        GError        **error);
static void     clay_plug_in_procedure_execute_async   (ClayProcedure  *procedure,
                                                        Clay           *clay,
                                                        ClayContext    *context,
                                                        ClayProgress   *progress,
                                                        ClayValueArray *args,
                                                        ClayDisplay    *display);

static GFile  * clay_plug_in_procedure_real_get_file   (ClayPlugInProcedure *procedure);

static gboolean clay_plug_in_procedure_validate_args   (ClayPlugInProcedure *proc,
                                                        Clay                *clay,
                                                        ClayValueArray      *args,
                                                        GError             **error);


G_DEFINE_TYPE (ClayPlugInProcedure, clay_plug_in_procedure,
               CLAY_TYPE_PROCEDURE)

#define parent_class clay_plug_in_procedure_parent_class

static guint clay_plug_in_procedure_signals[LAST_SIGNAL] = { 0 };


static void
clay_plug_in_procedure_class_init (ClayPlugInProcedureClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass    *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass  *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayProcedureClass *proc_class        = CLAY_PROCEDURE_CLASS (klass);

  clay_plug_in_procedure_signals[MENU_PATH_ADDED] =
    g_signal_new ("menu-path-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPlugInProcedureClass, menu_path_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  object_class->finalize            = clay_plug_in_procedure_finalize;

  clay_object_class->get_memsize    = clay_plug_in_procedure_get_memsize;

  viewable_class->default_icon_name = "system-run";
  viewable_class->get_description   = clay_plug_in_procedure_get_description;

  proc_class->get_label             = clay_plug_in_procedure_get_label;
  proc_class->get_menu_label        = clay_plug_in_procedure_get_menu_label;
  proc_class->get_blurb             = clay_plug_in_procedure_get_blurb;
  proc_class->get_help_id           = clay_plug_in_procedure_get_help_id;
  proc_class->get_sensitive         = clay_plug_in_procedure_get_sensitive;
  proc_class->execute               = clay_plug_in_procedure_execute;
  proc_class->execute_async         = clay_plug_in_procedure_execute_async;

  klass->get_file                   = clay_plug_in_procedure_real_get_file;
  klass->menu_path_added            = NULL;
}

static void
clay_plug_in_procedure_init (ClayPlugInProcedure *proc)
{
  CLAY_PROCEDURE (proc)->proc_type = CLAY_PDB_PROC_TYPE_PLUGIN;

  proc->icon_data_length = -1;
  proc->sensitivity_mask = CLAY_PROCEDURE_SENSITIVE_DRAWABLE | CLAY_PROCEDURE_SENSITIVE_DRAWABLES;
}

static void
clay_plug_in_procedure_finalize (GObject *object)
{
  ClayPlugInProcedure *proc = CLAY_PLUG_IN_PROCEDURE (object);

  g_object_unref (proc->file);
  g_free (proc->menu_label);

  g_list_free_full (proc->menu_paths, (GDestroyNotify) g_free);

  g_free (proc->help_id_with_domain);

  g_free (proc->icon_data);
  g_free (proc->image_types);
  g_free (proc->insensitive_reason);

  g_free (proc->extensions);
  g_free (proc->meta_extensions);
  g_free (proc->prefixes);
  g_free (proc->magics);
  g_free (proc->mime_types);

  g_slist_free_full (proc->extensions_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->meta_extensions_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->prefixes_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->magics_list, (GDestroyNotify) g_free);
  g_slist_free_full (proc->mime_types_list, (GDestroyNotify) g_free);

  g_free (proc->thumb_loader);
  g_free (proc->batch_interpreter_name);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_plug_in_procedure_get_memsize (ClayObject *object,
                                    gint64     *gui_size)
{
  ClayPlugInProcedure *proc    = CLAY_PLUG_IN_PROCEDURE (object);
  gint64               memsize = 0;
  GList               *list;
  GSList              *slist;

  memsize += clay_g_object_get_memsize (G_OBJECT (proc->file));
  memsize += clay_string_get_memsize (proc->menu_label);

  for (list = proc->menu_paths; list; list = g_list_next (list))
    memsize += sizeof (GList) + clay_string_get_memsize (list->data);

  switch (proc->icon_type)
    {
    case CLAY_ICON_TYPE_ICON_NAME:
    case CLAY_ICON_TYPE_IMAGE_FILE:
      memsize += clay_string_get_memsize ((const gchar *) proc->icon_data);
      break;

    case CLAY_ICON_TYPE_PIXBUF:
      memsize += proc->icon_data_length;
      break;
    }

  memsize += clay_string_get_memsize (proc->extensions);
  memsize += clay_string_get_memsize (proc->prefixes);
  memsize += clay_string_get_memsize (proc->magics);
  memsize += clay_string_get_memsize (proc->mime_types);
  memsize += clay_string_get_memsize (proc->thumb_loader);
  memsize += clay_string_get_memsize (proc->batch_interpreter_name);

  for (slist = proc->extensions_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + clay_string_get_memsize (slist->data);

  for (slist = proc->prefixes_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + clay_string_get_memsize (slist->data);

  for (slist = proc->magics_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + clay_string_get_memsize (slist->data);

  for (slist = proc->mime_types_list; slist; slist = g_slist_next (slist))
    memsize += sizeof (GSList) + clay_string_get_memsize (slist->data);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
clay_plug_in_procedure_get_description (ClayViewable  *viewable,
                                        gchar        **tooltip)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (viewable);

  if (tooltip)
    *tooltip = g_strdup (clay_procedure_get_blurb (procedure));

  return g_strdup (clay_procedure_get_label (procedure));
}

static const gchar *
clay_plug_in_procedure_get_label (ClayProcedure *procedure)
{
  ClayPlugInProcedure *proc = CLAY_PLUG_IN_PROCEDURE (procedure);

  if (! proc->menu_label)
    return NULL;

  return CLAY_PROCEDURE_CLASS (parent_class)->get_label (procedure);
}

static const gchar *
clay_plug_in_procedure_get_menu_label (ClayProcedure *procedure)
{
  ClayPlugInProcedure *proc = CLAY_PLUG_IN_PROCEDURE (procedure);

  if (proc->menu_label)
    return proc->menu_label;

  return CLAY_PROCEDURE_CLASS (parent_class)->get_menu_label (procedure);
}

static const gchar *
clay_plug_in_procedure_get_blurb (ClayProcedure *procedure)
{
  if (procedure->blurb)
    return procedure->blurb;

  return NULL;
}

static const gchar *
clay_plug_in_procedure_get_help_id (ClayProcedure *procedure)
{
  ClayPlugInProcedure *proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  const gchar         *domain;
  const gchar         *help_id;

  if (proc->help_id_with_domain)
    return proc->help_id_with_domain;

  domain = clay_plug_in_procedure_get_help_domain (proc);

  if (procedure->help_id)
    help_id = procedure->help_id;
  else
    help_id = clay_object_get_name (procedure);

  if (domain)
    proc->help_id_with_domain = g_strconcat (domain, "?", help_id, NULL);
  else
    proc->help_id_with_domain = g_strdup (help_id);

  return proc->help_id_with_domain;
}

static gboolean
clay_plug_in_procedure_get_sensitive (ClayProcedure  *procedure,
                                      ClayObject     *object,
                                      const gchar   **reason)
{
  ClayPlugInProcedure *proc       = CLAY_PLUG_IN_PROCEDURE (procedure);
  ClayImage           *image;
  GList               *drawables  = NULL;
  ClayImageType        image_type = -1;
  gboolean             sensitive  = FALSE;

  g_return_val_if_fail (object == NULL || CLAY_IS_IMAGE (object), FALSE);

  image = CLAY_IMAGE (object);
  if (image)
    drawables = clay_image_get_selected_drawables (image);

  if (drawables)
    {
      const Babl *format = clay_drawable_get_format (drawables->data);

      image_type = clay_babl_format_get_image_type (format);
    }

  if (proc->image_types_val)
    {
      switch (image_type)
        {
        case CLAY_RGB_IMAGE:
          sensitive = proc->image_types_val & CLAY_PLUG_IN_RGB_IMAGE;
          break;
        case CLAY_RGBA_IMAGE:
          sensitive = proc->image_types_val & CLAY_PLUG_IN_RGBA_IMAGE;
          break;
        case CLAY_GRAY_IMAGE:
          sensitive = proc->image_types_val & CLAY_PLUG_IN_GRAY_IMAGE;
          break;
        case CLAY_GRAYA_IMAGE:
          sensitive = proc->image_types_val & CLAY_PLUG_IN_GRAYA_IMAGE;
          break;
        case CLAY_INDEXED_IMAGE:
          sensitive = proc->image_types_val & CLAY_PLUG_IN_INDEXED_IMAGE;
          break;
        case CLAY_INDEXEDA_IMAGE:
          sensitive = proc->image_types_val & CLAY_PLUG_IN_INDEXEDA_IMAGE;
          break;
        default:
          sensitive = FALSE;
          break;
        }
    }
  else
    {
      sensitive = (image_type != -1);
    }

  if (! image &&
      (proc->sensitivity_mask & CLAY_PROCEDURE_SENSITIVE_NO_IMAGE) != 0)
    sensitive = TRUE;
  else if (g_list_length (drawables) == 1 &&
           (proc->sensitivity_mask & CLAY_PROCEDURE_SENSITIVE_DRAWABLE) == 0)
    sensitive = FALSE;
  else if (image && g_list_length (drawables) == 0)
    sensitive = proc->sensitivity_mask & CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES;
  else if (g_list_length (drawables) > 1 &&
           (proc->sensitivity_mask & CLAY_PROCEDURE_SENSITIVE_DRAWABLES) == 0)
    sensitive = FALSE;

  g_list_free (drawables);

  if (! sensitive)
    *reason = proc->insensitive_reason;

  return sensitive ? TRUE : FALSE;
}

static ClayValueArray *
clay_plug_in_procedure_execute (ClayProcedure  *procedure,
                                Clay           *clay,
                                ClayContext    *context,
                                ClayProgress   *progress,
                                ClayValueArray *args,
                                GError        **error)
{
  ClayPlugInProcedure *plug_in_procedure = CLAY_PLUG_IN_PROCEDURE (procedure);
  GError              *pdb_error         = NULL;

  if (! clay_plug_in_procedure_validate_args (plug_in_procedure, clay,
                                              args, &pdb_error))
    {
      ClayValueArray *return_vals;

      return_vals = clay_procedure_get_return_values (procedure, FALSE,
                                                      pdb_error);
      g_propagate_error (error, pdb_error);

      return return_vals;
    }

  if (procedure->proc_type == CLAY_PDB_PROC_TYPE_INTERNAL)
    return CLAY_PROCEDURE_CLASS (parent_class)->execute (procedure, clay,
                                                         context, progress,
                                                         args, error);

  return clay_plug_in_manager_call_run (clay->plug_in_manager,
                                        context, progress,
                                        CLAY_PLUG_IN_PROCEDURE (procedure),
                                        args, TRUE, NULL);
}

static void
clay_plug_in_procedure_execute_async (ClayProcedure  *procedure,
                                      Clay           *clay,
                                      ClayContext    *context,
                                      ClayProgress   *progress,
                                      ClayValueArray *args,
                                      ClayDisplay    *display)
{
  ClayPlugInProcedure *plug_in_procedure = CLAY_PLUG_IN_PROCEDURE (procedure);
  GError              *error             = NULL;

  if (clay_plug_in_procedure_validate_args (plug_in_procedure, clay,
                                            args, &error))
    {
      ClayValueArray *return_vals;

      return_vals = clay_plug_in_manager_call_run (clay->plug_in_manager,
                                                   context, progress,
                                                   plug_in_procedure,
                                                   args, FALSE, display);

      if (return_vals)
        {
          clay_plug_in_procedure_handle_return_values (plug_in_procedure,
                                                       clay, progress,
                                                       return_vals);
          clay_value_array_unref (return_vals);
        }
    }
  else
    {
      clay_message_literal (clay, G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}

static GFile *
clay_plug_in_procedure_real_get_file (ClayPlugInProcedure *procedure)
{
  return procedure->file;
}

static inline gboolean
CLAY_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == CLAY_TYPE_RUN_MODE);
}

static gboolean
clay_plug_in_procedure_validate_args (ClayPlugInProcedure *proc,
                                      Clay                *clay,
                                      ClayValueArray      *args,
                                      GError             **error)
{
#if 0
  ClayProcedure *procedure = CLAY_PROCEDURE (proc);
  GValue        *uri_value = NULL;

  if (! proc->file_proc)
    return TRUE;

  /*  make sure that the passed strings are actually URIs, not just a
   *  file path (bug 758685)
   */

  if ((procedure->num_args   >= 3)                     &&
      (procedure->num_values >= 1)                     &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) &&
      CLAY_IS_PARAM_SPEC_FILE     (procedure->args[1]) &&
      CLAY_IS_PARAM_SPEC_IMAGE    (procedure->values[0]))
    {
      uri_value = clay_value_array_index (args, 1);
    }
  else if ((procedure->num_args >= 5)                       &&
           CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) &&
           CLAY_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) &&
           CLAY_IS_PARAM_SPEC_DRAWABLE (procedure->args[2]) &&
           CLAY_IS_PARAM_SPEC_FILE     (procedure->args[3]))
    {
      uri_value = clay_value_array_index (args, 3);
    }

  if (uri_value)
    {
      GFile *file;

      file = file_utils_filename_to_file (clay,
                                          g_value_get_string (uri_value),
                                          error);

      if (! file)
        return FALSE;

      g_value_take_string (uri_value, g_file_get_uri (file));
      g_object_unref (file);
    }
#endif

  return TRUE;
}


/*  public functions  */

ClayProcedure *
clay_plug_in_procedure_new (ClayPDBProcType  proc_type,
                            GFile           *file,
                            GFile           *root_folder)
{
  ClayPlugInProcedure *proc;

  g_return_val_if_fail ((proc_type == CLAY_PDB_PROC_TYPE_INTERNAL &&
                         root_folder == NULL) ||
                        ((proc_type == CLAY_PDB_PROC_TYPE_PLUGIN ||
                          proc_type == CLAY_PDB_PROC_TYPE_PERSISTENT) &&
                         G_IS_FILE (root_folder)), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  proc = g_object_new (CLAY_TYPE_PLUG_IN_PROCEDURE, NULL);

  proc->file        = g_object_ref (file);
  proc->root_folder = root_folder ? g_object_ref (root_folder) : NULL;

  CLAY_PROCEDURE (proc)->proc_type = proc_type;

  return CLAY_PROCEDURE (proc);
}

ClayPlugInProcedure *
clay_plug_in_procedure_find (GSList      *list,
                             const gchar *proc_name)
{
  GSList *l;

  for (l = list; l; l = g_slist_next (l))
    {
      ClayObject *object = l->data;

      if (! strcmp (proc_name, clay_object_get_name (object)))
        return CLAY_PLUG_IN_PROCEDURE (object);
    }

  return NULL;
}

GFile *
clay_plug_in_procedure_get_file (ClayPlugInProcedure *proc)
{
  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return CLAY_PLUG_IN_PROCEDURE_GET_CLASS (proc)->get_file (proc);
}

GFile *
clay_plug_in_procedure_get_root_folder (ClayPlugInProcedure *proc)
{
  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return proc->root_folder;
}

void
clay_plug_in_procedure_set_help_domain (ClayPlugInProcedure *proc,
                                        const gchar         *help_domain)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  proc->help_domain = help_domain ? g_quark_from_string (help_domain) : 0;
}

const gchar *
clay_plug_in_procedure_get_help_domain (ClayPlugInProcedure *proc)
{
  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), NULL);

  return g_quark_to_string (proc->help_domain);
}

gboolean
clay_plug_in_procedure_set_help_uri (ClayPlugInProcedure  *proc,
                                     const gchar          *help_uri,
                                     GError              **error)
{
  gchar *scheme;
  gchar *basename;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), FALSE);

  if (help_uri == NULL || strlen (help_uri) == 0)
    {
      g_clear_pointer (&proc->help_uri, g_free);
      g_clear_object (&proc->help_file);
      return TRUE;
    }

  scheme = g_uri_parse_scheme (help_uri);
  if (scheme != NULL)
    {
      gchar *tmp;

      tmp    = g_ascii_strdown (scheme, -1);
      g_free (scheme);
      scheme = tmp;
    }

  basename = g_path_get_basename (clay_file_get_utf8_name (proc->file));
  if (g_strcmp0 (scheme, "https") != 0 && scheme != NULL)
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n\n"
                   "attempted to install procedure \"%s\" with a "
                   "documentation URI (%s) which is neither HTTPS "
                   "nor a relative path.",
                   basename, clay_file_get_utf8_name (proc->file),
                   clay_object_get_name (proc), help_uri);
      g_free (basename);
      g_free (scheme);
      return FALSE;
    }

  if (scheme == NULL)
    {
      GFile *local_file;

      if (g_path_is_absolute (help_uri))
        {
          g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                       "Plug-in \"%s\"\n(%s)\n\n"
                       "attempted to install procedure \"%s\" with a "
                       "absolute path as documentation URI (%s).",
                       basename, clay_file_get_utf8_name (proc->file),
                       clay_object_get_name (proc), help_uri);
          g_free (basename);
          return FALSE;
        }

      local_file  = g_file_resolve_relative_path (proc->root_folder, help_uri);
      /* Don't get tricked by some relative path leaving the plug-in
       * folder, e.g. "../else/where/".
       */
      if (! clay_file_is_ancestor (proc->root_folder, local_file))
        {
          g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                       "Plug-in \"%s\"\n(%s)\n\n"
                       "attempted to install procedure \"%s\" with a "
                       "documentation path outside its folder (%s).",
                       basename, clay_file_get_utf8_name (proc->file),
                       clay_object_get_name (proc), help_uri);
          g_object_unref (local_file);
          g_free (basename);
          return FALSE;
        }

      if (! g_file_query_exists (local_file, NULL))
        {
          g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                       "Plug-in \"%s\"\n(%s)\n\n"
                       "attempted to install procedure \"%s\" with a "
                       "documentation path which does not exist (%s).",
                       basename, clay_file_get_utf8_name (proc->file),
                       clay_object_get_name (proc), help_uri);
          g_object_unref (local_file);
          g_free (basename);
          return FALSE;
        }

      g_clear_object (&proc->help_file);
      proc->help_file = local_file;
    }
  else
    {
      g_clear_object (&proc->help_file);
      proc->help_file = g_file_new_for_uri (help_uri);
      g_free (scheme);
    }

  g_free (proc->help_uri);
  proc->help_uri = g_strdup (help_uri);

  g_free (basename);

  return TRUE;
}

gboolean
clay_plug_in_procedure_set_menu_label (ClayPlugInProcedure  *proc,
                                       const gchar          *menu_label,
                                       GError              **error)
{
  gboolean valid_utf8;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (menu_label != NULL && strlen (menu_label), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  valid_utf8 = g_utf8_validate (menu_label, -1, NULL);

  if (! valid_utf8 || menu_label[0] == '<')
    {
      gchar *basename = g_path_get_basename (clay_file_get_utf8_name (proc->file));

      if (! valid_utf8)
        g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                     "Plug-in \"%s\"\n(%s)\n\n"
                     "attempted to install procedure \"%s\" with a "
                     "menu label not encoded as UTF-8.",
                     basename, clay_file_get_utf8_name (proc->file),
                     clay_object_get_name (proc));
      else
        g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                     "Plug-in \"%s\"\n(%s)\n\n"
                     "attempted to install procedure \"%s\" with a full "
                     "menu path \"%s\" as menu label, this is not supported "
                     "any longer.",
                     basename, clay_file_get_utf8_name (proc->file),
                     clay_object_get_name (proc),
                     menu_label);

      g_free (basename);

      return FALSE;
    }

  g_clear_pointer (&CLAY_PROCEDURE (proc)->label, g_free);

  g_free (proc->menu_label);
  proc->menu_label = g_strdup (menu_label);

  return TRUE;
}

gboolean
clay_plug_in_procedure_add_menu_path (ClayPlugInProcedure  *proc,
                                      const gchar          *menu_path,
                                      GError              **error)
{
  ClayProcedure *procedure;
  gchar         *basename = NULL;
  const gchar   *required = NULL;
  const gchar   *p;
  gchar         *mapped_path;
  gboolean       valid_utf8;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  procedure  = CLAY_PROCEDURE (proc);
  valid_utf8 = g_utf8_validate (menu_path, -1, NULL);

  if (! valid_utf8 || ! proc->menu_label)
    {
      basename = g_path_get_basename (clay_file_get_utf8_name (proc->file));

      if (! valid_utf8)
        g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                     "Plug-in \"%s\"\n(%s)\n\n"
                     "attempted to install procedure \"%s\" with a "
                     "menu path not encoded as UTF-8.",
                     basename, clay_file_get_utf8_name (proc->file),
                     clay_object_get_name (proc));
      else
        g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                     "Plug-in \"%s\"\n(%s)\n"
                     "attempted to register the procedure \"%s\" "
                     "in the menu \"%s\", but the procedure has no label. "
                     "This is not allowed.",
                     basename, clay_file_get_utf8_name (proc->file),
                     clay_object_get_name (proc),
                     menu_path);

      g_free (basename);

      return FALSE;
    }

  p = strchr (menu_path, '>');
  if (p == NULL || (*(++p) && *p != '/'))
    {
      basename = g_path_get_basename (clay_file_get_utf8_name (proc->file));

      g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\"\n"
                   "in the invalid menu location \"%s\".\n"
                   "The menu path must look like either \"<Prefix>\" "
                   "or \"<Prefix>/path/to/item\".",
                   basename, clay_file_get_utf8_name (proc->file),
                   clay_object_get_name (proc),
                   menu_path);
      goto failure;
    }

  if (g_str_has_prefix (menu_path, "<Image>"))
    {
      if ((procedure->num_args < 1) ||
          ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
        {
          required = "ClayRunMode";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Layers>"))
    {
      if ((procedure->num_args < 3)                          ||
          ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! CLAY_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) ||
          ! (G_TYPE_FROM_INSTANCE       (procedure->args[2]) == CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY))
        {
          required = "ClayRunMode, ClayImage, NULL-terminated array of (ClayLayer | ClayDrawable)";
          goto failure;
        }
      else
        {
          const gchar *type_name = g_type_name (clay_param_spec_core_object_array_get_object_type (procedure->args[2]));

          if (g_strcmp0 (type_name, "ClayDrawable") != 0 &&
              g_strcmp0 (type_name, "ClayLayer")    != 0)
            {
              required = "ClayRunMode, ClayImage, NULL-terminated array of (ClayLayer | ClayDrawable)";
              goto failure;
            }
        }
    }
  else if (g_str_has_prefix (menu_path, "<Channels>"))
    {
      if ((procedure->num_args < 3)                          ||
          ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! CLAY_IS_PARAM_SPEC_IMAGE    (procedure->args[1]) ||
          ! (G_TYPE_FROM_INSTANCE       (procedure->args[2]) == CLAY_TYPE_PARAM_CORE_OBJECT_ARRAY))
        {
          required = "ClayRunMode, ClayImage, NULL-terminated array of (ClayChannel | ClayDrawable)";
          goto failure;
        }
      else
        {
          const gchar *type_name = g_type_name (clay_param_spec_core_object_array_get_object_type (procedure->args[2]));

          if (g_strcmp0 (type_name, "ClayDrawable") != 0 &&
              g_strcmp0 (type_name, "ClayChannel")  != 0)
            {
              required = "ClayRunMode, ClayImage, NULL-terminated array of (ClayChannel | ClayDrawable)";
              goto failure;
            }
        }
    }
  else if (g_str_has_prefix (menu_path, "<Colormap>") ||
           g_str_has_prefix (menu_path, "<Paths>"))
    {
      if ((procedure->num_args < 2)                          ||
          ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]) ||
          ! CLAY_IS_PARAM_SPEC_IMAGE    (procedure->args[1]))
        {
          required = "ClayRunMode, ClayImage";
          goto failure;
        }
    }
  else if (g_str_has_prefix (menu_path, "<Brushes>")        ||
           g_str_has_prefix (menu_path, "<Dynamics>")       ||
           g_str_has_prefix (menu_path, "<MyPaintBrushes>") ||
           g_str_has_prefix (menu_path, "<Gradients>")      ||
           g_str_has_prefix (menu_path, "<Palettes>")       ||
           g_str_has_prefix (menu_path, "<Patterns>")       ||
           g_str_has_prefix (menu_path, "<ToolPresets>")    ||
           g_str_has_prefix (menu_path, "<Fonts>")          ||
           g_str_has_prefix (menu_path, "<Buffers>"))
    {
      if ((procedure->num_args < 1) ||
          ! CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
        {
          required = "ClayRunMode";
          goto failure;
        }
    }
  else
    {
      basename = g_path_get_basename (clay_file_get_utf8_name (proc->file));

      g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\" "
                   "in the invalid menu location \"%s\".\n"
                   "Use either \"<Image>\", "
                   "\"<Layers>\", \"<Channels>\", \"<Paths>\", "
                   "\"<Colormap>\", \"<Brushes>\", \"<Dynamics>\", "
                   "\"<MyPaintBrushes>\", \"<Gradients>\", \"<Palettes>\", "
                   "\"<Patterns>\", \"<ToolPresets>\", \"<Fonts>\" "
                   "or \"<Buffers>\".",
                   basename, clay_file_get_utf8_name (proc->file),
                   clay_object_get_name (proc),
                   menu_path);
      goto failure;
    }

  g_free (basename);

  mapped_path = plug_in_menu_path_map (menu_path, NULL);

  proc->menu_paths = g_list_append (proc->menu_paths, mapped_path);

  g_signal_emit (proc, clay_plug_in_procedure_signals[MENU_PATH_ADDED], 0,
                 mapped_path);

  return TRUE;

 failure:
  if (required)
    {
      gchar *prefix = g_strdup (menu_path);
      gchar *c;

      c = strchr (prefix, '>') + 1;
      *c = '\0';

      basename = g_path_get_basename (clay_file_get_utf8_name (proc->file));

      g_set_error (error, CLAY_PLUG_IN_ERROR, CLAY_PLUG_IN_FAILED,
                   "Plug-in \"%s\"\n(%s)\n\n"
                   "attempted to install %s procedure \"%s\" "
                   "which does not take the standard %s plug-in's "
                   "arguments: (%s).",
                   basename, clay_file_get_utf8_name (proc->file),
                   prefix, clay_object_get_name (proc), prefix,
                   required);

      g_free (prefix);
    }

  g_free (basename);

  return FALSE;
}

gboolean
clay_plug_in_procedure_set_icon (ClayPlugInProcedure  *proc,
                                 ClayIconType          icon_type,
                                 const guint8         *icon_data,
                                 gint                  icon_data_length,
                                 GError              **error)
{
  return clay_plug_in_procedure_take_icon (proc, icon_type,
                                           g_memdup2 (icon_data, icon_data_length),
                                           icon_data_length, error);
}

gboolean
clay_plug_in_procedure_take_icon (ClayPlugInProcedure  *proc,
                                  ClayIconType          icon_type,
                                  guint8               *icon_data,
                                  gint                  icon_data_length,
                                  GError              **error)
{
  const gchar *icon_name   = NULL;
  GdkPixbuf   *icon_pixbuf = NULL;
  gboolean     success     = TRUE;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (proc->icon_data)
    {
      g_free (proc->icon_data);
      proc->icon_data_length = -1;
      proc->icon_data        = NULL;
    }

  proc->icon_type = icon_type;

  switch (proc->icon_type)
    {
      GdkPixbufLoader *loader;
      gchar           *path;

    case CLAY_ICON_TYPE_ICON_NAME:
      proc->icon_data_length = -1;
      proc->icon_data        = icon_data;

      icon_name = (const gchar *) proc->icon_data;
      break;

    case CLAY_ICON_TYPE_PIXBUF:
      proc->icon_data_length = icon_data_length;
      proc->icon_data        = icon_data;

      loader = gdk_pixbuf_loader_new ();

      if (! gdk_pixbuf_loader_write (loader,
                                     proc->icon_data,
                                     proc->icon_data_length,
                                     error))
        {
          gdk_pixbuf_loader_close (loader, NULL);
          success = FALSE;
        }
      else if (! gdk_pixbuf_loader_close (loader, error))
        {
          success = FALSE;
        }

      if (success)
        {
          icon_pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

          if (icon_pixbuf)
            g_object_ref (icon_pixbuf);
        }

      g_object_unref (loader);
      break;

    case CLAY_ICON_TYPE_IMAGE_FILE:
      proc->icon_data_length = -1;
      proc->icon_data        = icon_data;

      path = g_filename_from_uri ((const gchar *) proc->icon_data, NULL, error);
      if (path != NULL)
        {
          icon_pixbuf = gdk_pixbuf_new_from_file (path, error);
          g_free (path);
        }

      if (! icon_pixbuf)
        success = FALSE;
      break;
    }

  clay_viewable_set_icon_name (CLAY_VIEWABLE (proc), icon_name);
  g_object_set (proc, "icon-pixbuf", icon_pixbuf, NULL);

  if (icon_pixbuf)
    g_object_unref (icon_pixbuf);

  return success;
}

static ClayPlugInImageType
image_types_parse (const gchar *name,
                   const gchar *image_types)
{
  const gchar         *type_spec = image_types;
  ClayPlugInImageType  types     = 0;

  /*  If the plug_in registers with image_type == NULL or "", return 0
   *  By doing so it won't be touched by plug_in_set_menu_sensitivity()
   */
  if (! image_types)
    return types;

  while (*image_types)
    {
      while (*image_types &&
             ((*image_types == ' ') ||
              (*image_types == '\t') ||
              (*image_types == ',')))
        image_types++;

      if (*image_types)
        {
          if (g_str_has_prefix (image_types, "RGBA"))
            {
              types |= CLAY_PLUG_IN_RGBA_IMAGE;
              image_types += strlen ("RGBA");
            }
          else if (g_str_has_prefix (image_types, "RGB*"))
            {
              types |= CLAY_PLUG_IN_RGB_IMAGE | CLAY_PLUG_IN_RGBA_IMAGE;
              image_types += strlen ("RGB*");
            }
          else if (g_str_has_prefix (image_types, "RGB"))
            {
              types |= CLAY_PLUG_IN_RGB_IMAGE;
              image_types += strlen ("RGB");
            }
          else if (g_str_has_prefix (image_types, "GRAYA"))
            {
              types |= CLAY_PLUG_IN_GRAYA_IMAGE;
              image_types += strlen ("GRAYA");
            }
          else if (g_str_has_prefix (image_types, "GRAY*"))
            {
              types |= CLAY_PLUG_IN_GRAY_IMAGE | CLAY_PLUG_IN_GRAYA_IMAGE;
              image_types += strlen ("GRAY*");
            }
          else if (g_str_has_prefix (image_types, "GRAY"))
            {
              types |= CLAY_PLUG_IN_GRAY_IMAGE;
              image_types += strlen ("GRAY");
            }
          else if (g_str_has_prefix (image_types, "INDEXEDA"))
            {
              types |= CLAY_PLUG_IN_INDEXEDA_IMAGE;
              image_types += strlen ("INDEXEDA");
            }
          else if (g_str_has_prefix (image_types, "INDEXED*"))
            {
              types |= CLAY_PLUG_IN_INDEXED_IMAGE | CLAY_PLUG_IN_INDEXEDA_IMAGE;
              image_types += strlen ("INDEXED*");
            }
          else if (g_str_has_prefix (image_types, "INDEXED"))
            {
              types |= CLAY_PLUG_IN_INDEXED_IMAGE;
              image_types += strlen ("INDEXED");
            }
          else if (g_str_has_prefix (image_types, "*"))
            {
              types |= (CLAY_PLUG_IN_RGB_IMAGE     | CLAY_PLUG_IN_RGBA_IMAGE  |
                        CLAY_PLUG_IN_GRAY_IMAGE    | CLAY_PLUG_IN_GRAYA_IMAGE |
                        CLAY_PLUG_IN_INDEXED_IMAGE | CLAY_PLUG_IN_INDEXEDA_IMAGE);
              image_types += strlen ("*");
            }
          else
            {
              g_printerr ("%s: image-type contains unrecognizable parts:"
                          "'%s'\n", name, type_spec);

              /* skip to next token */
              while (*image_types &&
                     *image_types != ' '  &&
                     *image_types != '\t' &&
                     *image_types != ',')
                {
                  image_types++;
                }
            }
        }
    }

  return types;
}

void
clay_plug_in_procedure_set_image_types (ClayPlugInProcedure *proc,
                                        const gchar         *image_types)
{
  GList *types = NULL;

  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->image_types)
    g_free (proc->image_types);

  proc->image_types     = g_strdup (image_types);
  proc->image_types_val = image_types_parse (clay_object_get_name (proc),
                                             proc->image_types);

  g_clear_pointer (&proc->insensitive_reason, g_free);

  if (proc->image_types_val &
      (CLAY_PLUG_IN_RGB_IMAGE | CLAY_PLUG_IN_RGBA_IMAGE))
    {
      if ((proc->image_types_val & CLAY_PLUG_IN_RGB_IMAGE) &&
          (proc->image_types_val & CLAY_PLUG_IN_RGBA_IMAGE))
        {
          types = g_list_prepend (types, _("RGB"));
        }
      else if (proc->image_types_val & CLAY_PLUG_IN_RGB_IMAGE)
        {
          types = g_list_prepend (types, _("RGB without alpha"));
        }
      else
        {
          types = g_list_prepend (types, _("RGB with alpha"));
        }
    }

  if (proc->image_types_val &
      (CLAY_PLUG_IN_GRAY_IMAGE | CLAY_PLUG_IN_GRAYA_IMAGE))
    {
      if ((proc->image_types_val & CLAY_PLUG_IN_GRAY_IMAGE) &&
          (proc->image_types_val & CLAY_PLUG_IN_GRAYA_IMAGE))
        {
          types = g_list_prepend (types, _("Grayscale"));
        }
      else if (proc->image_types_val & CLAY_PLUG_IN_GRAY_IMAGE)
        {
          types = g_list_prepend (types, _("Grayscale without alpha"));
        }
      else
        {
          types = g_list_prepend (types, _("Grayscale with alpha"));
        }
    }

  if (proc->image_types_val &
      (CLAY_PLUG_IN_INDEXED_IMAGE | CLAY_PLUG_IN_INDEXEDA_IMAGE))
    {
      if ((proc->image_types_val & CLAY_PLUG_IN_INDEXED_IMAGE) &&
          (proc->image_types_val & CLAY_PLUG_IN_INDEXEDA_IMAGE))
        {
          types = g_list_prepend (types, _("Indexed"));
        }
      else if (proc->image_types_val & CLAY_PLUG_IN_INDEXED_IMAGE)
        {
          types = g_list_prepend (types, _("Indexed without alpha"));
        }
      else
        {
          types = g_list_prepend (types, _("Indexed with alpha"));
        }
    }

  if (types)
    {
      GString *string;
      GList   *list;

      types = g_list_reverse (types);

      string = g_string_new (_("This plug-in only works on the "
                               "following layer types:"));
      g_string_append (string, " ");

      for (list = types; list; list = g_list_next (list))
        {
          g_string_append (string, list->data);

          if (list->next)
            g_string_append (string, ", ");
          else
            g_string_append (string, ".");
        }

      g_list_free (types);

      proc->insensitive_reason = g_string_free (string, FALSE);
    }
}

void
clay_plug_in_procedure_set_sensitivity_mask (ClayPlugInProcedure *proc,
                                             gint                 sensitivity_mask)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  if (sensitivity_mask == 0)
    proc->sensitivity_mask = CLAY_PROCEDURE_SENSITIVE_DRAWABLE | CLAY_PROCEDURE_SENSITIVE_DRAWABLES;
  else
    proc->sensitivity_mask = sensitivity_mask;
}

static GSList *
extensions_parse (gchar *extensions)
{
  GSList *list = NULL;

  /*  extensions can be NULL.  Avoid calling strtok if it is.  */
  if (extensions)
    {
      gchar *extension;
      gchar *next_token;
#ifdef _UCRT
      gchar *context = NULL;
#endif

      /*  work on a copy  */
      extensions = g_strdup (extensions);

      next_token = extensions;
#ifndef _UCRT
      extension = strtok (next_token, " \t,");
#else
      extension = strtok_s (next_token, " \t,", &context);
#endif

      while (extension)
        {
          list = g_slist_prepend (list, g_strdup (extension));
#ifndef _UCRT
          extension = strtok (NULL, " \t,");
#else
          extension = strtok_s (NULL, " \t,", &context);
#endif
        }

      g_free (extensions);
    }

  return g_slist_reverse (list);
}

void
clay_plug_in_procedure_set_file_proc (ClayPlugInProcedure *proc,
                                      const gchar         *extensions,
                                      const gchar         *meta_extensions,
                                      const gchar         *prefixes,
                                      const gchar         *magics)
{
  GSList *list;

  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  proc->file_proc = TRUE;

  /*  extensions  */

  if (proc->extensions != extensions)
    {
      g_free (proc->extensions);

      proc->extensions = g_strdup (extensions);
    }

  if (proc->extensions_list)
    g_slist_free_full (proc->extensions_list, (GDestroyNotify) g_free);

  proc->extensions_list = extensions_parse (proc->extensions);

  if (proc->meta_extensions != meta_extensions)
    {
      g_free (proc->meta_extensions);

      proc->meta_extensions = g_strdup (meta_extensions);
    }

  if (proc->meta_extensions_list)
    g_slist_free_full (proc->meta_extensions_list, (GDestroyNotify) g_free);

  proc->meta_extensions_list = extensions_parse (proc->meta_extensions);

  /*  prefixes  */

  if (proc->prefixes != prefixes)
    {
      if (proc->prefixes)
        g_free (proc->prefixes);

      proc->prefixes = g_strdup (prefixes);
    }

  if (proc->prefixes_list)
    g_slist_free_full (proc->prefixes_list, (GDestroyNotify) g_free);

  proc->prefixes_list = extensions_parse (proc->prefixes);

  /* don't allow "file:" to be registered as prefix */
  for (list = proc->prefixes_list; list; list = g_slist_next (list))
    {
      const gchar *prefix = list->data;

      if (prefix && strcmp (prefix, "file:") == 0)
        {
          g_free (list->data);
          proc->prefixes_list = g_slist_delete_link (proc->prefixes_list, list);
          break;
        }
    }

  /*  magics  */

  if (proc->magics != magics)
    {
      if (proc->magics)
        g_free (proc->magics);

      proc->magics = g_strdup (magics);
    }

  if (proc->magics_list)
    g_slist_free_full (proc->magics_list, (GDestroyNotify) g_free);

  proc->magics_list = extensions_parse (proc->magics);
}

gchar *
clay_plug_in_procedure_get_save_extensions (ClayPlugInProcedure *proc,
                                            gboolean             one_full)
{
  GString *str = g_string_new ("");

  for (GSList *iter = proc->extensions_list; iter; iter = iter->next)
    if (g_str_has_prefix (iter->data, "xcf"))
      {
        g_string_append (str, iter->data);

        if (one_full)
          break;

        if (iter->next && ! one_full)
          g_string_append (str, ",");
      }

  return g_string_free (str, (str->len == 0));
}

gchar *
clay_plug_in_procedure_get_export_extensions (ClayPlugInProcedure *proc,
                                              gboolean             one_full)
{
  GString *str = g_string_new ("");

  for (GSList *iter = proc->extensions_list; iter; iter = iter->next)
    if (! g_str_has_prefix (iter->data, "xcf"))
      {
        g_string_append (str, iter->data);

        if (one_full)
          break;

        if (iter->next)
          g_string_append (str, ",");
      }

  if ((str->len == 0 || ! one_full) && proc->meta_extensions_list)
    {
      if (str->len > 0)
        g_string_append (str, ",");

      for (GSList *iter = proc->meta_extensions_list; iter; iter = iter->next)
        {
          /* The one_full case is used to actually set an extension. If
           * we don't have a good common export format using this meta
           * format, then let's use PNG.
           * The other case is for displaying full listing of supported
           * export extensions. We use a glob '*' for display.
           */
          if (one_full)
            g_string_append_printf (str, "png.%s", (gchar *) iter->data);
          else
            g_string_append_printf (str, "*.%s", (gchar *) iter->data);

          if (one_full)
            break;

          if (iter->next)
            g_string_append (str, ",");
        }
    }

  return g_string_free (str, (str->len == 0));
}

gchar *
clay_plug_in_procedure_get_open_extensions (ClayPlugInProcedure *proc)
{
  GString *str = g_string_new ("");

  for (GSList *iter = proc->extensions_list; iter; iter = iter->next)
    {
      g_string_append (str, iter->data);

      if (iter->next)
        g_string_append (str, ",");
    }

  if (proc->meta_extensions_list)
    {
      if (str->len > 0)
        g_string_append (str, ",");

      for (GSList *iter = proc->meta_extensions_list; iter; iter = iter->next)
        {
          g_string_append_printf (str, "*.%s", (gchar *) iter->data);

          if (iter->next)
            g_string_append (str, ",");
        }
    }

  return g_string_free (str, (str->len == 0));
}

void
clay_plug_in_procedure_set_generic_file_proc (ClayPlugInProcedure *proc,
                                              gboolean             is_generic_file_proc)
{
  proc->generic_file_proc = is_generic_file_proc;
}

void
clay_plug_in_procedure_set_priority (ClayPlugInProcedure *proc,
                                     gint                 priority)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  proc->priority = priority;
}

void
clay_plug_in_procedure_set_mime_types (ClayPlugInProcedure *proc,
                                       const gchar         *mime_types)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->mime_types != mime_types)
    {
      if (proc->mime_types)
        g_free (proc->mime_types);

      proc->mime_types = g_strdup (mime_types);
    }

  if (proc->mime_types_list)
    g_slist_free_full (proc->mime_types_list, (GDestroyNotify) g_free);

  proc->mime_types_list = extensions_parse (proc->mime_types);
}

void
clay_plug_in_procedure_set_handles_remote (ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  proc->handles_remote = TRUE;
}

void
clay_plug_in_procedure_set_handles_raw (ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  proc->handles_raw = TRUE;
}

void
clay_plug_in_procedure_set_handles_vector (ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  proc->handles_vector = TRUE;
}

void
clay_plug_in_procedure_set_thumb_loader (ClayPlugInProcedure *proc,
                                         const gchar         *thumb_loader)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  if (proc->thumb_loader)
    g_free (proc->thumb_loader);

  proc->thumb_loader = g_strdup (thumb_loader);
}

void
clay_plug_in_procedure_set_batch_interpreter (ClayPlugInProcedure *proc,
                                              const gchar         *name)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (name != NULL);

  if (proc->batch_interpreter_name)
    g_free (proc->batch_interpreter_name);

  proc->batch_interpreter      = TRUE;
  proc->batch_interpreter_name = g_strdup (name);
}

void
clay_plug_in_procedure_handle_return_values (ClayPlugInProcedure *proc,
                                             Clay                *clay,
                                             ClayProgress        *progress,
                                             ClayValueArray      *return_vals)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (return_vals != NULL);

  if (clay_value_array_length (return_vals) == 0 ||
      G_VALUE_TYPE (clay_value_array_index (return_vals, 0)) !=
      CLAY_TYPE_PDB_STATUS_TYPE)
    {
      return;
    }

  switch (g_value_get_enum (clay_value_array_index (return_vals, 0)))
    {
    case CLAY_PDB_SUCCESS:
      break;

    case CLAY_PDB_CALLING_ERROR:
      if (clay_value_array_length (return_vals) > 1 &&
          G_VALUE_HOLDS_STRING (clay_value_array_index (return_vals, 1)))
        {
          clay_message (clay, G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                        _("Calling error for '%s':\n"
                          "%s"),
                        clay_procedure_get_label (CLAY_PROCEDURE (proc)),
                        g_value_get_string (clay_value_array_index (return_vals, 1)));
        }
      break;

    case CLAY_PDB_EXECUTION_ERROR:
      if (clay_value_array_length (return_vals) > 1 &&
          G_VALUE_HOLDS_STRING (clay_value_array_index (return_vals, 1)))
        {
          clay_message (clay, G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                        _("Execution error for '%s':\n"
                          "%s"),
                        clay_procedure_get_label (CLAY_PROCEDURE (proc)),
                        g_value_get_string (clay_value_array_index (return_vals, 1)));
        }
      break;
    }
}

gboolean
clay_plug_in_procedure_is_xcf_load (ClayPlugInProcedure *file_proc)
{
  const gchar *proc_name;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (file_proc), TRUE);

  proc_name = clay_object_get_name (file_proc);

  /* Even when loading through an intermediate container format plug-in
   * (e.g. file-compressor), the stored procedure shall be the inner
   * format.
   * See commit bb9d8df855b.
   */
  return (g_strcmp0 (proc_name, "clay-xcf-load") == 0);
}
