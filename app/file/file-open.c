/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-open.c
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

#include "core/core-types.h"

#include "gegl/clay-babl.h"

#include "core/clay.h"
#include "core/clay-data-factories.h"
#include "core/claycontext.h"
#include "core/claydocumentlist.h"
#include "core/clayimage.h"
#include "core/clayimage-merge.h"
#include "core/clayimage-undo.h"
#include "core/clayimagefile.h"
#include "core/claylayer.h"
#include "core/claylink.h"
#include "core/claylinklayer.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"

#include "pdb/claypdb.h"

#include "plug-in/claypluginmanager-file.h"
#include "plug-in/claypluginprocedure.h"

#include "xcf/xcf-private.h"
#include "xcf/xcf-load.h"

#include "file-import.h"
#include "file-open.h"
#include "file-remote.h"
#include "clay-file.h"

#include "clay-intl.h"


static ClayImage * file_open_link_image           (Clay                 *clay,
                                                   ClayContext          *context,
                                                   ClayImage            *dest_image,
                                                   ClayProgress         *progress,
                                                   GFile                *file,
                                                   gint                  vector_width,
                                                   gint                  vector_height,
                                                   gboolean              vector_keep_ratio,
                                                   gboolean              as_new,
                                                   ClayPlugInProcedure  *file_proc,
                                                   ClayRunMode           run_mode,
                                                   gboolean             *file_proc_handles_vector,
                                                   ClayPDBStatusType    *status,
                                                   const gchar         **mime_type,
                                                   GError              **error);

static void        file_open_sanitize_image       (ClayImage            *image,
                                                   gboolean              as_new);
static void        file_open_convert_items        (ClayImage            *dest_image,
                                                   const gchar          *basename,
                                                   GList                *items);
static GList     * file_open_get_layers           (ClayImage            *image,
                                                   gboolean              merge_visible,
                                                   gint                 *n_visible);
static gboolean    file_open_valid_permissions    (GFile                *file,
                                                   GError              **error);
static gboolean    file_open_pre_check            (Clay                 *clay,
                                                   GFile                *file,
                                                   ClayImage            *dest_image,
                                                   gboolean              as_link,
                                                   ClayProgress         *progress,
                                                   ClayPlugInProcedure **file_proc,
                                                   GFile               **local_file,
                                                   ClayPDBStatusType    *status,
                                                   GError              **error);


/*  public functions  */

