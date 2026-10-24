/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "actions-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/clayimagefile.h"
#include "core/clayprogress.h"
#include "core/claytemplate.h"

#include "plug-in/claypluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"
#include "file/clay-file.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayclipboard.h"
#include "widgets/claydialogfactory.h"
#include "widgets/clayexportdialog.h"
#include "widgets/clayfiledialog.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/clayopendialog.h"
#include "widgets/claysavedialog.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplay-foreach.h"

#include "dialogs/dialogs.h"
#include "dialogs/file-save-dialog.h"

#include "actions.h"
#include "file-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void        file_open_dialog_show        (Clay         *clay,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 ClayImage    *image,
                                                 GFile        *file,
                                                 gboolean      open_as_layers,
                                                 gboolean      open_as_link);
static GtkWidget * file_save_dialog_show        (Clay         *clay,
                                                 ClayImage    *image,
                                                 GtkWidget    *parent,
                                                 const gchar  *title,
                                                 gboolean      save_a_copy,
                                                 gboolean      close_after_saving,
                                                 ClayDisplay  *display);
static GtkWidget * file_export_dialog_show      (Clay         *clay,
                                                 ClayImage    *image,
                                                 GtkWidget    *parent,
                                                 ClayDisplay  *display);
static void        file_save_dialog_response    (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 gpointer      data);
static void        file_export_dialog_response  (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 gpointer      data);
static void        file_new_template_callback   (GtkWidget    *widget,
                                                 const gchar  *name,
                                                 gpointer      data);
static void        file_revert_confirm_response (GtkWidget    *dialog,
                                                 gint          response_id,
                                                 ClayDisplay  *display);



/*  public functions  */


void
file_open_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Clay        *clay;
  GtkWidget   *widget;
  ClayImage   *image;
  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  image = action_data_get_image (data);

  file_open_dialog_show (clay, widget,
                         _("Open Image"),
                         image, NULL, FALSE, FALSE);
}

void
file_open_as_layers_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  Clay        *clay;
  GtkWidget   *widget;
  ClayDisplay *display;
  ClayImage   *image = NULL;
  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  display = action_data_get_display (data);

  if (display)
    image = clay_display_get_image (display);

  file_open_dialog_show (clay, widget,
                         _("Open Image as Layers"),
                         image, NULL, TRUE, FALSE);
}

void
file_open_as_link_layer_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  Clay        *clay;
  GtkWidget   *widget;
  ClayDisplay *display;
  ClayImage   *image = NULL;
  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  display = action_data_get_display (data);

  if (display)
    image = clay_display_get_image (display);

  file_open_dialog_show (clay, widget,
                         _("Open Image as Link Layer"),
                         image, NULL, TRUE, TRUE);
}

void
file_open_location_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                  clay_widget_get_monitor (widget),
                                  NULL /*ui_manager*/,
                                  widget,
                                  "clay-file-open-location-dialog", -1, TRUE);
}

