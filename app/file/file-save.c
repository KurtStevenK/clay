/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-save.c
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

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydocumentlist.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayimagefile.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"

#include "pdb/claypdb.h"

#include "plug-in/claypluginprocedure.h"

#include "file-remote.h"
#include "file-save.h"
#include "clay-file.h"

#include "clay-intl.h"


/*  public functions  */

ClayPDBStatusType
file_save (Clay                *clay,
           ClayImage           *image,
           ClayProgress        *progress,
           GFile               *file,
           ClayPlugInProcedure *file_proc,
           ClayRunMode          run_mode,
           gboolean             change_saved_state,
           gboolean             export_backward,
           gboolean             export_forward,
           GError             **error)
{
  ClayValueArray    *return_vals;
  GFile             *orig_file;
  ClayPDBStatusType  status     = CLAY_PDB_EXECUTION_ERROR;
  GFile             *local_file = NULL;
  gboolean           mounted    = TRUE;
  GError            *my_error   = NULL;
  ClayExportOptions *options    = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), CLAY_PDB_CALLING_ERROR);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), CLAY_PDB_CALLING_ERROR);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress),
                        CLAY_PDB_CALLING_ERROR);
  g_return_val_if_fail (G_IS_FILE (file), CLAY_PDB_CALLING_ERROR);
  g_return_val_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (file_proc),
                        CLAY_PDB_CALLING_ERROR);
  g_return_val_if_fail ((export_backward && export_forward) == FALSE,
                        CLAY_PDB_CALLING_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL,
                        CLAY_PDB_CALLING_ERROR);

  orig_file = file;

  /*  ref image and file, so they can't get deleted during save  */
  g_object_ref (image);
  g_object_ref (orig_file);

  clay_image_saving (image);

  /* XXX - In the future, we'll pass special generic export options this
   * way (e.g. crops, resize, filter run at export or others).
   */
  options = g_object_new (CLAY_TYPE_EXPORT_OPTIONS, NULL);

  /* FIXME enable these tests for remote files again, needs testing */
  if (g_file_is_native (file) &&
      g_file_query_exists (file, NULL))
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                G_FILE_QUERY_INFO_NONE,
                                NULL, error);
      if (! info)
        {
          /* extra paranoia */
          if (error && ! *error)
            g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 _("Failed to get file information"));
          goto out;
        }

      if (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) != G_FILE_TYPE_REGULAR)
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Not a regular file"));
          g_object_unref (info);
          goto out;
        }

      if (! g_file_info_get_attribute_boolean (info,
                                               G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Permission denied"));
          g_object_unref (info);
          goto out;
        }

      g_object_unref (info);
    }

  if (! g_file_is_native (file) &&
      ! file_remote_mount_file (clay, file, progress, &my_error))
    {
      if (my_error)
        {
          g_printerr ("%s: mounting remote volume failed, trying to upload "
                      "the file: %s\n",
                      G_STRFUNC, my_error->message);
          g_clear_error (&my_error);

          mounted = FALSE;
        }
      else
        {
          status = CLAY_PDB_CANCEL;

          goto out;
        }
    }

  if (! file_proc->handles_remote || ! mounted)
    {
      gchar *my_path = g_file_get_path (file);

      if (! my_path)
        {
          local_file = file_remote_upload_image_prepare (clay, file, progress,
                                                         &my_error);

          if (! local_file)
            {
              if (my_error)
                g_propagate_error (error, my_error);
              else
                status = CLAY_PDB_CANCEL;

              goto out;
            }

          file = local_file;
        }

      g_free (my_path);
    }

  return_vals =
    clay_pdb_execute_procedure_by_name (image->clay->pdb,
                                        clay_get_user_context (clay),
                                        progress, error,
                                        clay_object_get_name (file_proc),
                                        CLAY_TYPE_RUN_MODE,       run_mode,
                                        CLAY_TYPE_IMAGE,          image,
                                        G_TYPE_FILE,              file,
                                        CLAY_TYPE_EXPORT_OPTIONS, options,
                                        G_TYPE_NONE);
  status = g_value_get_enum (clay_value_array_index (return_vals, 0));

  clay_value_array_unref (return_vals);

  if (local_file)
    {
      if (status == CLAY_PDB_SUCCESS)
        {
          GError *my_error = NULL;

          if (! file_remote_upload_image_finish (clay, orig_file, local_file,
                                                 progress, &my_error))
            {
              status = CLAY_PDB_EXECUTION_ERROR;

              if (my_error)
                g_propagate_error (error, my_error);
              else
                status = CLAY_PDB_CANCEL;
            }
        }

      g_file_delete (local_file, NULL, NULL);
      g_object_unref (local_file);
    }

  if (status == CLAY_PDB_SUCCESS)
    {
      ClayDocumentList *documents;
      ClayImagefile    *imagefile;

      if (change_saved_state)
        {
          clay_image_set_file (image, orig_file);
          clay_image_set_save_proc (image, file_proc);

          /* Forget the import source when we save. We interpret a
           * save as that the user is not interested in being able
           * to quickly export back to the original any longer
           */
          clay_image_set_imported_file (image, NULL);

          clay_image_clean_all (image);
        }
      else if (export_backward)
        {
          /* We exported the image back to its imported source,
           * change nothing about export/import flags, only set
           * the export state to clean
           */
          clay_image_export_clean_all (image);
        }
      else if (export_forward)
        {
          /* Remember the last entered Export URI for the image. We
           * only need to do this explicitly when exporting. It
           * happens implicitly when saving since the ClayObject name
           * of a ClayImage is the last-save URI
           */
          clay_image_set_exported_file (image, orig_file);
          clay_image_set_export_proc (image, file_proc);

          /* An image can not be considered both exported and imported
           * at the same time, so stop consider it as imported now
           * that we consider it exported.
           */
          clay_image_set_imported_file (image, NULL);

          clay_image_export_clean_all (image);
        }

      if (export_backward || export_forward)
        clay_image_exported (image, orig_file);
      else
        clay_image_saved (image, orig_file);

      documents = CLAY_DOCUMENT_LIST (image->clay->documents);

      imagefile = clay_document_list_add_file (documents, orig_file,
                                               g_slist_nth_data (file_proc->mime_types_list, 0));

      /* only save a thumbnail if we are saving as XCF, see bug #25272 */
      if (CLAY_PROCEDURE (file_proc)->proc_type == CLAY_PDB_PROC_TYPE_INTERNAL)
        clay_imagefile_save_thumbnail (imagefile,
                                       g_slist_nth_data (file_proc->mime_types_list, 0),
                                       image,
                                       NULL);
    }
  else if (status != CLAY_PDB_CANCEL)
    {
      if (error && *error == NULL)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("%s plug-in could not save image"),
                       clay_procedure_get_label (CLAY_PROCEDURE (file_proc)));
        }
    }

  clay_image_flush (image);

 out:
  g_object_unref (orig_file);
  g_object_unref (image);

  return status;
}
