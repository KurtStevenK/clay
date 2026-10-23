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

#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clay-utils.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayviewabledialog.h"

#include "print-size-dialog.h"

#include "clay-intl.h"


#define RESPONSE_RESET 1
#define SB_WIDTH       8


typedef struct _PrintSizeDialog PrintSizeDialog;

struct _PrintSizeDialog
{
  ClayImage              *image;
  ClaySizeEntry          *size_entry;
  ClaySizeEntry          *resolution_entry;
  ClayChainButton        *chain;
  gdouble                 xres;
  gdouble                 yres;
  ClayResolutionCallback  callback;
  gpointer                user_data;
};


/*  local function prototypes  */

static void   print_size_dialog_free               (PrintSizeDialog *private);
static void   print_size_dialog_response           (GtkWidget       *dialog,
                                                    gint             response_id,
                                                    PrintSizeDialog *private);
static void   print_size_dialog_reset              (PrintSizeDialog *private);

static void   print_size_dialog_size_changed       (GtkWidget       *widget,
                                                    PrintSizeDialog *private);
static void   print_size_dialog_resolution_changed (GtkWidget       *widget,
                                                    PrintSizeDialog *private);
static void   print_size_dialog_set_size           (PrintSizeDialog *private,
                                                    gdouble          width,
                                                    gdouble          height);
static void   print_size_dialog_set_resolution     (PrintSizeDialog *private,
                                                    gdouble          xres,
                                                    gdouble          yres);


/*  public functions  */

GtkWidget *
print_size_dialog_new (ClayImage              *image,
                       ClayContext            *context,
                       const gchar            *title,
                       const gchar            *role,
                       GtkWidget              *parent,
                       ClayHelpFunc            help_func,
                       const gchar            *help_id,
                       ClayResolutionCallback  callback,
                       gpointer                user_data)
{
  PrintSizeDialog *private;
  GtkWidget       *dialog;
  GtkWidget       *frame;
  GtkWidget       *grid;
  GtkWidget       *entry;
  GtkWidget       *label;
  GtkWidget       *width;
  GtkWidget       *height;
  GtkWidget       *hbox;
  GtkWidget       *chain;
  GtkAdjustment   *adj;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (PrintSizeDialog);

  private->image     = image;
  private->callback  = callback;
  private->user_data = user_data;

  clay_image_get_resolution (image, &private->xres, &private->yres);

  dialog = clay_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                     title, role,
                                     CLAY_ICON_DOCUMENT_PRINT_RESOLUTION, title,
                                     parent,
                                     help_func, help_id,

                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_OK"),     GTK_RESPONSE_OK,

                                     NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) print_size_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (print_size_dialog_response),
                    private);

  frame = clay_frame_new (_("Print Size"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  /*  the print size entry  */

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  width = clay_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  height = clay_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (height), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  entry = clay_size_entry_new (0, clay_get_default_unit (), "%n",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               CLAY_SIZE_ENTRY_UPDATE_SIZE);
  private->size_entry = CLAY_SIZE_ENTRY (entry);

  label = gtk_label_new_with_mnemonic (_("_Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new_with_mnemonic (_("H_eight:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 0, 1, 2);
  gtk_widget_set_visible (hbox, TRUE);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (entry, TRUE);

  clay_size_entry_add_field (CLAY_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_grid_attach (GTK_GRID (entry), height, 0, 1, 1, 1);
  gtk_widget_set_visible (height, TRUE);

  clay_size_entry_add_field (CLAY_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_grid_attach (GTK_GRID (entry), width, 0, 0, 1, 1);
  gtk_widget_set_visible (width, TRUE);

  clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (entry), 0,
                                  private->xres, FALSE);
  clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (entry), 1,
                                  private->yres, FALSE);

  clay_size_entry_set_refval_boundaries
    (CLAY_SIZE_ENTRY (entry), 0, CLAY_MIN_IMAGE_SIZE, CLAY_MAX_IMAGE_SIZE);
  clay_size_entry_set_refval_boundaries
    (CLAY_SIZE_ENTRY (entry), 1, CLAY_MIN_IMAGE_SIZE, CLAY_MAX_IMAGE_SIZE);

  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (entry), 0,
                              clay_image_get_width  (image));
  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (entry), 1,
                              clay_image_get_height (image));

  /*  the resolution entry  */

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  width = clay_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  height = clay_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (height), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  label = gtk_label_new_with_mnemonic (_("_X resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new_with_mnemonic (_("_Y resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 2, 1, 2);
  gtk_widget_set_visible (hbox, TRUE);

  entry = clay_size_entry_new (0, clay_image_get_unit (image), _("pixels/%a"),
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               CLAY_SIZE_ENTRY_UPDATE_RESOLUTION);
  private->resolution_entry = CLAY_SIZE_ENTRY (entry);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (entry, TRUE);

  clay_size_entry_add_field (CLAY_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_grid_attach (GTK_GRID (entry), height, 0, 1, 1, 1);
  gtk_widget_set_visible (height, TRUE);

  clay_size_entry_add_field (CLAY_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_grid_attach (GTK_GRID (entry), width, 0, 0, 1, 1);
  gtk_widget_set_visible (width, TRUE);

  clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (entry), 0,
                                         CLAY_MIN_RESOLUTION,
                                         CLAY_MAX_RESOLUTION);
  clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (entry), 1,
                                         CLAY_MIN_RESOLUTION,
                                         CLAY_MAX_RESOLUTION);

  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (entry), 0, private->xres);
  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (entry), 1, private->yres);

  chain = clay_chain_button_new (CLAY_CHAIN_RIGHT);
  if (ABS (private->xres - private->yres) < CLAY_MIN_RESOLUTION)
    clay_chain_button_set_active (CLAY_CHAIN_BUTTON (chain), TRUE);
  gtk_grid_attach (GTK_GRID (entry), chain, 1, 0, 1, 2);
  gtk_widget_set_visible (chain, TRUE);

  private->chain = CLAY_CHAIN_BUTTON (chain);

  /* Set focus to size entry for immediate editing */
  clay_size_entry_set_activates_default (private->size_entry, TRUE);
  clay_size_entry_grab_focus (private->size_entry);

  g_signal_connect (private->size_entry, "value-changed",
                    G_CALLBACK (print_size_dialog_size_changed),
                    private);
  g_signal_connect (private->resolution_entry, "value-changed",
                    G_CALLBACK (print_size_dialog_resolution_changed),
                    private);

  return dialog;
}