ClayImage *
file_open_image (Clay                *clay,
                 ClayContext         *context,
                 ClayProgress        *progress,
                 GFile               *file,
                 gint                 vector_width,
                 gint                 vector_height,
                 gboolean             vector_keep_ratio,
                 gboolean             as_new,
                 ClayPlugInProcedure *file_proc,
                 ClayRunMode          run_mode,
                 gboolean            *file_proc_handles_vector,
                 ClayPDBStatusType   *status,
                 const gchar        **mime_type,
                 GError             **error)
{
  ClayValueArray *return_vals;
  GFile          *orig_file;
  ClayImage      *image       = NULL;
  GFile          *local_file  = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! file_open_pre_check (clay, file, NULL, FALSE, progress,
                             &file_proc, &local_file, status, error))
    return NULL;

  *status = CLAY_PDB_EXECUTION_ERROR;

  orig_file = file;

  if (local_file)
    file = local_file;

  if (file_proc_handles_vector)
    *file_proc_handles_vector = file_proc->handles_vector;

  if (progress)
    g_object_add_weak_pointer (G_OBJECT (progress), (gpointer) &progress);

  if (file_proc->handles_vector)
    {
      return_vals =
        clay_pdb_execute_procedure_by_name (clay->pdb,
                                            context, progress, error,
                                            clay_object_get_name (file_proc),
                                            CLAY_TYPE_RUN_MODE, run_mode,
                                            G_TYPE_FILE,        file,
                                            G_TYPE_INT,         vector_width,
                                            G_TYPE_INT,         vector_height,
                                            G_TYPE_BOOLEAN,     vector_keep_ratio,
                                            G_TYPE_BOOLEAN,     vector_width && vector_height ? FALSE : TRUE,
                                            G_TYPE_NONE);
    }
  else
    {
      return_vals =
        clay_pdb_execute_procedure_by_name (clay->pdb,
                                            context, progress, error,
                                            clay_object_get_name (file_proc),
                                            CLAY_TYPE_RUN_MODE, run_mode,
                                            G_TYPE_FILE,        file,
                                            G_TYPE_NONE);
    }

  if (progress)
    g_object_remove_weak_pointer (G_OBJECT (progress), (gpointer) &progress);

  *status = g_value_get_enum (clay_value_array_index (return_vals, 0));

  if (*status == CLAY_PDB_SUCCESS && ! file_proc->generic_file_proc)
    image = g_value_get_object (clay_value_array_index (return_vals, 1));

  /*  set the file on all images, plug-ins are not required to do it
   *  any longer
   */
  if (image)
    clay_image_set_file (image, orig_file);

  if (local_file)
    {
      g_file_delete (local_file, NULL, NULL);
      g_object_unref (local_file);
    }

  if (*status == CLAY_PDB_SUCCESS)
    {
      if (image)
        {
          /* Only set the load procedure if it hasn't already been set.
           * The reason is that we want to know the information of the
           * inner format, in case loading the file went through
           * intermediate container file formats procedures, typically
           * the procedures registered by the file-compressor plug-in.
           *
           * E.g. it could be used for our compressed XCF, but also for
           * commonly compressed formats such as .hgt.zip.
           */
          if (! clay_image_get_load_proc (image))
            clay_image_set_load_proc (image, file_proc);

          file_proc = clay_image_get_load_proc (image);

          if (mime_type)
            *mime_type = g_slist_nth_data (file_proc->mime_types_list, 0);
        }
      else if (! file_proc->generic_file_proc)
        {
          if (error && ! *error)
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("%s plug-in returned SUCCESS but did not "
                           "return an image"),
                         clay_procedure_get_label (CLAY_PROCEDURE (file_proc)));

          *status = CLAY_PDB_EXECUTION_ERROR;
        }
    }
  else if (*status != CLAY_PDB_CANCEL)
    {
      if (error && ! *error)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("%s plug-in could not open image"),
                     clay_procedure_get_label (CLAY_PROCEDURE (file_proc)));
    }

  clay_value_array_unref (return_vals);

  if (image)
    {
      clay_image_undo_disable (image);

      if (! clay_plug_in_procedure_is_xcf_load (file_proc))
        {
          file_import_image (image, context, orig_file,
                             run_mode == CLAY_RUN_INTERACTIVE,
                             progress);
        }

      /* Enables undo again */
      file_open_sanitize_image (image, as_new);
    }

  return image;
}

/**
 * file_open_thumbnail:
 * @clay:
 * @context:
 * @progress:
 * @file:         an image file
 * @size:         requested size of the thumbnail
 * @mime_type:    return location for image MIME type
 * @image_width:  return location for image width
 * @image_height: return location for image height
 * @format:       return location for image format (set to NULL if unknown)
 * @num_layers:   return location for number of layers
 *                (set to -1 if the number of layers is not known)
 * @error:
 *
 * Attempts to load a thumbnail by using a registered thumbnail loader.
 *
 * Returns: the thumbnail image
 */
