/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-dialog.h
 * Copyright (C) 2015 Michael Natterer <mitch@gimp.org>
 *
 * Partly based on the lcms plug-in
 * Copyright (C) 2006, 2007  Sven Neumann <sven@gimp.org>
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayviewabledialog.h"
#include "widgets/claywidgets-constructors.h"
#include "widgets/claywidgets-utils.h"

#include "color-profile-dialog.h"

#include "clay-intl.h"


typedef struct
{
  ColorProfileDialogType    dialog_type;
  ClayImage                *image;
  ClayColorProfile         *current_profile;
  ClayColorProfile         *default_profile;
  ClayColorRenderingIntent  intent;
  gboolean                  bpc;
  ClayColorProfileCallback  callback;
  gpointer                  user_data;

  ClayColorConfig          *config;
  GtkWidget                *dialog;
  GtkWidget                *main_vbox;
  GtkWidget                *combo;
  GtkWidget                *dest_view;

} ProfileDialog;


static void        color_profile_dialog_free     (ProfileDialog *private);
static GtkWidget * color_profile_combo_box_new   (ProfileDialog *private);
static void        color_profile_dialog_response (GtkWidget     *dialog,
                                                  gint           response_id,
                                                  ProfileDialog *private);
static void        color_profile_dest_changed    (GtkWidget     *combo,
                                                  ProfileDialog *private);


/*  public functions  */

