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

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "gegl/clay-babl.h"

#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clay-utils.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayviewabledialog.h"
#include "widgets/claywidgets-utils.h"

#include "convert-precision-dialog.h"

#include "clay-intl.h"


typedef struct _ConvertDialog ConvertDialog;

struct _ConvertDialog
{
  ClayImage                    *image;
  ClayComponentType             component_type;
  ClayTRCType                   trc;
  GeglDitherMethod              layer_dither_method;
  GeglDitherMethod              text_layer_dither_method;
  GeglDitherMethod              channel_dither_method;
  ClayConvertPrecisionCallback  callback;
  gpointer                      user_data;
};


/*  local function prototypes  */

static void   convert_precision_dialog_free     (ConvertDialog    *private);
static void   convert_precision_dialog_response (GtkWidget        *widget,
                                                 gint              response_id,
                                                 ConvertDialog    *private);


/*  public functions  */

GtkWidget *
convert_precision_dialog_new (ClayImage                    *image,
                              ClayContext                  *context,
                              GtkWidget                    *parent,
                              ClayComponentType             component_type,
                              GeglDitherMethod              layer_dither_method,
                              GeglDitherMethod              text_layer_dither_method,
                              GeglDitherMethod              channel_dither_method,
                              ClayConvertPrecisionCallback  callback,
                              gpointer                      user_data)

{
  ConvertDialog *private;
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *perceptual_radio;
  const gchar   *enum_desc;
  gchar         *blurb;
  const Babl    *old_format;
  const Babl    *new_format;
  gint           old_bits;
  gint           new_bits;
  gboolean       dither;
  ClayTRCType    trc;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  /* random formats with the right precision */
  old_format = clay_image_get_layer_format (image, FALSE);
  new_format = clay_babl_format (CLAY_RGB,
                                 clay_babl_precision (component_type, FALSE),
                                 FALSE,
                                 babl_format_get_space (old_format));

  old_bits = (babl_format_get_bytes_per_pixel (old_format) * 8 /
              babl_format_get_n_components (old_format));
  new_bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

  /*  don't dither if we are converting to a higher bit depth,
   *  or to more than MAX_DITHER_BITS.
   */
  dither = (new_bits <  old_bits &&
            new_bits <= CONVERT_PRECISION_DIALOG_MAX_DITHER_BITS);

  trc = clay_babl_format_get_trc (old_format);
  trc = clay_suggest_trc_for_component_type (component_type, trc);

  private = g_slice_new0 (ConvertDialog);

  private->image                    = image;
  private->component_type           = component_type;
  private->trc                      = trc;
  private->layer_dither_method      = layer_dither_method;
  private->text_layer_dither_method = text_layer_dither_method;
  private->channel_dither_method    = channel_dither_method;
  private->callback                 = callback;
  private->user_data                = user_data;

  clay_enum_get_value (CLAY_TYPE_COMPONENT_TYPE, component_type,
                       NULL, NULL, &enum_desc, NULL);

  blurb = g_strdup_printf (_("Convert Image to %s"), enum_desc);

  dialog = clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                     _("Encoding Conversion"),
                                     "clay-image-convert-precision",
                                     CLAY_ICON_CONVERT_PRECISION,
                                     blurb,
                                     parent,
                                     clay_standard_help_func,
                                     CLAY_HELP_IMAGE_CONVERT_PRECISION,

                                     _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                     _("C_onvert"), GTK_RESPONSE_OK,

                                     NULL);

  g_free (blurb);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) convert_precision_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (convert_precision_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_vbox, TRUE);


  /*  gamma  */

  frame = clay_frame_new (_("Gamma"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = clay_int_radio_group_new (FALSE, NULL,
                                   G_CALLBACK (clay_radio_button_update),
                                   &private->trc, NULL,
                                   trc,

                                   _("Linear light"),
                                   CLAY_TRC_LINEAR, NULL,

                                   _("Non-Linear"),
                                   CLAY_TRC_NON_LINEAR, NULL,

                                   _("Perceptual (sRGB)"),
                                   CLAY_TRC_PERCEPTUAL, &perceptual_radio,

                                   NULL);

  if (private->trc != CLAY_TRC_PERCEPTUAL)
    gtk_widget_set_visible (perceptual_radio, FALSE);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);


  /*  dithering  */

  if (dither)
    {
      GtkWidget    *hbox;
      GtkWidget    *label;
      GtkWidget    *combo;
      GtkSizeGroup *size_group;

      frame = clay_frame_new (_("Dithering"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_set_visible (frame, TRUE);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_set_visible (vbox, TRUE);

      size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      /*  layers  */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      label = gtk_label_new_with_mnemonic (_("_Layers:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_size_group_add_widget (size_group, label);
      gtk_widget_set_visible (label, TRUE);

      combo = clay_enum_combo_box_new (GEGL_TYPE_DITHER_METHOD);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_set_visible (combo, TRUE);

      clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                                  private->layer_dither_method,
                                  G_CALLBACK (clay_int_combo_box_get_active),
                                  &private->layer_dither_method, NULL);

      /*  text layers  */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      label = gtk_label_new_with_mnemonic (_("_Text Layers:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_size_group_add_widget (size_group, label);
      gtk_widget_set_visible (label, TRUE);

      combo = clay_enum_combo_box_new (GEGL_TYPE_DITHER_METHOD);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_set_visible (combo, TRUE);

      clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                                  private->text_layer_dither_method,
                                  G_CALLBACK (clay_int_combo_box_get_active),
                                  &private->text_layer_dither_method, NULL);

      clay_help_set_help_data (combo,
                               _("Dithering text layers will make them "
                                 "uneditable"),
                               NULL);

      /*  channels  */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      label = gtk_label_new_with_mnemonic (_("_Channels and Masks:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_size_group_add_widget (size_group, label);
      gtk_widget_set_visible (label, TRUE);

      combo = clay_enum_combo_box_new (GEGL_TYPE_DITHER_METHOD);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_set_visible (combo, TRUE);

      clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                                  private->channel_dither_method,
                                  G_CALLBACK (clay_int_combo_box_get_active),
                                  &private->channel_dither_method, NULL);

      g_object_unref (size_group);
    }

  return dialog;
}


/*  private functions  */

static void
convert_precision_dialog_free (ConvertDialog *private)
{
  g_slice_free (ConvertDialog, private);
}

static void
convert_precision_dialog_response (GtkWidget     *dialog,
                                   gint           response_id,
                                   ConvertDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      ClayPrecision precision = clay_babl_precision (private->component_type,
                                                     private->trc);

      private->callback (dialog,
                         private->image,
                         precision,
                         private->layer_dither_method,
                         private->text_layer_dither_method,
                         private->channel_dither_method,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}