ClayImage *
file_open_thumbnail (Clay           *clay,
                     ClayContext    *context,
                     ClayProgress   *progress,
                     GFile          *file,
                     gint            size,
                     const gchar   **mime_type,
                     gint           *image_width,
                     gint           *image_height,
                     const Babl    **format,
                     gint           *num_layers,
                     GError        **error)
{
  ClayPlugInProcedure *file_proc;
  ClayProcedure       *procedure;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (image_width != NULL, NULL);
  g_return_val_if_fail (image_height != NULL, NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (num_layers != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *image_width  = 0;
  *image_height = 0;
  *format       = NULL;
  *num_layers   = -1;

  file_proc = clay_plug_in_manager_file_procedure_find (clay->plug_in_manager,
                                                        CLAY_FILE_PROCEDURE_GROUP_OPEN,
                                                        file, NULL);

  if (! file_proc || ! file_proc->thumb_loader)
    return NULL;

  procedure = clay_pdb_lookup_procedure (clay->pdb, file_proc->thumb_loader);

  if (procedure && procedure->num_args >= 2 && procedure->num_values >= 1)
    {
      ClayPDBStatusType  status;
      ClayValueArray    *return_vals;
      ClayImage         *image = NULL;
      gchar             *uri   = NULL;

      uri = g_file_get_uri (file);

      return_vals =
        clay_pdb_execute_procedure_by_name (clay->pdb,
                                            context, progress, error,
                                            clay_object_get_name (procedure),
                                            G_TYPE_FILE, file,
                                            G_TYPE_INT,  size,
                                            G_TYPE_NONE);

      g_free (uri);

      status = g_value_get_enum (clay_value_array_index (return_vals, 0));

      if (status == CLAY_PDB_SUCCESS &&
          CLAY_VALUE_HOLDS_IMAGE (clay_value_array_index (return_vals, 1)))
        {
          image = g_value_get_object (clay_value_array_index (return_vals, 1));

          if (clay_value_array_length (return_vals) >= 4 &&
              G_VALUE_HOLDS_INT (clay_value_array_index (return_vals, 2)) &&
              G_VALUE_HOLDS_INT (clay_value_array_index (return_vals, 3)))
            {
              *image_width =
                MAX (0, g_value_get_int (clay_value_array_index (return_vals, 2)));

              *image_height =
                MAX (0, g_value_get_int (clay_value_array_index (return_vals, 3)));

              if (clay_value_array_length (return_vals) >= 5 &&
                  G_VALUE_HOLDS_ENUM (clay_value_array_index (return_vals, 4)))
                {
                  ClayImageType itype = g_value_get_enum (clay_value_array_index (return_vals, 4));

                  switch (itype)
                    {
                    case CLAY_RGB_IMAGE:
                      *format = clay_babl_format (CLAY_RGB,
                                                  CLAY_PRECISION_U8_NON_LINEAR,
                                                  FALSE, NULL);
                      break;

                    case CLAY_RGBA_IMAGE:
                      *format = clay_babl_format (CLAY_RGB,
                                                  CLAY_PRECISION_U8_NON_LINEAR,
                                                  TRUE, NULL);
                      break;

                    case CLAY_GRAY_IMAGE:
                      *format = clay_babl_format (CLAY_GRAY,
                                                  CLAY_PRECISION_U8_NON_LINEAR,
                                                  FALSE, NULL);
                      break;

                    case CLAY_GRAYA_IMAGE:
                      *format = clay_babl_format (CLAY_GRAY,
                                                  CLAY_PRECISION_U8_NON_LINEAR,
                                                  TRUE, NULL);
                      break;

                    case CLAY_INDEXED_IMAGE:
                    case CLAY_INDEXEDA_IMAGE:
                      {
                        const Babl *rgb;
                        const Babl *rgba;

                        babl_new_palette ("-clay-indexed-format-dummy",
                                          &rgb, &rgba);

                        if (itype == CLAY_INDEXED_IMAGE)
                          *format = rgb;
                        else
                          *format = rgba;
                      }
                      break;

                    default:
                      break;
                    }
                }

              if (clay_value_array_length (return_vals) >= 6 &&
                  G_VALUE_HOLDS_INT (clay_value_array_index (return_vals, 5)))
                {
                  *num_layers =
                    MAX (0, g_value_get_int (clay_value_array_index (return_vals, 5)));
                }
            }

          if (image)
            {
              file_open_sanitize_image (image, FALSE);

              *mime_type = g_slist_nth_data (file_proc->mime_types_list, 0);

#ifdef CLAY_UNSTABLE
              g_printerr ("opened thumbnail at %d x %d\n",
                          clay_image_get_width  (image),
                          clay_image_get_height (image));
#endif
            }
        }

      clay_value_array_unref (return_vals);

      return image;
    }

  return NULL;
}

ClayImage *
file_open_with_display (Clay               *clay,
                        ClayContext        *context,
                        ClayProgress       *progress,
                        GFile              *file,
                        gboolean            as_new,
                        GObject            *monitor,
                        ClayPDBStatusType  *status,
                        GError            **error)
{
  return file_open_with_proc_and_display (clay, context, progress,
                                          file, as_new, FALSE, NULL,
                                          monitor, status, error);
}

ClayImage *
file_open_with_proc_and_display (Clay                *clay,
                                 ClayContext         *context,
                                 ClayProgress        *progress,
                                 GFile               *file,
                                 gboolean             as_new,
                                 gboolean             as_link,
                                 ClayPlugInProcedure *file_proc,
                                 GObject             *monitor,
                                 ClayPDBStatusType   *status,
                                 GError             **error)
{
  ClayImage   *image     = NULL;
  const gchar *mime_type = NULL;
  ClayRunMode  run_mode  = CLAY_RUN_INTERACTIVE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (monitor == NULL || G_IS_OBJECT (monitor), NULL);
  g_return_val_if_fail (status != NULL, NULL);

  if (clay->no_interface)
    run_mode = CLAY_RUN_NONINTERACTIVE;

  if (as_link)
    image = file_open_link_image (clay, context, NULL, progress,
                                  file, 0, 0, TRUE,
                                  as_new,
                                  file_proc,
                                  run_mode,
                                  NULL,
                                  status,
                                  &mime_type,
                                  error);
  else
    image = file_open_image (clay, context, progress,
                             file, 0, 0, TRUE,
                             as_new,
                             file_proc,
                             run_mode,
                             NULL,
                             status,
                             &mime_type,
                             error);

  if (image)
    {
      /* If the file was imported we want to set the layer name to the
       * file name. For now, assume that multi-layered imported images
       * have named the layers already, so only rename the layer of
       * single-layered imported files. Note that this will also
       * rename already named layers from e.g. single-layered PSD
       * files. To solve this properly, we would need new file plug-in
       * API.
       */
      if (! file_proc)
        file_proc = clay_image_get_load_proc (image);

      if (! clay_plug_in_procedure_is_xcf_load (file_proc) &&
          clay_image_get_n_layers (image) == 1)
        {
          ClayObject *layer = clay_image_get_layer_iter (image)->data;

          if (! CLAY_IS_LINK_LAYER (layer))
            {
              gchar *basename;

              basename = g_path_get_basename (clay_file_get_utf8_name (file));

              clay_item_rename (CLAY_ITEM (layer), basename, NULL);

              g_free (basename);
            }

          clay_image_undo_free (image);
          clay_image_clean_all (image);
        }

      if (clay_create_display (image->clay, image, clay_unit_pixel (), 1.0,
                               monitor))
        {
          /*  the display owns the image now  */
          g_object_unref (image);
        }

      if (! as_new)
        {
          ClayDocumentList *documents = CLAY_DOCUMENT_LIST (clay->documents);
          ClayImagefile    *imagefile;
          GFile            *any_file;

          imagefile = clay_document_list_add_file (documents, file, mime_type);

          /*  can only create a thumbnail if the passed file and the
           *  resulting image's file match. Use any_file() here so we
           *  create thumbnails for both XCF and imported images.
           */
          any_file = clay_image_get_any_file (image);

          if (any_file && g_file_equal (file, any_file))
            {
              /*  no need to save a thumbnail if there's a good one already  */
              if (! clay_imagefile_check_thumbnail (imagefile))
                {
                  clay_imagefile_save_thumbnail (imagefile, mime_type, image,
                                                 NULL);
                }
            }
        }

      /*  announce that we opened this image  */
      clay_image_opened (image->clay, file);
    }

  return image;
}

GList *
file_open_layers (Clay                *clay,
                  ClayContext         *context,
                  ClayProgress        *progress,
                  ClayImage           *dest_image,
                  gboolean             merge_visible,
                  gboolean             as_link,
                  GFile               *file,
                  ClayRunMode          run_mode,
                  ClayPlugInProcedure *file_proc,
                  ClayPDBStatusType   *status,
                  GError             **error)
{
  ClayImage   *new_image;
  GList       *layers    = NULL;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (as_link)
    new_image = file_open_link_image (clay, context, dest_image, progress,
                                      file,
                                      clay_image_get_width (dest_image),
                                      clay_image_get_height (dest_image),
                                      TRUE,
                                      FALSE,
                                      file_proc,
                                      run_mode,
                                      NULL, status, &mime_type, error);
  else
    new_image = file_open_image (clay, context, progress,
                                 file,
                                 clay_image_get_width (dest_image),
                                 clay_image_get_height (dest_image),
                                 TRUE,
                                 FALSE,
                                 file_proc,
                                 run_mode,
                                 NULL, status, &mime_type, error);

  if (new_image)
    {
      gint n_visible = 0;

      clay_image_undo_disable (new_image);

      layers = file_open_get_layers (new_image, merge_visible, &n_visible);

      if (merge_visible && n_visible > 1)
        {
          g_list_free (layers);

          layers = clay_image_merge_visible_layers (new_image, context,
                                                    CLAY_CLIP_TO_IMAGE,
                                                    FALSE, FALSE,
                                                    NULL);
          layers = g_list_copy (layers);
        }

      if (layers)
        {
          gchar *basename = NULL;

          if (! clay_plug_in_procedure_is_xcf_load (clay_image_get_load_proc (new_image)))
            basename = g_path_get_basename (clay_file_get_utf8_name (file));
          file_open_convert_items (dest_image, basename, layers);
          g_free (basename);

          clay_document_list_add_file (CLAY_DOCUMENT_LIST (clay->documents),
                                       file, mime_type);
        }
      else
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Image doesn't contain any layers"));
          *status = CLAY_PDB_EXECUTION_ERROR;
        }

      g_object_unref (new_image);
    }

  return g_list_reverse (layers);
}


/*  This function is called for filenames passed on the command-line
 *  or from the D-Bus service.
 */
gboolean
file_open_from_command_line (Clay     *clay,
                             GFile    *file,
                             gboolean  as_new,
                             GObject  *monitor)

{
  ClayImage         *image;
  ClayDisplay       *display;
  ClayPDBStatusType  status;
  gboolean           success = FALSE;
  GError            *error   = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (monitor == NULL || G_IS_OBJECT (monitor), FALSE);

  display = clay_get_empty_display (clay);

  /* show the progress in the last opened display, see bug #704896 */
  if (! display)
    display = clay_context_get_display (clay_get_user_context (clay));

  if (display)
    g_object_add_weak_pointer (G_OBJECT (display), (gpointer) &display);

  image = file_open_with_display (clay,
                                  clay_get_user_context (clay),
                                  CLAY_PROGRESS (display),
                                  file, as_new,
                                  monitor,
                                  &status, &error);

  if (image)
    {
      success = TRUE;

      clay_image_set_from_command_line (image, TRUE);
      g_object_set_data_full (G_OBJECT (clay), CLAY_FILE_OPEN_LAST_FILE_KEY,
                              g_object_ref (file),
                              (GDestroyNotify) g_object_unref);
    }
  else if (status != CLAY_PDB_SUCCESS && status != CLAY_PDB_CANCEL)
    {
      clay_message (clay, G_OBJECT (display), CLAY_MESSAGE_ERROR,
                    _("Opening '%s' failed: %s"),
                    clay_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  if (display)
    g_object_remove_weak_pointer (G_OBJECT (display), (gpointer) &display);

  return success;
}


/*  private functions  */

static ClayImage *
file_open_link_image (Clay                *clay,
                      ClayContext         *context,
                      ClayImage           *dest_image,
                      ClayProgress        *progress,
                      GFile               *file,
                      gint                 vector_width,
                      gint                 vector_height,
                      gboolean             vector_keep_ratio,
                      gboolean             as_new,
                      ClayPlugInProcedure *file_proc,
                      ClayRunMode          run_mode,
                      gboolean            *file_proc_handles_vector,
                      ClayPDBStatusType   *status,
                      const gchar        **mime_type,
                      GError             **error)
{
  ClayImage *image = NULL;
  ClayLink  *link;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (G_IS_FILE (file) && g_file_is_native (file), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! file_open_pre_check (clay, file, dest_image, TRUE, progress,
                             &file_proc, NULL, status, error))
    return NULL;

  link = clay_link_new (clay, file,
                        vector_width, vector_height, vector_keep_ratio,
                        progress, error);

  if (clay_link_is_broken (link))
    {
      *status = CLAY_PDB_EXECUTION_ERROR;
    }
  else
    {
      ClayLayer *layer;
      gint       width;
      gint       height;

      clay_link_get_size (link, &width, &height);
      image = clay_image_new (clay, width, height,
                              clay_link_get_base_type (link),
                              clay_link_get_precision (link));
      layer = clay_link_layer_new (image, link);
      clay_image_add_layer (image, layer, NULL, 0, FALSE);

      if (! clay_image_get_load_proc (image))
        clay_image_set_load_proc (image, clay_link_get_load_proc (link));

      file_proc = clay_image_get_load_proc (image);
      if (mime_type)
        *mime_type = g_slist_nth_data (file_proc->mime_types_list, 0);
      if (file_proc_handles_vector)
        *file_proc_handles_vector = file_proc->handles_vector;

      *status = CLAY_PDB_SUCCESS;
    }

  g_clear_object (&link);

  return image;
}

static void
file_open_sanitize_image (ClayImage *image,
                          gboolean   as_new)
{
  /* This should never occur, but apparently it does sometimes #13702
   * Avoids infinite loop in the while statement below.
   */
  g_return_if_fail (CLAY_IS_IMAGE (image));

  if (as_new)
    clay_image_set_file (image, NULL);

  /* clear all undo steps */
  clay_image_undo_free (image);

  /* make sure that undo is enabled */
  while (! clay_image_undo_is_enabled (image))
    clay_image_undo_thaw (image);

  /* Set the image to clean. Note that export dirtiness is not set to
   * clean here; we can only consider export clean after the first
   * export
   */
  clay_image_clean_all (image);

  /* Make sure the projection is completely constructed from valid
   * layers, this is needed in case something triggers projection or
   * image preview creation before all layers are loaded, see bug #767663.
   */
  clay_image_invalidate_all (image);

  /* Make sure all image states are up-to-date */
  clay_image_flush (image);
}

/* Converts items from one image to another */
static void
file_open_convert_items (ClayImage   *dest_image,
                         const gchar *basename,
                         GList       *items)
{
  GList *list;

  for (list = items; list; list = g_list_next (list))
    {
      ClayItem *src = list->data;
      ClayItem *item;

      item = clay_item_convert (src, dest_image, G_TYPE_FROM_INSTANCE (src));

      if (g_list_length (items) == 1 && basename != NULL)
        {
          clay_object_set_name (CLAY_OBJECT (item), basename);
        }
      else
        {
          clay_object_set_name (CLAY_OBJECT (item),
                                clay_object_get_name (src));
        }

      list->data = item;
    }
}

static GList *
file_open_get_layers (ClayImage *image,
                      gboolean   merge_visible,
                      gint      *n_visible)
{
  GList *iter   = NULL;
  GList *layers = NULL;

  for (iter = clay_image_get_layer_iter (image);
       iter;
       iter = g_list_next (iter))
    {
      ClayItem *item = iter->data;

      if (! merge_visible)
        layers = g_list_prepend (layers, item);

      if (clay_item_get_visible (item))
        {
          if (n_visible)
            (*n_visible)++;

          if (! layers)
            layers = g_list_prepend (layers, item);
        }
    }

  return layers;
}

static gboolean
file_open_valid_permissions (GFile   *file,
                             GError **error)
{
  /* FIXME enable these tests for remote files again, needs testing */
  if (g_file_is_native (file))
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                                G_FILE_QUERY_INFO_NONE,
                                NULL, error);

      if (info != NULL)
        {
          if (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) != G_FILE_TYPE_REGULAR)
            {
              g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                   _("Not a regular file"));
              g_object_unref (info);
              return FALSE;
            }

          if (! g_file_info_get_attribute_boolean (info,
                                                   G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
            {
              g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                   _("Permission denied"));
              g_object_unref (info);
              return FALSE;
            }

          g_object_unref (info);
        }
      else
        {
          /* File likely does not exists. error will already have a more
           * accurate reason.
           */
          return FALSE;
        }

      return TRUE;
    }

  return FALSE;
}