void
file_open_recent_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  Clay          *clay;
  ClayImagefile *imagefile;
  gint           index;
  gint           num_entries;
  return_if_no_clay (clay, data);

  index = g_variant_get_int32 (value);

  num_entries = clay_container_get_n_children (clay->documents);

  if (index >= num_entries)
    return;

  imagefile = (ClayImagefile *)
    clay_container_get_child_by_index (clay->documents, index);

  if (imagefile)
    {
      GFile             *file;
      ClayDisplay       *display;
      GtkWidget         *widget;
      ClayProgress      *progress;
      ClayImage         *image;
      ClayPDBStatusType  status;
      GError            *error = NULL;
      return_if_no_display (display, data);
      return_if_no_widget (widget, data);

      g_object_ref (display);
      g_object_ref (imagefile);

      file = clay_imagefile_get_file (imagefile);

      progress = clay_display_get_image (display) ?
                 NULL : CLAY_PROGRESS (display);

      image = file_open_with_display (clay, action_data_get_context (data),
                                      progress,
                                      file, FALSE,
                                      G_OBJECT (clay_widget_get_monitor (widget)),
                                      &status, &error);

      if (! image && status != CLAY_PDB_CANCEL)
        {
          clay_message (clay, G_OBJECT (display), CLAY_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (imagefile);
      g_object_unref (display);
    }
}

void
file_save_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Clay         *clay;
  ClayDisplay  *display;
  ClayImage    *image;
  GtkWidget    *widget;
  ClaySaveMode  save_mode;
  GFile        *file  = NULL;
  gboolean      saved = FALSE;
  return_if_no_clay (clay, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = clay_display_get_image (display);

  save_mode = (ClaySaveMode) g_variant_get_int32 (value);

  file = clay_image_get_file (image);

  switch (save_mode)
    {
    case CLAY_SAVE_MODE_SAVE:
    case CLAY_SAVE_MODE_SAVE_AND_CLOSE:
      /*  Only save if the image has been modified, or if it is new.  */
      if ((clay_image_is_dirty (image) ||
           ! CLAY_GUI_CONFIG (image->clay->config)->trust_dirty_flag) ||
          file == NULL)
        {
          ClayPlugInProcedure *save_proc  = clay_image_get_save_proc (image);
          gboolean             valid_file = FALSE;

          if (file)
            {
              gchar *uri = g_file_get_uri (file);

              /* Non-valid URI (such as "Untitled.xcd" without a scheme) are
               * considered non-native by GLib and will trigger remote file code
               * path in file_save_dialog_save_image(), eventually failing with
               * a weird error. When we encounter such non-valid URI, we just
               * consider that the file was entered manually with a bogus name
               * (possibly by some script or plug-in) and we fall through
               * directly to showing the file dialog. The file name will still
               * be useful as default file name.
               */
              valid_file = g_uri_is_valid (uri, G_URI_FLAGS_NONE, NULL);
              g_free (uri);
            }

          if (valid_file && ! save_proc)
            {
              save_proc =
                clay_plug_in_manager_file_procedure_find (image->clay->plug_in_manager,
                                                          CLAY_FILE_PROCEDURE_GROUP_SAVE,
                                                          file, NULL);
            }

          if (valid_file && save_proc)
            {
              saved = file_save_dialog_save_image (CLAY_PROGRESS (display),
                                                   clay, image, file,
                                                   save_proc,
                                                   CLAY_RUN_WITH_LAST_VALS,
                                                   TRUE, FALSE, FALSE,
                                                   clay_image_get_xcf_compression (image),
                                                   TRUE);
              break;
            }

          /* fall thru */
        }
      else
        {
          clay_message_literal (image->clay,
                                G_OBJECT (display), CLAY_MESSAGE_INFO,
                                _("No changes need to be saved"));
          saved = TRUE;
          break;
        }

    case CLAY_SAVE_MODE_SAVE_AS:
      file_save_dialog_show (clay, image, widget,
                             _("Save Image"), FALSE,
                             save_mode == CLAY_SAVE_MODE_SAVE_AND_CLOSE, display);
      break;

    case CLAY_SAVE_MODE_SAVE_A_COPY:
      file_save_dialog_show (clay, image, widget,
                             _("Save a Copy of the Image"), TRUE,
                             FALSE, display);
      break;

    case CLAY_SAVE_MODE_EXPORT_AS:
      file_export_dialog_show (clay, image, widget, display);
      break;

    case CLAY_SAVE_MODE_EXPORT:
    case CLAY_SAVE_MODE_OVERWRITE:
      {
        GFile               *file        = NULL;
        ClayPlugInProcedure *export_proc = NULL;
        gboolean             overwrite   = FALSE;

        if (save_mode == CLAY_SAVE_MODE_EXPORT)
          {
            file        = clay_image_get_exported_file (image);
            export_proc = clay_image_get_export_proc (image);

            if (! file)
              {
                /* Behave as if Export As... was invoked */
                file_export_dialog_show (clay, image, widget, display);
                break;
              }

            overwrite = FALSE;
          }
        else if (save_mode == CLAY_SAVE_MODE_OVERWRITE)
          {
            file = clay_image_get_imported_file (image);

            overwrite = TRUE;
          }

        if (file && ! export_proc)
          {
            export_proc =
              clay_plug_in_manager_file_procedure_find (image->clay->plug_in_manager,
                                                        CLAY_FILE_PROCEDURE_GROUP_EXPORT,
                                                        file, NULL);
          }

        if (file && export_proc)
          {
            saved = file_save_dialog_save_image (CLAY_PROGRESS (display),
                                                 clay, image, file,
                                                 export_proc,
                                                 CLAY_RUN_WITH_LAST_VALS,
                                                 FALSE,
                                                 overwrite, ! overwrite,
                                                 FALSE, TRUE);
          }
      }
      break;
    }

  if (save_mode == CLAY_SAVE_MODE_SAVE_AND_CLOSE &&
      saved &&
      ! clay_image_is_dirty (image))
    {
      clay_display_close (display);
    }
}

void
file_create_template_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

  dialog = clay_query_string_box (_("Create New Template"),
                                  GTK_WIDGET (clay_display_get_shell (display)),
                                  clay_standard_help_func,
                                  CLAY_HELP_FILE_CREATE_TEMPLATE,
                                  _("Enter a name for this template"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  file_new_template_callback,
                                  image, NULL);
  gtk_widget_set_visible (dialog, TRUE);
}

void
file_revert_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  GFile       *file;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

  file = clay_image_get_file (image);

  if (! file)
    file = clay_image_get_imported_file (image);

  if (! file)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (display), CLAY_MESSAGE_ERROR,
                            _("Revert failed. "
                              "No file name associated with this image."));
      return;
    }

