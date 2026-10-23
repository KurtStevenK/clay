/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayexportdialog.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/clayimage.h"

#include "config/claycoreconfig.h"

#include "file/clay-file.h"

#include "clayexportdialog.h"
#include "clayhelp-ids.h"

#include "clay-intl.h"


G_DEFINE_TYPE (ClayExportDialog, clay_export_dialog,
               CLAY_TYPE_FILE_DIALOG)

#define parent_class clay_export_dialog_parent_class


static void
clay_export_dialog_class_init (ClayExportDialogClass *klass)
{
}

static void
clay_export_dialog_init (ClayExportDialog *dialog)
{
}


/*  public functions  */

GtkWidget *
clay_export_dialog_new (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_new (CLAY_TYPE_EXPORT_DIALOG,
                       "clay",                  clay,
                       "title",                 _("Export Image"),
                       "role",                  "clay-file-export",
                       "help-id",               CLAY_HELP_FILE_EXPORT_AS,
                       "ok-button-label",       _("_Export"),

                       "automatic-label",       _("By Extension"),
                       "automatic-help-id",     CLAY_HELP_FILE_SAVE_BY_EXTENSION,

                       "action",                GTK_FILE_CHOOSER_ACTION_SAVE,
                       "file-procs",            CLAY_FILE_PROCEDURE_GROUP_EXPORT,
                       "file-procs-all-images", CLAY_FILE_PROCEDURE_GROUP_SAVE,
                       "file-filter-label",     _("All export images"),
                       NULL);
}

void
clay_export_dialog_set_image (ClayExportDialog *dialog,
                              ClayImage        *image,
                              ClayObject       *display)
{
  ClayFileDialog *file_dialog;
  GFile          *dir_file  = NULL;
  GFile          *name_file = NULL;
  GFile          *ext_file  = NULL;
  gchar          *basename;

  g_return_if_fail (CLAY_IS_EXPORT_DIALOG (dialog));
  g_return_if_fail (CLAY_IS_IMAGE (image));

  file_dialog = CLAY_FILE_DIALOG (dialog);

  file_dialog->image = image;
  dialog->display    = display;

  clay_file_dialog_set_file_proc (file_dialog, NULL);

  /* Priority of default paths for Export:
   *
   *   1. Last Export path
   *   2. Path of import source
   *   3. Path of XCF source
   *   4. Last path of any save to XCF
   *   5. Last Export path of any document
   *   6. The default path (usually the OS 'Documents' path)
   */

  dir_file = clay_image_get_exported_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (image),
                                  "clay-image-source-file");

  if (! dir_file)
    dir_file = clay_image_get_imported_file (image);

  if (! dir_file)
    dir_file = clay_image_get_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (file_dialog->clay),
                                  CLAY_FILE_SAVE_LAST_FILE_KEY);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (file_dialog->clay),
                                  CLAY_FILE_EXPORT_LAST_FILE_KEY);

  if (! dir_file)
    dir_file = clay_file_dialog_get_default_folder (file_dialog);

  /* Priority of default basenames for Export:
   *
   *   1. Last Export name
   *   3. Save URI
   *   2. Source file name
   *   3. 'Untitled'
   */

  name_file = clay_image_get_exported_file (image);

  if (! name_file)
    name_file = clay_image_get_file (image);

  if (! name_file)
    name_file = clay_image_get_imported_file (image);

  if (! name_file)
    name_file = clay_image_get_untitled_file (image);


  /* Priority of default type/extension for Export:
   *
   *   1. Type of last Export
   *   2. Type of the image Import
   *   3. Type of latest Export of any document
   *   4. Default file type set in Preferences
   */

  ext_file = clay_image_get_exported_file (image);

  if (! ext_file)
    ext_file = clay_image_get_imported_file (image);

  if (! ext_file)
    ext_file = g_object_get_data (G_OBJECT (file_dialog->clay),
                                  CLAY_FILE_EXPORT_LAST_FILE_KEY);

  if (ext_file)
    {
      g_object_ref (ext_file);
    }
  else
    {
      const gchar *extension;
      gchar       *uri;

      clay_enum_get_value (CLAY_TYPE_EXPORT_FILE_TYPE,
                           image->clay->config->export_file_type,
                           NULL, &extension, NULL, NULL);

      uri = g_strconcat ("file:///we/only/care/about/extension.",
                         extension, NULL);
      ext_file = g_file_new_for_uri (uri);
      g_free (uri);
    }

  if (ext_file)
    {
      GFile *tmp_file = clay_file_with_new_extension (name_file, ext_file);
      basename = g_path_get_basename (clay_file_get_utf8_name (tmp_file));
      g_object_unref (tmp_file);
      g_object_unref (ext_file);
    }
  else
    {
      basename = g_path_get_basename (clay_file_get_utf8_name (name_file));
    }

  if (g_file_query_file_type (dir_file, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                dir_file, NULL);
    }
  else
    {
      GFile *parent_file = g_file_get_parent (dir_file);
      gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                parent_file, NULL);
      g_object_unref (parent_file);
    }

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);
}