GtkWidget *
color_profile_dialog_new (ColorProfileDialogType    dialog_type,
                          ClayImage                *image,
                          ClayContext              *context,
                          GtkWidget                *parent,
                          ClayColorProfile         *current_profile,
                          ClayColorProfile         *default_profile,
                          ClayColorRenderingIntent  intent,
                          gboolean                  bpc,
                          ClayColorProfileCallback  callback,
                          gpointer                  user_data)
{
  ProfileDialog *private;
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *expander;
  GtkWidget     *label;
  const gchar   *dest_label;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (current_profile == NULL ||
                        CLAY_IS_COLOR_PROFILE (current_profile), NULL);
  g_return_val_if_fail (default_profile == NULL ||
                        CLAY_IS_COLOR_PROFILE (default_profile), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (ProfileDialog);

  private->dialog_type     = dialog_type;
  private->image           = image;
  private->current_profile = current_profile;
  private->default_profile = default_profile;
  private->intent          = intent;
  private->bpc             = bpc;
  private->callback        = callback;
  private->user_data       = user_data;
  private->config          = image->clay->config->color_management;

  switch (dialog_type)
    {
    case COLOR_PROFILE_DIALOG_ASSIGN_PROFILE:
      dialog =
        clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Assign ICC Color Profile"),
                                  "clay-image-color-profile-assign",
                                  NULL,
                                  _("Assign a color profile to the image"),
                                  parent,
                                  clay_standard_help_func,
                                  CLAY_HELP_IMAGE_COLOR_PROFILE_ASSIGN,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_Assign"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Assign");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE:
      dialog =
        clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Convert to ICC Color Profile"),
                                  "clay-image-color-profile-convert",
                                  NULL,
                                  _("Convert the image to a color profile"),
                                  parent,
                                  clay_standard_help_func,
                                  CLAY_HELP_IMAGE_COLOR_PROFILE_CONVERT,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_RGB:
      dialog =
        clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("RGB Conversion"),
                                  "clay-image-convert-rgb",
                                  CLAY_ICON_CONVERT_RGB,
                                  _("Convert Image to RGB"),
                                  parent,
                                  clay_standard_help_func,
                                  CLAY_HELP_IMAGE_CONVERT_RGB,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY:
      dialog =
        clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Grayscale Conversion"),
                                  "clay-image-convert-gray",
                                  CLAY_ICON_CONVERT_GRAYSCALE,
                                  _("Convert Image to Grayscale"),
                                  parent,
                                  clay_standard_help_func,
                                  CLAY_HELP_IMAGE_CONVERT_GRAYSCALE,

                                  _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                  _("C_onvert"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("Convert to");
      break;

    case COLOR_PROFILE_DIALOG_SELECT_SOFTPROOF_PROFILE:
      dialog =
        clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                  _("Soft-Proof Profile"),
                                  "clay-select-softproof-profile",
                                  CLAY_ICON_DOCUMENT_PRINT,
                                  _("Select Soft-Proof Profile"),
                                  parent,
                                  clay_standard_help_func,
                                  CLAY_HELP_VIEW_COLOR_MANAGEMENT,

                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                  _("_Select"), GTK_RESPONSE_OK,

                                  NULL);
      dest_label = _("New Color Profile");
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  private->dialog = dialog;

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) color_profile_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (color_profile_dialog_response),
                    private);

  private->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (private->main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      private->main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (private->main_vbox, TRUE);

  frame = clay_frame_new (_("Current Color Profile"));
  gtk_box_pack_start (GTK_BOX (private->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  label = clay_color_profile_label_new (private->current_profile);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_set_visible (label, TRUE);

  frame = clay_frame_new (dest_label);
  gtk_box_pack_start (GTK_BOX (private->main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  private->combo = color_profile_combo_box_new (private);
  gtk_box_pack_start (GTK_BOX (vbox), private->combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (private->combo, TRUE);

  expander = gtk_expander_new_with_mnemonic (_("Profile _details"));
  gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);
  gtk_widget_set_visible (expander, TRUE);

  private->dest_view = clay_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (expander), private->dest_view);
  gtk_widget_set_visible (private->dest_view, TRUE);

  g_signal_connect (private->combo, "changed",
                    G_CALLBACK (color_profile_dest_changed),
                    private);

  color_profile_dest_changed (private->combo, private);

  if (dialog_type == COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *combo;
      GtkWidget *toggle;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (private->main_vbox), vbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (vbox, TRUE);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      label = gtk_label_new_with_mnemonic (_("_Rendering Intent:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      combo = clay_enum_combo_box_new (CLAY_TYPE_COLOR_RENDERING_INTENT);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_set_visible (combo, TRUE);

      clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                                  private->intent,
                                  G_CALLBACK (clay_int_combo_box_get_active),
                                  &private->intent, NULL);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), private->bpc);
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_set_visible (toggle, TRUE);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (clay_toggle_button_update),
                        &private->bpc);
    }

  return dialog;
}


/*  private functions  */

static void
color_profile_dialog_free (ProfileDialog *private)
{
  g_slice_free (ProfileDialog, private);
}

static GtkWidget *
color_profile_combo_box_new (ProfileDialog *private)
{
  GtkListStore *store;
  GtkWidget    *combo;
  GtkWidget    *chooser;
  GFile        *history;

  history = clay_directory_file ("profilerc", NULL);
  store = clay_color_profile_store_new (history);
  g_object_unref (history);

  if (private->default_profile)
    {
      ClayImageBaseType  base_type;
      ClayPrecision      precision;
      GError            *error = NULL;

      switch (private->dialog_type)
        {
        case COLOR_PROFILE_DIALOG_ASSIGN_PROFILE:
        case COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE:
          base_type = clay_image_get_base_type (private->image);
          break;

        case COLOR_PROFILE_DIALOG_CONVERT_TO_RGB:
          base_type = CLAY_RGB;
          break;

        case COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY:
          base_type = CLAY_GRAY;
          break;

        default:
          g_return_val_if_reached (NULL);
        }

      precision = clay_image_get_precision (private->image);

      if (! clay_color_profile_store_add_defaults (CLAY_COLOR_PROFILE_STORE (store),
                                                   private->config,
                                                   base_type,
                                                   precision,
                                                   &error))
        {
          clay_message (private->image->clay, G_OBJECT (private->dialog),
                        CLAY_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      clay_color_profile_store_add_file (CLAY_COLOR_PROFILE_STORE (store),
                                         NULL, NULL);
    }

  chooser =
    clay_color_profile_chooser_dialog_new (_("Select Destination Profile"),
                                           NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN);

  clay_color_profile_chooser_dialog_connect_path (chooser,
                                                  G_OBJECT (private->image->clay->config),
                                                  "color-profile-path");

  combo = clay_color_profile_combo_box_new_with_model (chooser,
                                                       GTK_TREE_MODEL (store));
  g_object_unref (store);

  clay_color_profile_combo_box_set_active_file (CLAY_COLOR_PROFILE_COMBO_BOX (combo),
                                                NULL, NULL);

  return combo;
}

static void
color_profile_dialog_response (GtkWidget     *dialog,
                               gint           response_id,
                               ProfileDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      ClayColorProfile *profile = NULL;
      GFile            *file;

      file = clay_color_profile_combo_box_get_active_file (CLAY_COLOR_PROFILE_COMBO_BOX (private->combo));

      if (file)
        {
          GError *error = NULL;

          profile = clay_color_profile_new_from_file (file, &error);
          g_object_unref (file);

          if (! profile)
            {
              clay_message (private->image->clay, G_OBJECT (dialog),
                            CLAY_MESSAGE_ERROR,
                            "%s", error->message);
              g_clear_error (&error);

              return;
            }
        }
      else if (private->default_profile)
        {
          profile = g_object_ref (private->default_profile);
        }

      private->callback (dialog,
                         private->image,
                         profile,
                         file,
                         private->intent,
                         private->bpc,
                         private->user_data);

      if (profile)
        g_object_unref (profile);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
color_profile_dest_changed (GtkWidget     *combo,
                            ProfileDialog *private)
{
  ClayColorProfile *dest_profile = NULL;
  GFile            *file;

  file = clay_color_profile_combo_box_get_active_file (CLAY_COLOR_PROFILE_COMBO_BOX (combo));

  if (file)
    {
      GError *error = NULL;

      dest_profile = clay_color_profile_new_from_file (file, &error);
      g_object_unref (file);

      if (! dest_profile)
        {
          clay_color_profile_view_set_error (CLAY_COLOR_PROFILE_VIEW (private->dest_view),
                                             error->message);
          g_clear_error (&error);
        }
    }
  else if (private->default_profile)
    {
      dest_profile = g_object_ref (private->default_profile);
    }
  else
    {
      clay_color_profile_view_set_error (CLAY_COLOR_PROFILE_VIEW (private->dest_view),
                                         C_("profile", "None"));
    }

  if (dest_profile)
    {
      clay_color_profile_view_set_profile (CLAY_COLOR_PROFILE_VIEW (private->dest_view),
                                           dest_profile);
      g_object_unref (dest_profile);
    }
}
