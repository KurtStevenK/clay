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
#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylayer.h"

#include "widgets/claycontainercombobox.h"
#include "widgets/claycontainerview.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayviewabledialog.h"
#include "widgets/claywidgets-utils.h"

#include "layer-add-mask-dialog.h"

#include "clay-intl.h"


typedef struct _LayerAddMaskDialog LayerAddMaskDialog;

struct _LayerAddMaskDialog
{
  GList               *layers;
  ClayAddMaskType      add_mask_type;
  ClayChannel         *channel;
  gboolean             invert;
  gboolean             edit_mask;
  ClayAddMaskCallback  callback;
  gpointer             user_data;
};


/*  local function prototypes  */

static void   layer_add_mask_dialog_free             (LayerAddMaskDialog *private);
static void   layer_add_mask_dialog_response         (GtkWidget          *dialog,
                                                      gint                response_id,
                                                      LayerAddMaskDialog *private);
static void   layer_add_mask_dialog_channel_selected (ClayContainerView  *view,
                                                      LayerAddMaskDialog *private);


/*  public functions  */

GtkWidget *
layer_add_mask_dialog_new (GList               *layers,
                           ClayContext         *context,
                           GtkWidget           *parent,
                           ClayAddMaskType      add_mask_type,
                           gboolean             invert,
                           gboolean             edit_mask,
                           ClayAddMaskCallback  callback,
                           gpointer             user_data)
{
  LayerAddMaskDialog *private;
  GtkWidget          *dialog;
  GtkWidget          *vbox;
  GtkWidget          *hbox;
  GtkWidget          *frame;
  GtkWidget          *combo;
  GtkWidget          *button;
  ClayImage          *image;
  ClayChannel        *channel;
  GList              *channels;
  gchar              *title;
  gchar              *desc;
  gint                n_layers = g_list_length (layers);

  g_return_val_if_fail (layers, NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  private = g_slice_new0 (LayerAddMaskDialog);

  private->layers        = layers;
  private->add_mask_type = add_mask_type;
  private->invert        = invert;
  private->edit_mask     = edit_mask;
  private->callback      = callback;
  private->user_data     = user_data;

  title = ngettext ("Add Layer Mask", "Add Layer Masks", n_layers);
  title = g_strdup_printf (title, n_layers);
  desc  = ngettext ("Add a Mask to the Layer", "Add Masks to %d Layers", n_layers);
  desc  = g_strdup_printf (desc, n_layers);

  dialog = clay_viewable_dialog_new (layers, context,
                                     title,
                                     "clay-layer-add-mask",
                                     CLAY_ICON_LAYER_MASK,
                                     desc,
                                     parent,
                                     clay_standard_help_func,
                                     CLAY_HELP_LAYER_MASK_ADD,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Add"),    GTK_RESPONSE_OK,

                                     NULL);

  g_free (title);
  g_free (desc);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) layer_add_mask_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (layer_add_mask_dialog_response),
                    private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  frame =
    clay_enum_radio_frame_new (CLAY_TYPE_ADD_MASK_TYPE,
                               gtk_label_new (_("Initialize Layer Mask to:")),
                               G_CALLBACK (clay_radio_button_update),
                               &private->add_mask_type, NULL,
                               &button);
  clay_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                   private->add_mask_type);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  image = clay_item_get_image (CLAY_ITEM (layers->data));

  combo = clay_container_combo_box_new (clay_image_get_channels (image),
                                        context,
                                        CLAY_VIEW_SIZE_SMALL, 1);
  clay_enum_radio_frame_add (GTK_FRAME (frame), combo,
                             CLAY_ADD_MASK_CHANNEL, TRUE);
  gtk_widget_set_visible (combo, TRUE);

  g_signal_connect (combo, "selection-changed",
                    G_CALLBACK (layer_add_mask_dialog_channel_selected),
                    private);

  channels = clay_image_get_selected_channels (image);
  if (channels)
    /* Mask dialog only requires one channel. Just take any of the
     * selected ones randomly.
     */
    channel = channels->data;
  else
    channel = CLAY_CHANNEL (clay_container_get_first_child (clay_image_get_channels (image)));

  clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (combo),
                                      CLAY_VIEWABLE (channel));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  button = gtk_check_button_new_with_mnemonic (_("In_vert mask"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), private->invert);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (clay_toggle_button_update),
                    &private->invert);

  button = gtk_check_button_new_with_mnemonic (_("_Edit mask immediately"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), private->edit_mask);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (clay_toggle_button_update),
                    &private->edit_mask);

  return dialog;
}


/*  private functions  */

static void
layer_add_mask_dialog_free (LayerAddMaskDialog *private)
{
  g_slice_free (LayerAddMaskDialog, private);
}

static void
layer_add_mask_dialog_response (GtkWidget          *dialog,
                                gint                response_id,
                                LayerAddMaskDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (private->layers->data));

      if (private->add_mask_type == CLAY_ADD_MASK_CHANNEL &&
          ! private->channel)
        {
          clay_message_literal (image->clay,
                                G_OBJECT (dialog), CLAY_MESSAGE_WARNING,
                                _("Please select a channel first"));
          return;
        }

      private->callback (dialog,
                         private->layers,
                         private->add_mask_type,
                         private->channel,
                         private->invert,
                         private->edit_mask,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
layer_add_mask_dialog_channel_selected (ClayContainerView  *view,
                                        LayerAddMaskDialog *private)
{
  private->channel = CLAY_CHANNEL (clay_container_view_get_1_selected (view));
}