/*  private functions  */

static void
print_size_dialog_free (PrintSizeDialog *private)
{
  g_slice_free (PrintSizeDialog, private);
}

static void
print_size_dialog_response (GtkWidget       *dialog,
                            gint             response_id,
                            PrintSizeDialog *private)
{
  ClaySizeEntry *entry = private->resolution_entry;

  switch (response_id)
    {
    case RESPONSE_RESET:
      print_size_dialog_reset (private);
      break;

    case GTK_RESPONSE_OK:
      private->callback (dialog,
                         private->image,
                         clay_size_entry_get_refval (entry, 0),
                         clay_size_entry_get_refval (entry, 1),
                         clay_size_entry_get_unit (entry),
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
print_size_dialog_reset (PrintSizeDialog *private)
{
  gdouble  xres, yres;

  clay_size_entry_set_unit (private->resolution_entry,
                            clay_get_default_unit ());

  clay_image_get_resolution (private->image, &xres, &yres);
  print_size_dialog_set_resolution (private, xres, yres);
}

static void
print_size_dialog_size_changed (GtkWidget       *widget,
                                PrintSizeDialog *private)
{
  ClayImage *image = private->image;
  gdouble    width;
  gdouble    height;
  gdouble    xres;
  gdouble    yres;
  gdouble    scale;

  scale = clay_unit_get_factor (clay_size_entry_get_unit (private->size_entry));

  width  = clay_size_entry_get_value (private->size_entry, 0);
  height = clay_size_entry_get_value (private->size_entry, 1);

  xres = scale * clay_image_get_width  (image) / MAX (0.001, width);
  yres = scale * clay_image_get_height (image) / MAX (0.001, height);

  xres = CLAMP (xres, CLAY_MIN_RESOLUTION, CLAY_MAX_RESOLUTION);
  yres = CLAMP (yres, CLAY_MIN_RESOLUTION, CLAY_MAX_RESOLUTION);

  print_size_dialog_set_resolution (private, xres, yres);
  print_size_dialog_set_size (private,
                              clay_image_get_width  (image),
                              clay_image_get_height (image));
}

static void
print_size_dialog_resolution_changed (GtkWidget       *widget,
                                      PrintSizeDialog *private)
{
  ClaySizeEntry *entry = private->resolution_entry;
  gdouble        xres  = clay_size_entry_get_refval (entry, 0);
  gdouble        yres  = clay_size_entry_get_refval (entry, 1);

  print_size_dialog_set_resolution (private, xres, yres);
}

static void
print_size_dialog_set_size (PrintSizeDialog *private,
                            gdouble          width,
                            gdouble          height)
{
  g_signal_handlers_block_by_func (private->size_entry,
                                   print_size_dialog_size_changed,
                                   private);

  clay_size_entry_set_refval (private->size_entry, 0, width);
  clay_size_entry_set_refval (private->size_entry, 1, height);

  g_signal_handlers_unblock_by_func (private->size_entry,
                                     print_size_dialog_size_changed,
                                     private);
}

static void
print_size_dialog_set_resolution (PrintSizeDialog *private,
                                  gdouble          xres,
                                  gdouble          yres)
{
  if (private->chain && clay_chain_button_get_active (private->chain))
    {
      if (xres != private->xres)
        yres = xres;
      else
        xres = yres;
    }

  private->xres = xres;
  private->yres = yres;

  g_signal_handlers_block_by_func (private->resolution_entry,
                                   print_size_dialog_resolution_changed,
                                   private);

  clay_size_entry_set_refval (private->resolution_entry, 0, xres);
  clay_size_entry_set_refval (private->resolution_entry, 1, yres);

  g_signal_handlers_unblock_by_func (private->resolution_entry,
                                     print_size_dialog_resolution_changed,
                                     private);

  g_signal_handlers_block_by_func (private->size_entry,
                                   print_size_dialog_size_changed,
                                   private);

  clay_size_entry_set_resolution (private->size_entry, 0, xres, TRUE);
  clay_size_entry_set_resolution (private->size_entry, 1, yres, TRUE);

  g_signal_handlers_unblock_by_func (private->size_entry,
                                     print_size_dialog_size_changed,
                                     private);
}