static gboolean
file_open_pre_check (Clay                 *clay,
                     GFile                *file,
                     ClayImage            *dest_image,
                     gboolean              as_link,
                     ClayProgress         *progress,
                     ClayPlugInProcedure **file_proc,
                     GFile               **local_file,
                     ClayPDBStatusType    *status,
                     GError              **error)
{
  gboolean  mounted  = TRUE;
  GError   *my_error = NULL;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (file_proc != NULL, FALSE);
  g_return_val_if_fail ((as_link && local_file == NULL) ||
                        (! as_link && local_file != NULL && *local_file == NULL),
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  *status = CLAY_PDB_EXECUTION_ERROR;

  if (g_file_is_native (file) && ! file_open_valid_permissions (file, error))
    return FALSE;

  if (! *file_proc)
    *file_proc = clay_plug_in_manager_file_procedure_find (clay->plug_in_manager,
                                                           CLAY_FILE_PROCEDURE_GROUP_OPEN,
                                                           file, error);

  if (as_link)
    {
      GFile *dest_file  = dest_image ? clay_image_get_file (dest_image) : NULL;

      if (! g_file_is_native (file))
        {
          gchar *uri = g_file_get_uri (file);

          g_clear_error (error);

          if (error && ! *error)
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Only platform-native file paths are supported: '%s' cannot be opened as link."),
                         uri);

          g_free (uri);

          return FALSE;
        }

      if (dest_file && clay_plug_in_procedure_is_xcf_load (*file_proc))
        {
          gboolean loop_found = xcf_load_file_equal (dest_file, file);

          if (! loop_found)
            {
              GInputStream  *input;
              GList         *parent_files;
              GList         *loop_files = NULL;
              XcfInfo        info       = { 0, };
              gint           width;
              gint           height;
              gint           type;
              ClayPrecision  precision;

              parent_files = g_list_prepend (NULL, dest_file);

              input = G_INPUT_STREAM (g_file_read (file, NULL, NULL));
              if (input && xcf_load_magic_version (clay, input, file, NULL, &info))
                xcf_load_image_header (clay, &info, &width, &height, &type, &precision,
                                       parent_files, &loop_files, &loop_found, NULL);

              g_clear_object (&input);
              g_list_free (parent_files);
              g_list_free_full (loop_files, g_object_unref);
            }

          if (loop_found)
            {
              g_clear_error (error);
              if (error && ! *error)
                g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                             _("Circular reference detected with '%s'."),
                             g_file_peek_path (file));

              return FALSE;
            }
        }

      return (*file_proc != NULL);
    }

  /* Below are checks relevant only for non-link file opening. */

  if (! g_file_is_native (file) &&
      ! file_remote_mount_file (clay, file, progress, &my_error))
    {
      if (my_error)
        {
          g_printerr ("%s: mounting remote volume failed, trying to download "
                      "the file: %s\n",
                      G_STRFUNC, my_error->message);
          g_clear_error (&my_error);

          mounted = FALSE;
        }
      else
        {
          *status = CLAY_PDB_CANCEL;
          g_clear_error (error);

          return FALSE;
        }
    }

  if (! *file_proc || ! (*file_proc)->handles_remote || ! mounted)
    {
      gchar *my_path = g_file_get_path (file);

      if (! my_path)
        {
          g_clear_error (error);

          *local_file = file_remote_download_image (clay, file, progress,
                                                    &my_error);

          if (! *local_file)
            {
              if (my_error)
                g_propagate_error (error, my_error);
              else
                *status = CLAY_PDB_CANCEL;

              return FALSE;
            }

          /*  if we don't have a file proc yet, try again on the local
           *  file
           */
          if (! *file_proc)
            *file_proc = clay_plug_in_manager_file_procedure_find (clay->plug_in_manager,
                                                                   CLAY_FILE_PROCEDURE_GROUP_OPEN,
                                                                   *local_file, error);

          if (! *file_proc)
            {
              g_file_delete (*local_file, NULL, NULL);
              g_clear_object (local_file);

              return FALSE;
            }
        }

      g_free (my_path);
    }

  if (*file_proc != NULL &&
      clay_plug_in_procedure_is_xcf_load (*file_proc))
    return clay_data_factories_wait (clay);

  return (*file_proc != NULL);
}
