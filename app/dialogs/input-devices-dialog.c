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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "core/clay.h"

#include "widgets/claydeviceeditor.h"
#include "widgets/claydevicemanager.h"
#include "widgets/claydevices.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"

#include "input-devices-dialog.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   input_devices_dialog_response (GtkWidget *dialog,
                                             guint      response_id,
                                             Clay      *clay);


/*  public functions  */

GtkWidget *
input_devices_dialog_new (Clay *clay)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *editor;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  dialog = clay_dialog_new (_("Configure Input Devices"),
                            "clay-input-devices-dialog",
                            NULL, 0,
                            clay_standard_help_func,
                            CLAY_HELP_INPUT_DEVICES,

                            _("_Reset"),  GTK_RESPONSE_REJECT,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_REJECT,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (input_devices_dialog_response),
                    clay);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  editor = clay_device_editor_new (clay);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (content_area), editor, TRUE, TRUE, 0);
  gtk_widget_set_visible (editor, TRUE);

  return dialog;
}


/*  private functions  */

static void
input_devices_dialog_response (GtkWidget *dialog,
                               guint      response_id,
                               Clay      *clay)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      clay_devices_save (clay, TRUE);
      break;

    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
      clay_devices_restore (clay);
      break;

    case GTK_RESPONSE_REJECT:
      {
        GtkWidget *confirm;

        confirm = clay_message_dialog_new (_("Reset Input Device Configuration"),
                                           CLAY_ICON_DIALOG_QUESTION,
                                           dialog,
                                           GTK_DIALOG_MODAL |
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           clay_standard_help_func, NULL,

                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                           _("_Reset"),  GTK_RESPONSE_OK,

                                           NULL);

        clay_dialog_set_alternative_button_order (GTK_DIALOG (confirm),
                                                  GTK_RESPONSE_OK,
                                                  GTK_RESPONSE_CANCEL,
                                                  -1);

        clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (confirm)->box,
                                           _("Do you really want to reset all "
                                             "input devices to default configuration?"));

        if (clay_dialog_run (CLAY_DIALOG (confirm)) == GTK_RESPONSE_OK)
          {
            clay_device_manager_reset (clay_devices_get_manager (clay));
            clay_devices_save (clay, TRUE);
            clay_devices_restore (clay);
          }
        gtk_widget_destroy (confirm);
      }
      return;

    default:
      break;
    }

  gtk_widget_destroy (dialog);
}
