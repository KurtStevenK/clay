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

#include "actions-types.h"

#include "core/clay.h"
#include "core/claygradient-save.h"
#include "core/claycontext.h"

#include "widgets/claycontainereditor.h"
#include "widgets/claycontainerview.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "dialogs/dialogs.h"

#include "gradients-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   gradients_save_as_pov_ray_response (GtkNativeDialog *dialog,
                                                  gint             response_id,
                                                  ClayGradient    *gradient);


/*  public functions  */

void
gradients_save_as_pov_ray_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayContainerEditor  *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext          *context;
  ClayGradient         *gradient;
  GtkFileChooserNative *dialog;

  context  = clay_container_view_get_context (editor->view);
  gradient = clay_context_get_gradient (context);

  if (! gradient)
    return;

#define SAVE_AS_POV_DIALOG_KEY "clay-save-as-pov-ray-dialog"

  dialog = GTK_FILE_CHOOSER_NATIVE (dialogs_get_native_dialog (G_OBJECT (gradient),
                                    SAVE_AS_POV_DIALOG_KEY));

  if (! dialog)
    {
      gchar *title = g_strdup_printf (_("Save '%s' as POV-Ray"),
                                      clay_object_get_name (gradient));

      dialog = gtk_file_chooser_native_new (title, NULL,
                                            GTK_FILE_CHOOSER_ACTION_SAVE,
                                            _("_Save"), _("_Cancel"));

      g_free (title);

      g_object_set_data (G_OBJECT (dialog), "clay", context->clay);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gradients_save_as_pov_ray_response),
                        gradient);

      dialogs_attach_native_dialog (G_OBJECT (gradient),
                                    SAVE_AS_POV_DIALOG_KEY,
                                    GTK_NATIVE_DIALOG (dialog));
    }

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (dialog));
}


/*  private functions  */

static void
gradients_save_as_pov_ray_response (GtkNativeDialog *dialog,
                                    gint             response_id,
                                    ClayGradient    *gradient)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      if (! clay_gradient_save_pov (gradient, file, &error))
        {
          clay_message_literal (g_object_get_data (G_OBJECT (dialog), "clay"),
                                G_OBJECT (dialog), CLAY_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  dialogs_detach_native_dialog (G_OBJECT (gradient), dialog);
  gtk_native_dialog_destroy (dialog);
}