#define REVERT_DIALOG_KEY "clay-revert-confirm-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), REVERT_DIALOG_KEY);

  if (! dialog)
    {
      dialog =
        clay_message_dialog_new (_("Revert Image"), CLAY_ICON_DOCUMENT_REVERT,
                                 GTK_WIDGET (clay_display_get_shell (display)),
                                 0,
                                 clay_standard_help_func, CLAY_HELP_FILE_REVERT,

                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                 _("_Revert"), GTK_RESPONSE_OK,

                                 NULL);

      clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect_object (display, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (file_revert_confirm_response),
                        display);

      clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                         _("Revert '%s' to '%s'?"),
                                         clay_image_get_display_name (image),
                                         clay_file_get_utf8_name (file));

      clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                 _("By reverting the image to the state saved "
                                   "on disk, you will lose all changes, "
                                   "including all undo information."));

      dialogs_attach_dialog (G_OBJECT (image), REVERT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
file_close_all_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  Clay *clay;
  return_if_no_clay (clay, data);

  if (! clay_displays_dirty (clay))
    {
      clay_displays_close (clay);
    }
  else
    {
      GtkWidget *widget;
      return_if_no_widget (widget, data);

      clay_dialog_factory_dialog_raise (clay_dialog_factory_get_singleton (),
                                        clay_widget_get_monitor (widget),
                                        widget,
                                        "clay-close-all-dialog", -1);
    }
}

void
file_copy_location_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  Clay         *clay;
  ClayDisplay  *display;
  ClayImage    *image;
  GFile        *file;
  return_if_no_clay (clay, data);
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

  file = clay_image_get_any_file (image);

  if (file)
    {
      gchar *uri = g_file_get_uri (file);

      clay_clipboard_set_text (clay, uri);
      g_free (uri);
    }
}

void
file_show_in_file_manager_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  Clay         *clay;
  ClayDisplay  *display;
  ClayImage    *image;
  GFile        *file;
  return_if_no_clay (clay, data);
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

  file = clay_image_get_any_file (image);

  if (file)
    {
      GError *error = NULL;

      if (! clay_file_show_in_file_manager (file, &error))
        {
          clay_message (clay, G_OBJECT (display), CLAY_MESSAGE_ERROR,
                        _("Can't show file in file manager: %s"),
                        error->message);
          g_clear_error (&error);
        }
    }
}

