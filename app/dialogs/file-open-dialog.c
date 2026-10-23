/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/clayprogress.h"

#include "file/file-open.h"
#include "file/clay-file.h"

#include "widgets/clayfiledialog.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayopendialog.h"
#include "widgets/claywidgets-utils.h"

#include "file-open-dialog.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void       file_open_dialog_response    (GtkWidget           *dialog,
                                                gint                 response_id,
                                                Clay                *clay);
static ClayImage *file_open_dialog_open_image  (GtkWidget           *dialog,
                                                Clay                *clay,
                                                GFile               *file,
                                                ClayPlugInProcedure *load_proc,
                                                gboolean             as_link);
static gboolean   file_open_dialog_open_layers (GtkWidget           *dialog,
                                                ClayImage           *image,
                                                GFile               *file,
                                                ClayPlugInProcedure *load_proc,
                                                gboolean             as_link);


/*  public functions  */

GtkWidget *
file_open_dialog_new (Clay *clay)
{
  GtkWidget *dialog;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  dialog = clay_open_dialog_new (clay);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  clay_file_dialog_load_state (CLAY_FILE_DIALOG (dialog),
                               "clay-file-open-dialog-state");

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_dialog_response),
                    clay);

  return dialog;
}


/*  private functions  */

static void
file_open_dialog_response (GtkWidget *dialog,
                           gint       response_id,
                           Clay      *clay)
{
  ClayFileDialog *file_dialog = CLAY_FILE_DIALOG (dialog);
  ClayOpenDialog *open_dialog = CLAY_OPEN_DIALOG (dialog);
  GSList         *files;
  GSList         *list;
  gboolean        success = FALSE;

  clay_file_dialog_save_state (CLAY_FILE_DIALOG (dialog),
                               "clay-file-open-dialog-state");

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! file_dialog->busy && response_id != GTK_RESPONSE_HELP)
        gtk_widget_destroy (dialog);

      return;
    }

  files = gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog));

  if (files)
    g_object_set_data_full (G_OBJECT (clay), CLAY_FILE_OPEN_LAST_FILE_KEY,
                            g_object_ref (files->data),
                            (GDestroyNotify) g_object_unref);

  clay_file_dialog_set_sensitive (file_dialog, FALSE);

  /* When we are going to open new image windows, unset the transient
   * window. We don't need it since we will use gdk_window_raise() to
   * keep the dialog on top. And if we don't do it, then the dialog
   * will pull the image window it was invoked from on top of all the
   * new opened image windows, and we don't want that to happen.
   */
  if (! open_dialog->open_as_layers)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), NULL);

  if (file_dialog->image)
    g_object_ref (file_dialog->image);

  /* If we open multiple files as layers, compress the undos */
  if (file_dialog->image          &&
      open_dialog->open_as_layers &&
      g_slist_length (files) > 1)
    clay_image_undo_group_start (file_dialog->image,
                                 CLAY_UNDO_GROUP_LAYER_ADD,
                                 _("Open layers"));

  for (list = files; list; list = g_slist_next (list))
    {
      GFile *file = list->data;

      if (open_dialog->open_as_layers)
        {
          if (! file_dialog->image)
            {
              clay_open_dialog_set_image (open_dialog,
                                          file_open_dialog_open_image (dialog,
                                                                       clay,
                                                                       file,
                                                                       file_dialog->file_proc,
                                                                       open_dialog->open_as_link),
                                          TRUE,
                                          open_dialog->open_as_link);

              if (file_dialog->image)
                {
                  g_object_ref (file_dialog->image);

                  if (g_slist_length (files) > 1)
                    clay_image_undo_group_start (file_dialog->image,
                                                 CLAY_UNDO_GROUP_LAYER_ADD,
                                                 _("Open layers"));

                  success = TRUE;
                }
            }
          else if (file_open_dialog_open_layers (dialog,
                                                 file_dialog->image,
                                                 file,
                                                 file_dialog->file_proc,
                                                 open_dialog->open_as_link))
            {
              success = TRUE;
            }
        }
      else
        {
          if (file_open_dialog_open_image (dialog,
                                           clay,
                                           file,
                                           file_dialog->file_proc,
                                           open_dialog->open_as_link))
            {
              success = TRUE;

              /* Make the dialog stay on top of all images we open if
               * we open say 10 at once
               */
              gdk_window_raise (gtk_widget_get_window (dialog));
            }
        }

      if (file_dialog->canceled)
        break;
    }

  if (file_dialog->image          &&
      open_dialog->open_as_layers &&
      g_slist_length (files) > 1)
    clay_image_undo_group_end (file_dialog->image);

  if (success)
    {
      if (file_dialog->image)
        {
          if (open_dialog->open_as_layers)
            clay_image_flush (file_dialog->image);

          g_object_unref (file_dialog->image);
        }

      gtk_widget_destroy (dialog);
    }
  else
    {
      if (file_dialog->image)
        g_object_unref (file_dialog->image);

      clay_file_dialog_set_sensitive (file_dialog, TRUE);
    }

  g_slist_free_full (files, (GDestroyNotify) g_object_unref);
}

static ClayImage *
file_open_dialog_open_image (GtkWidget           *dialog,
                             Clay                *clay,
                             GFile               *file,
                             ClayPlugInProcedure *load_proc,
                             gboolean             as_link)
{
  ClayImage         *image;
  ClayPDBStatusType  status;
  GError            *error = NULL;

  image = file_open_with_proc_and_display (clay,
                                           clay_get_user_context (clay),
                                           CLAY_PROGRESS (dialog),
                                           file, FALSE, as_link,
                                           load_proc,
                                           G_OBJECT (clay_widget_get_monitor (dialog)),
                                           &status, &error);

  if (! image && status != CLAY_PDB_SUCCESS && status != CLAY_PDB_CANCEL)
    {
      if (error)
        clay_message (clay, G_OBJECT (dialog), CLAY_MESSAGE_ERROR,
                      _("Opening '%s' failed:\n\n%s"),
                      clay_file_get_utf8_name (file), error->message);
      else
        clay_message (clay, G_OBJECT (dialog), CLAY_MESSAGE_ERROR,
                      _("Opening '%s' failed."),
                      clay_file_get_utf8_name (file));
      g_clear_error (&error);
    }

  return image;
}

static gboolean
file_open_dialog_open_layers (GtkWidget           *dialog,
                              ClayImage           *image,
                              GFile               *file,
                              ClayPlugInProcedure *load_proc,
                              gboolean             as_link)
{
  GList             *new_layers;
  ClayPDBStatusType  status;
  GError            *error = NULL;

  new_layers = file_open_layers (image->clay,
                                 clay_get_user_context (image->clay),
                                 CLAY_PROGRESS (dialog),
                                 image, FALSE, as_link,
                                 file, CLAY_RUN_INTERACTIVE, load_proc,
                                 &status, &error);

  if (new_layers)
    {
      clay_image_add_layers (image, new_layers,
                             CLAY_IMAGE_ACTIVE_PARENT, -1,
                             0, 0,
                             clay_image_get_width (image),
                             clay_image_get_height (image),
                             _("Open layers"));

      g_list_free (new_layers);

      return TRUE;
    }
  else if (status != CLAY_PDB_CANCEL)
    {
      clay_message (image->clay, G_OBJECT (dialog), CLAY_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    clay_file_get_utf8_name (file), error ? error->message: _("n/a"));
      g_clear_error (&error);
    }

  return FALSE;
}
