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

#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayitem.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claysizebox.h"
#include "widgets/clayviewabledialog.h"

#include "scale-dialog.h"

#include "clay-intl.h"


#define RESPONSE_RESET 1

typedef struct _ScaleDialog ScaleDialog;

struct _ScaleDialog
{
  ClayViewable          *viewable;
  ClayUnit              *unit;
  ClayInterpolationType  interpolation;
  GtkWidget             *box;
  GtkWidget             *combo;
  ClayScaleCallback      callback;
  gpointer               user_data;
};


/*  local function prototypes  */

static void   scale_dialog_free     (ScaleDialog *private);
static void   scale_dialog_response (GtkWidget   *dialog,
                                     gint         response_id,
                                     ScaleDialog *private);
static void   scale_dialog_reset    (ScaleDialog *private);


/*  public function  */

GtkWidget *
scale_dialog_new (ClayViewable          *viewable,
                  ClayContext           *context,
                  const gchar           *title,
                  const gchar           *role,
                  GtkWidget             *parent,
                  ClayHelpFunc           help_func,
                  const gchar           *help_id,
                  ClayUnit              *unit,
                  ClayInterpolationType  interpolation,
                  ClayScaleCallback      callback,
                  gpointer               user_data)
{
  GtkWidget      *dialog;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *frame;
  GtkWidget      *label;
  ClaySizeEntry  *entry;
  ScaleDialog    *private;
  ClayImage      *image = NULL;
  const gchar    *text  = NULL;
  gint            width, height;
  gdouble         xres, yres;

  g_return_val_if_fail (CLAY_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  if (CLAY_IS_IMAGE (viewable))
    {
      image = CLAY_IMAGE (viewable);

      width  = clay_image_get_width (image);
      height = clay_image_get_height (image);

      text = _("Image Size");
    }
  else if (CLAY_IS_ITEM (viewable))
    {
      ClayItem *item = CLAY_ITEM (viewable);

      image = clay_item_get_image (item);

      width  = clay_item_get_width  (item);
      height = clay_item_get_height (item);

      text = _("Layer Size");
    }
  else
    {
      g_return_val_if_reached (NULL);
    }

  private = g_slice_new0 (ScaleDialog);

  private->viewable      = viewable;
  private->interpolation = interpolation;
  private->unit          = unit;
  private->callback      = callback;
  private->user_data     = user_data;

  clay_image_get_resolution (image, &xres, &yres);

  dialog = clay_viewable_dialog_new (g_list_prepend (NULL, viewable), context,
                                     title, role, CLAY_ICON_OBJECT_SCALE, title,
                                     parent,
                                     help_func, help_id,

                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Scale"),  GTK_RESPONSE_OK,

                                     NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) scale_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (scale_dialog_response),
                    private);

  private->box = g_object_new (CLAY_TYPE_SIZE_BOX,
                               "width",           width,
                               "height",          height,
                               "unit",            unit,
                               "xresolution",     xres,
                               "yresolution",     yres,
                               "resolution-unit", clay_image_get_unit (image),
                               "keep-aspect",     TRUE,
                               "edit-resolution", CLAY_IS_IMAGE (viewable),
                               NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  frame = clay_frame_new (text);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  gtk_container_add (GTK_CONTAINER (frame), private->box);
  gtk_widget_set_visible (private->box, TRUE);

  frame = clay_frame_new (_("Quality"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  label = gtk_label_new_with_mnemonic (_("I_nterpolation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  gtk_size_group_add_widget (CLAY_SIZE_BOX (private->box)->size_group, label);

  private->combo = clay_enum_combo_box_new (CLAY_TYPE_INTERPOLATION_TYPE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), private->combo);
  gtk_box_pack_start (GTK_BOX (hbox), private->combo, TRUE, TRUE, 0);
  gtk_widget_set_visible (private->combo, TRUE);

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (private->combo),
                                 private->interpolation);

  /* Set focus to size entry for immediate editing */
  entry = clay_size_box_get_size_entry (CLAY_SIZE_BOX (private->box));
  clay_size_entry_set_activates_default (entry, TRUE);
  clay_size_entry_grab_focus (entry);

  return dialog;
}


/*  private functions  */

static void
scale_dialog_free (ScaleDialog *private)
{
  g_slice_free (ScaleDialog, private);
}

static void
scale_dialog_response (GtkWidget   *dialog,
                       gint         response_id,
                       ScaleDialog *private)
{
  ClayUnit *unit          = private->unit;
  gint      interpolation = private->interpolation;
  ClayUnit *resolution_unit;
  gint      width, height;
  gdouble   xres, yres;

  switch (response_id)
    {
    case RESPONSE_RESET:
      scale_dialog_reset (private);
      break;

    case GTK_RESPONSE_OK:
      g_object_get (private->box,
                    "width",           &width,
                    "height",          &height,
                    "unit",            &unit,
                    "xresolution",     &xres,
                    "yresolution",     &yres,
                    "resolution-unit", &resolution_unit,
                    NULL);

      clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (private->combo),
                                     &interpolation);

      private->callback (dialog,
                         private->viewable,
                         width, height, unit, interpolation,
                         xres, yres, resolution_unit,
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
scale_dialog_reset (ScaleDialog *private)
{
  ClayImage *image;
  gint       width, height;
  gdouble    xres, yres;

  if (CLAY_IS_IMAGE (private->viewable))
    {
      image = CLAY_IMAGE (private->viewable);

      width  = clay_image_get_width (image);
      height = clay_image_get_height (image);
    }
  else if (CLAY_IS_ITEM (private->viewable))
    {
      ClayItem *item = CLAY_ITEM (private->viewable);

      image = clay_item_get_image (item);

      width  = clay_item_get_width  (item);
      height = clay_item_get_height (item);
    }
  else
    {
      g_return_if_reached ();
    }

  clay_image_get_resolution (image, &xres, &yres);

  g_object_set (private->box,
                "keep-aspect",     FALSE,
                NULL);

  g_object_set (private->box,
                "width",           width,
                "height",          height,
                "unit",            private->unit,
                NULL);

  g_object_set (private->box,
                "keep-aspect",     TRUE,
                "xresolution",     xres,
                "yresolution",     yres,
                "resolution-unit", clay_image_get_unit (image),
                NULL);

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (private->combo),
                                 private->interpolation);
}
