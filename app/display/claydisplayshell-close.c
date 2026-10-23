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

#include <time.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/claydisplayconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "menus/menus.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-close.h"
#include "clayimagewindow.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void      clay_display_shell_close_dialog       (ClayDisplayShell *shell,
                                                        ClayImage        *image);
static void      clay_display_shell_close_name_changed (ClayImage        *image,
                                                        ClayMessageBox   *box);
static void      clay_display_shell_close_exported     (ClayImage        *image,
                                                        GFile            *file,
                                                        ClayMessageBox   *box);
static gboolean  clay_display_shell_close_time_changed (ClayMessageBox   *box);
static void      clay_display_shell_close_response     (GtkWidget        *widget,
                                                        gboolean          close,
                                                        ClayDisplayShell *shell);
static void      clay_display_shell_close_accel_marshal(GClosure         *closure,
                                                        GValue           *return_value,
                                                        guint             n_param_values,
                                                        const GValue     *param_values,
                                                        gpointer          invocation_hint,
                                                        gpointer          marshal_data);
static void      clay_time_since                       (gint64            then,
                                                        gint             *hours,
                                                        gint             *minutes);


/*  public functions  */

void
clay_display_shell_close (ClayDisplayShell *shell,
                          gboolean          kill_it)
{
  ClayImage *image;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  image = clay_display_get_image (shell->display);

  /*  FIXME: clay_busy HACK not really appropriate here because we only
   *  want to prevent the busy image and display to be closed.  --Mitch
   */
  if (shell->display->clay->busy)
    return;

  /*  If the image has been modified, give the user a chance to save
   *  it before nuking it--this only applies if its the last view
   *  to an image canvas.  (a image with disp_count = 1)
   */
  if (! kill_it                                 &&
      image                                     &&
      clay_image_get_display_count (image) == 1 &&
      clay_image_is_dirty (image))
    {
      /*  If there's a save dialog active for this image, then raise it.
       *  (see bug #511965)
       */
      GtkWidget *dialog = g_object_get_data (G_OBJECT (image),
                                             "clay-file-save-dialog");
      if (dialog)
        {
          gtk_window_present (GTK_WINDOW (dialog));
        }
      else
        {
          clay_display_shell_close_dialog (shell, image);
        }
    }
  else if (image)
    {
      clay_display_close (shell->display);
    }
  else
    {
      ClayImageWindow *window = clay_display_shell_get_window (shell);

      if (window)
        {
          ClayUIManager *manager = menus_get_image_manager_singleton (shell->display->clay);

          /* Activate the action instead of simply calling clay_exit(), so
           * the quit action's sensitivity is taken into account.
           */
          clay_ui_manager_activate_action (manager, "file", "file-quit");
        }
    }
}


/*  private functions  */

#define RESPONSE_SAVE  1


static void
clay_display_shell_close_dialog (ClayDisplayShell *shell,
                                 ClayImage        *image)
{
  GtkWidget       *dialog;
  ClayMessageBox  *box;
  GtkWidget       *label;
  GtkAccelGroup   *accel_group;
  GClosure        *closure;
  GSource         *source;
  guint            accel_key;
  GdkModifierType  accel_mods;
  gchar           *title;
  gchar           *accel_string;
  gchar           *hint;
  gchar           *markup;
  GFile           *file;

  if (shell->close_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->close_dialog));
      return;
    }

  file = clay_image_get_file (image);

  title = g_strdup_printf (_("Close %s"), clay_image_get_display_name (image));

  shell->close_dialog =
    dialog = clay_message_dialog_new (title, CLAY_ICON_DOCUMENT_SAVE,
                                      GTK_WIDGET (shell),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      clay_standard_help_func, NULL,

                                      file ?
                                      _("_Save") :
                                      _("Save _As"),         RESPONSE_SAVE,
                                      _("_Cancel"),          GTK_RESPONSE_CANCEL,
                                      _("_Discard Changes"), GTK_RESPONSE_CLOSE,
                                      NULL);

  g_free (title);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_SAVE,
                                           GTK_RESPONSE_CLOSE,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &shell->close_dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (clay_display_shell_close_response),
                    shell);

  /* connect <Primary>D to the quit/close button */
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (shell->close_dialog), accel_group);
  g_object_unref (accel_group);

  closure = g_closure_new_object (sizeof (GClosure),
                                  G_OBJECT (shell->close_dialog));
  g_closure_set_marshal (closure, clay_display_shell_close_accel_marshal);
  gtk_accelerator_parse ("<Primary>D", &accel_key, &accel_mods);
  gtk_accel_group_connect (accel_group, accel_key, accel_mods, 0, closure);

  box = CLAY_MESSAGE_DIALOG (dialog)->box;

  accel_string = gtk_accelerator_get_label (accel_key, accel_mods);
  hint = g_strdup_printf (_("Press %s to discard all changes and close the image."),
                          accel_string);
  markup = g_strdup_printf ("<i><small>%s</small></i>", hint);

  label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  g_free (markup);
  g_free (hint);
  g_free (accel_string);

  g_signal_connect_object (image, "name-changed",
                           G_CALLBACK (clay_display_shell_close_name_changed),
                           box, 0);
  g_signal_connect_object (image, "exported",
                           G_CALLBACK (clay_display_shell_close_exported),
                           box, 0);

  clay_display_shell_close_name_changed (image, box);

  closure =
    g_cclosure_new_object (G_CALLBACK (clay_display_shell_close_time_changed),
                           G_OBJECT (box));

  /*  update every 10 seconds  */
  source = g_timeout_source_new_seconds (10);
  g_source_set_closure (source, closure);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /*  The dialog is destroyed with the shell, so it should be safe
   *  to hold an image pointer for the lifetime of the dialog.
   */
  g_object_set_data (G_OBJECT (box), "clay-image", image);

  clay_display_shell_close_time_changed (box);

  gtk_widget_set_visible (dialog, TRUE);
}