void
file_quit_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Clay *clay;
  return_if_no_clay (clay, data);

  clay_exit (clay, FALSE);
}

void
file_restart_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  Clay *clay;
  return_if_no_clay (clay, data);

  clay_restart (clay);
}

void
file_file_open_dialog (Clay      *clay,
                       GFile     *file,
                       GtkWidget *parent)
{
  file_open_dialog_show (clay, parent,
                         _("Open Image"),
                         NULL, file, FALSE, FALSE);
}


/*  private functions  */

static void
file_open_dialog_show (Clay        *clay,
                       GtkWidget   *parent,
                       const gchar *title,
                       ClayImage   *image,
                       GFile       *file,
                       gboolean     open_as_layers,
                       gboolean     open_as_link)
{
  GtkWidget *dialog;

  dialog = clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                           clay_widget_get_monitor (parent),
                                           NULL /*ui_manager*/,
                                           parent,
                                           "clay-file-open-dialog", -1, FALSE);

  if (dialog)
    {
      if (! file && image)
        file = clay_image_get_file (image);

      if (! file)
        file = g_object_get_data (G_OBJECT (clay),
                                  CLAY_FILE_OPEN_LAST_FILE_KEY);

      if (file)
        {
          gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, NULL);
        }
      else if (clay->default_folder)
        {
          gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                    clay->default_folder, NULL);
        }

      gtk_window_set_title (GTK_WINDOW (dialog), title);

      clay_open_dialog_set_image (CLAY_OPEN_DIALOG (dialog),
                                  image, open_as_layers, open_as_link);

      gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                    GTK_WINDOW (gtk_widget_get_toplevel (parent)));

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static GtkWidget *
file_save_dialog_show (Clay        *clay,
                       ClayImage   *image,
                       GtkWidget   *parent,
                       const gchar *title,
                       gboolean     save_a_copy,
                       gboolean     close_after_saving,
                       ClayDisplay *display)
{
  GtkWidget *dialog;

#define SAVE_DIALOG_KEY "clay-file-save-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SAVE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                               clay_widget_get_monitor (parent),
                                               NULL /*ui_manager*/,
                                               parent,
                                               "clay-file-save-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (gtk_widget_get_toplevel (parent)));

          dialogs_attach_dialog (G_OBJECT (image), SAVE_DIALOG_KEY, dialog);
          g_signal_connect_object (image, "disconnect",
                                   G_CALLBACK (gtk_widget_destroy),
                                   dialog, G_CONNECT_SWAPPED);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (file_save_dialog_response),
                            image);
        }
    }

  if (dialog)
    {
      gtk_window_set_title (GTK_WINDOW (dialog), title);

      clay_save_dialog_set_image (CLAY_SAVE_DIALOG (dialog),
                                  image, save_a_copy,
                                  close_after_saving, CLAY_OBJECT (display));

      gtk_window_present (GTK_WINDOW (dialog));
    }

  return dialog;
}

static void
file_save_dialog_response (GtkWidget *dialog,
                           gint       response_id,
                           gpointer   data)
{
  if (response_id == FILE_SAVE_RESPONSE_OTHER_DIALOG)
    {
      ClayFileDialog *file_dialog = CLAY_FILE_DIALOG (dialog);
      GtkWindow      *parent;
      GtkWidget      *other;
      GFile          *file;
      gchar          *folder;
      gchar          *basename;

      parent   = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      file     = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      folder   = g_path_get_dirname (clay_file_get_utf8_name (file));
      basename = g_path_get_basename (clay_file_get_utf8_name (file));
      g_object_unref (file);

      other = file_export_dialog_show (CLAY_FILE_DIALOG (file_dialog)->image->clay,
                                       CLAY_FILE_DIALOG (file_dialog)->image,
                                       GTK_WIDGET (parent), NULL);

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), basename);

      g_free (folder);
      g_free (basename);
    }
}