static void
clay_display_shell_close_name_changed (ClayImage      *image,
                                       ClayMessageBox *box)
{
  GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET (box));

  if (GTK_IS_WINDOW (window))
    {
      gchar *title = g_strdup_printf (_("Close %s"),
                                      clay_image_get_display_name (image));

      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
    }

  clay_message_box_set_primary_text (box,
                                     _("Save the changes to image '%s' "
                                       "before closing?"),
                                     clay_image_get_display_name (image));
}

static void
clay_display_shell_close_exported (ClayImage      *image,
                                   GFile          *file,
                                   ClayMessageBox *box)
{
  clay_display_shell_close_time_changed (box);
}

static gboolean
clay_display_shell_close_time_changed (ClayMessageBox *box)
{
  ClayImage   *image       = g_object_get_data (G_OBJECT (box), "clay-image");
  gint64       dirty_time  = clay_image_get_dirty_time (image);
  gchar       *time_text   = NULL;
  gchar       *export_text = NULL;

  if (dirty_time)
    {
      gint hours   = 0;
      gint minutes = 0;

      clay_time_since (dirty_time, &hours, &minutes);

      if (hours > 0)
        {
          if (hours > 1 || minutes == 0)
            {
              time_text =
                g_strdup_printf (ngettext ("If you don't save the image, "
                                           "changes from the last hour "
                                           "will be lost.",
                                           "If you don't save the image, "
                                           "changes from the last %d "
                                           "hours will be lost.",
                                           hours), hours);
            }
          else
            {
              time_text =
                g_strdup_printf (ngettext ("If you don't save the image, "
                                           "changes from the last hour "
                                           "and %d minute will be lost.",
                                           "If you don't save the image, "
                                           "changes from the last hour "
                                           "and %d minutes will be lost.",
                                           minutes), minutes);
            }
        }
      else
        {
          time_text =
            g_strdup_printf (ngettext ("If you don't save the image, "
                                       "changes from the last minute "
                                       "will be lost.",
                                       "If you don't save the image, "
                                       "changes from the last %d "
                                       "minutes will be lost.",
                                       minutes), minutes);
        }
    }

  if (! clay_image_is_export_dirty (image))
    {
      GFile *file;

      file = clay_image_get_exported_file (image);
      if (! file)
        file = clay_image_get_imported_file (image);

      export_text = g_strdup_printf (_("The image has been exported to '%s'."),
                                     clay_file_get_utf8_name (file));
    }

  if (time_text && export_text)
    clay_message_box_set_text (box, "%s\n\n%s", time_text, export_text);
  else if (time_text || export_text)
    clay_message_box_set_text (box, "%s", time_text ? time_text : export_text);
  else
    clay_message_box_set_text (box, "%s", time_text);

  g_free (time_text);
  g_free (export_text);

  return TRUE;
}

static void
clay_display_shell_close_response (GtkWidget        *widget,
                                   gint              response_id,
                                   ClayDisplayShell *shell)
{
  gtk_widget_destroy (widget);

  switch (response_id)
    {
    case GTK_RESPONSE_CLOSE:
      clay_display_close (shell->display);
      break;

    case RESPONSE_SAVE:
      {
        ClayImageWindow *window = clay_display_shell_get_window (shell);

        if (window)
          {
            ClayUIManager *manager = menus_get_image_manager_singleton (shell->display->clay);

            clay_image_window_set_active_shell (window, shell);

            clay_ui_manager_activate_action (manager,
                                             "file", "file-save-and-close");
          }
      }
      break;

    default:
      break;
    }
}

static void
clay_display_shell_close_accel_marshal (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data)
{
  gtk_dialog_response (GTK_DIALOG (closure->data), GTK_RESPONSE_CLOSE);

  /* we handled the accelerator */
  g_value_set_boolean (return_value, TRUE);
}

static void
clay_time_since (gint64 then,
                 gint  *hours,
                 gint  *minutes)
{
  gint64 now  = time (NULL);
  gint64 diff = 1 + now - then;

  g_return_if_fail (now >= then);

  /*  first round up to the nearest minute  */
  diff = (diff + 59) / 60;

  /*  then optionally round minutes to multiples of 5 or 10  */
  if (diff > 50)
    diff = ((diff + 8) / 10) * 10;
  else if (diff > 20)
    diff = ((diff + 3) / 5) * 5;

  /*  determine full hours  */
  if (diff >= 60)
    {
      *hours = diff / 60;
      diff = (diff % 60);
    }

  /*  round up to full hours for 2 and more  */
  if (*hours > 1 && diff > 0)
    {
      *hours += 1;
      diff = 0;
    }

  *minutes = diff;
}