static GtkWidget *
file_export_dialog_show (Clay        *clay,
                         ClayImage   *image,
                         GtkWidget   *parent,
                         ClayDisplay *display)
{
  GtkWidget *dialog;

#define EXPORT_DIALOG_KEY "clay-file-export-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY);

  if (! dialog)
    {
      dialog = clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                               clay_widget_get_monitor (parent),
                                               NULL /*ui_manager*/,
                                               parent,
                                               "clay-file-export-dialog",
                                               -1, FALSE);

      if (dialog)
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (gtk_widget_get_toplevel (parent)));

          dialogs_attach_dialog (G_OBJECT (image), EXPORT_DIALOG_KEY, dialog);
          g_signal_connect_object (image, "disconnect",
                                   G_CALLBACK (gtk_widget_destroy),
                                   dialog, G_CONNECT_SWAPPED);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (file_export_dialog_response),
                            image);
        }
    }

  if (dialog)
    {
      clay_export_dialog_set_image (CLAY_EXPORT_DIALOG (dialog), image,
                                    CLAY_OBJECT (display));

      gtk_window_present (GTK_WINDOW (dialog));
    }

  return dialog;
}

static void
file_export_dialog_response (GtkWidget *dialog,
                             gint       response_id,
                             gpointer   data)
{
  if (response_id == FILE_SAVE_RESPONSE_OTHER_DIALOG)
    {
      ClayFileDialog *file_dialog = CLAY_FILE_DIALOG (dialog);
      GtkWindow      *parent;
      GtkWidget      *other;
      GFile          *file;
      gchar          *folder;
      gchar          *basename;

      parent   = gtk_window_get_transient_for (GTK_WINDOW (dialog));
      file     = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      folder   = g_path_get_dirname (clay_file_get_utf8_name (file));
      basename = g_path_get_basename (clay_file_get_utf8_name (file));
      g_object_unref (file);

      other = file_save_dialog_show (CLAY_FILE_DIALOG (file_dialog)->image->clay,
                                     CLAY_FILE_DIALOG (file_dialog)->image,
                                     GTK_WIDGET (parent),
                                     _("Save Image"),
                                     FALSE, FALSE, NULL);

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (other), folder);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (other), basename);

      g_free (folder);
      g_free (basename);
    }
}

static void
file_new_template_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  ClayTemplate *template;
  ClayImage    *image;

  image = (ClayImage *) data;

  if (! (name && strlen (name)))
    name = _("(Unnamed Template)");

  template = clay_template_new (name);
  clay_template_set_from_image (template, image);
  clay_container_add (image->clay->templates, CLAY_OBJECT (template));
  g_object_unref (template);
}

static void
file_revert_confirm_response (GtkWidget   *dialog,
                              gint         response_id,
                              ClayDisplay *display)
{
  ClayImage *old_image = clay_display_get_image (display);

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      Clay              *clay = old_image->clay;
      ClayImage         *new_image;
      GFile             *file;
      ClayPDBStatusType  status;
      GError            *error = NULL;

      file = clay_image_get_file (old_image);

      if (! file)
        file = clay_image_get_imported_file (old_image);

      new_image = file_open_image (clay, clay_get_user_context (clay),
                                   CLAY_PROGRESS (display),
                                   file, 0, 0, TRUE, FALSE, NULL,
                                   CLAY_RUN_INTERACTIVE,
                                   NULL, &status, NULL, &error);

      if (new_image)
        {
          clay_displays_reconnect (clay, old_image, new_image);
          clay_image_flush (new_image);

          /*  the displays own the image now  */
          g_object_unref (new_image);
        }
      else if (status != CLAY_PDB_CANCEL)
        {
          clay_message (clay, G_OBJECT (display), CLAY_MESSAGE_ERROR,
                        _("Reverting to '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }
    }
}
