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
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "core/claycontext.h"
#include "core/claydrawable-filters.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylink.h"
#include "core/claylinklayer.h"
#include "core/clayrasterizable.h"

#include "path/claypath.h"
#include "path/clayvectorlayer.h"

#include "text/claytext.h"
#include "text/claytextlayer.h"

#include "widgets/claycontainercombobox.h"
#include "widgets/claycontainerlistview.h"
#include "widgets/claycontainerview.h"
#include "widgets/claylayermodebox.h"
#include "widgets/clayopendialog.h"
#include "widgets/clayviewabledialog.h"

#include "item-options-dialog.h"
#include "layer-options-dialog.h"

#include "clay-intl.h"


typedef struct _LayerOptionsDialog LayerOptionsDialog;

struct _LayerOptionsDialog
{
  Clay                     *clay;
  ClayLayer                *layer;
  ClayLayerMode             mode;
  ClayLayerColorSpace       blend_space;
  ClayLayerColorSpace       composite_space;
  ClayLayerCompositeMode    composite_mode;
  gdouble                   opacity;
  ClayFillType              fill_type;
  gboolean                  lock_alpha;
  gboolean                  rename_text_layers;
  ClayLayerOptionsCallback  callback;
  gpointer                  user_data;

  GtkWidget                *mode_box;
  GtkWidget                *blend_space_combo;
  GtkWidget                *composite_space_combo;
  GtkWidget                *composite_mode_combo;
  GtkWidget                *size_se;
  GtkWidget                *offset_se;

  ClayLink                 *link;
  ClayPath                 *initial_path;
};


/*  local function prototypes  */

static void   layer_options_dialog_free           (LayerOptionsDialog *private);
static void   layer_options_dialog_callback       (GtkWidget          *dialog,
                                                   ClayImage          *image,
                                                   ClayItem           *item,
                                                   ClayContext        *context,
                                                   const gchar        *item_name,
                                                   gboolean            item_visible,
                                                   ClayColorTag        item_color_tag,
                                                   gboolean            item_lock_content,
                                                   gboolean            item_lock_position,
                                                   gboolean            item_lock_visibility,
                                                   gpointer            user_data);
static void
     layer_options_dialog_update_mode_sensitivity (LayerOptionsDialog *private);
static void   layer_options_dialog_mode_notify    (GtkWidget          *widget,
                                                   const GParamSpec   *pspec,
                                                   LayerOptionsDialog *private);
static void   layer_options_dialog_rename_toggled (GtkWidget          *widget,
                                                   LayerOptionsDialog *private);

static void   layer_options_file_set              (GtkFileChooserButton *widget,
                                                   LayerOptionsDialog   *private);
static void   layer_options_dialog_path_selected  (ClayContainerView    *view,
                                                   LayerOptionsDialog   *private);


/*  public functions  */

GtkWidget *
layer_options_dialog_new (ClayImage                *image,
                          ClayLayer                *layer,
                          ClayContext              *context,
                          GtkWidget                *parent,
                          const gchar              *title,
                          const gchar              *role,
                          const gchar              *icon_name,
                          const gchar              *desc,
                          const gchar              *help_id,
                          const gchar              *layer_name,
                          ClayLayerMode             layer_mode,
                          ClayLayerColorSpace       layer_blend_space,
                          ClayLayerColorSpace       layer_composite_space,
                          ClayLayerCompositeMode    layer_composite_mode,
                          gdouble                   layer_opacity,
                          ClayFillType              layer_fill_type,
                          gboolean                  layer_visible,
                          ClayColorTag              layer_color_tag,
                          gboolean                  layer_lock_content,
                          gboolean                  layer_lock_position,
                          gboolean                  layer_lock_visibility,
                          gboolean                  layer_lock_alpha,
                          ClayLayerOptionsCallback  callback,
                          gpointer                  user_data)
{
  LayerOptionsDialog   *private;
  GtkWidget            *dialog;
  GtkWidget            *grid;
  GtkListStore         *space_model;
  GtkWidget            *combo;
  GtkWidget            *file_select;
  GtkWidget            *scale;
  GtkWidget            *label;
  GtkAdjustment        *adjustment;
  GtkWidget            *spinbutton;
  GtkWidget            *button;
  ClayLayerModeContext  mode_context;
  gdouble               xres;
  gdouble               yres;
  gint                  row = 0;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (LayerOptionsDialog);

  private->clay               = image->clay;
  private->layer              = layer;
  private->mode               = layer_mode;
  private->blend_space        = layer_blend_space;
  private->composite_space    = layer_composite_space;
  private->composite_mode     = layer_composite_mode;
  private->opacity            = layer_opacity * 100.0;
  private->fill_type          = layer_fill_type;
  private->lock_alpha         = layer_lock_alpha;
  private->rename_text_layers = FALSE;
  private->callback           = callback;
  private->user_data          = user_data;

  private->link               = NULL;
  private->initial_path       = NULL;

  if (layer && clay_item_is_text_layer (CLAY_ITEM (layer)))
    private->rename_text_layers = clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (layer));

  dialog = item_options_dialog_new (image, CLAY_ITEM (layer), context,
                                    parent, title, role,
                                    icon_name, desc, help_id,
                                    _("Layer _name:"),
                                    CLAY_ICON_LOCK_CONTENT,
                                    _("Lock _pixels"),
                                    _("Lock position and _size"),
                                    _("Lock visibility"),
                                    layer_name,
                                    layer_visible,
                                    layer_color_tag,
                                    layer_lock_content,
                                    layer_lock_position,
                                    layer_lock_visibility,
                                    layer_options_dialog_callback,
                                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) layer_options_dialog_free, private);

  if (! layer || clay_viewable_get_children (CLAY_VIEWABLE (layer)) == NULL)
    mode_context = CLAY_LAYER_MODE_CONTEXT_LAYER;
  else
    mode_context = CLAY_LAYER_MODE_CONTEXT_GROUP;

  private->mode_box = clay_layer_mode_box_new (mode_context);
  item_options_dialog_add_widget (dialog, _("_Mode:"), private->mode_box);
  clay_layer_mode_box_set_mode (CLAY_LAYER_MODE_BOX (private->mode_box),
                                private->mode);

  g_signal_connect (private->mode_box, "notify::layer-mode",
                    G_CALLBACK (layer_options_dialog_mode_notify),
                    private);

  space_model =
    clay_enum_store_new_with_values (CLAY_TYPE_LAYER_COLOR_SPACE,
                                     4,
                                     CLAY_LAYER_COLOR_SPACE_AUTO,
                                     CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
                                     CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
                                     CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL);

  private->blend_space_combo = combo =
    clay_enum_combo_box_new_with_model (CLAY_ENUM_STORE (space_model));
  item_options_dialog_add_widget (dialog, _("_Blend space:"), combo);
  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (combo),
                                       "clay-layer-color-space");
  clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                              private->blend_space,
                              G_CALLBACK (clay_int_combo_box_get_active),
                              &private->blend_space, NULL);

  private->composite_space_combo = combo =
    clay_enum_combo_box_new_with_model (CLAY_ENUM_STORE (space_model));
  item_options_dialog_add_widget (dialog, _("Compos_ite space:"), combo);
  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (combo),
                                       "clay-layer-color-space");
  clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                              private->composite_space,
                              G_CALLBACK (clay_int_combo_box_get_active),
                              &private->composite_space, NULL);

  g_object_unref (space_model);

  private->composite_mode_combo = combo =
    clay_enum_combo_box_new (CLAY_TYPE_LAYER_COMPOSITE_MODE);
  item_options_dialog_add_widget (dialog, _("Composite mo_de:"), combo);
  clay_enum_combo_box_set_icon_prefix (CLAY_ENUM_COMBO_BOX (combo),
                                       "clay-layer-composite");
  clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                              private->composite_mode,
                              G_CALLBACK (clay_int_combo_box_get_active),
                              &private->composite_mode, NULL);

  /*  set the sensitivity of above 3 menus  */
  layer_options_dialog_update_mode_sensitivity (private);

  adjustment = gtk_adjustment_new (private->opacity, 0.0, 100.0,
                                   1.0, 10.0, 0.0);
  scale = clay_spin_scale_new (adjustment, NULL, 1);
  item_options_dialog_add_widget (dialog, _("_Opacity:"), scale);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (clay_double_adjustment_update),
                    &private->opacity);

  grid = item_options_dialog_get_grid (dialog, &row);

  clay_image_get_resolution (image, &xres, &yres);

  if (! layer)
    {
      /*  The size labels  */
      label = gtk_label_new (_("Width:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
      gtk_widget_set_visible (label, TRUE);

      label = gtk_label_new (_("Height:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
      gtk_widget_set_visible (label, TRUE);

      /*  The size sizeentry  */
      adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
      spinbutton = clay_spin_button_new (adjustment, 1.0, 2);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

      private->size_se = clay_size_entry_new (1, clay_unit_pixel (), "%a",
                                              TRUE, TRUE, FALSE, 10,
                                              CLAY_SIZE_ENTRY_UPDATE_SIZE);

      clay_size_entry_add_field (CLAY_SIZE_ENTRY (private->size_se),
                                 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_grid_attach (GTK_GRID (private->size_se), spinbutton, 1, 0, 1, 1);
      gtk_widget_set_visible (spinbutton, TRUE);

      gtk_grid_attach (GTK_GRID (grid), private->size_se, 1, row, 1, 2);
      gtk_widget_set_visible (private->size_se, TRUE);

      clay_size_entry_set_unit (CLAY_SIZE_ENTRY (private->size_se),
                                clay_unit_pixel ());

      clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (private->size_se), 0,
                                      xres, FALSE);
      clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (private->size_se), 1,
                                      yres, FALSE);

      clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (private->size_se), 0,
                                             CLAY_MIN_IMAGE_SIZE,
                                             CLAY_MAX_IMAGE_SIZE);
      clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (private->size_se), 1,
                                             CLAY_MIN_IMAGE_SIZE,
                                             CLAY_MAX_IMAGE_SIZE);

      clay_size_entry_set_size (CLAY_SIZE_ENTRY (private->size_se), 0,
                                0, clay_image_get_width  (image));
      clay_size_entry_set_size (CLAY_SIZE_ENTRY (private->size_se), 1,
                                0, clay_image_get_height (image));

      clay_size_entry_set_refval (CLAY_SIZE_ENTRY (private->size_se), 0,
                                  clay_image_get_width  (image));
      clay_size_entry_set_refval (CLAY_SIZE_ENTRY (private->size_se), 1,
                                  clay_image_get_height (image));

      row += 2;
    }

  if (! layer || ! clay_item_is_vector_layer (CLAY_ITEM (layer)))
    {
      /*  The offset labels  */
      label = gtk_label_new (_("Offset X:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
      gtk_widget_set_visible (label, TRUE);

      label = gtk_label_new (_("Offset Y:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
      gtk_widget_set_visible (label, TRUE);

      /*  The offset sizeentry  */
      adjustment = gtk_adjustment_new (0, 1, 1, 1, 10, 0);
      spinbutton = clay_spin_button_new (adjustment, 1.0, 2);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

      private->offset_se = clay_size_entry_new (1, clay_unit_pixel (), "%a",
                                                TRUE, TRUE, FALSE, 10,
                                                CLAY_SIZE_ENTRY_UPDATE_SIZE);

      clay_size_entry_add_field (CLAY_SIZE_ENTRY (private->offset_se),
                                 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_grid_attach (GTK_GRID (private->offset_se), spinbutton, 1, 0, 1, 1);
      gtk_widget_set_visible (spinbutton, TRUE);

      gtk_grid_attach (GTK_GRID (grid), private->offset_se, 1, row, 1, 2);
      gtk_widget_set_visible (private->offset_se, TRUE);

      clay_size_entry_set_unit (CLAY_SIZE_ENTRY (private->offset_se),
                                clay_unit_pixel ());

      clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (private->offset_se), 0,
                                      xres, FALSE);
      clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (private->offset_se), 1,
                                      yres, FALSE);

      clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (private->offset_se), 0,
                                             -CLAY_MAX_IMAGE_SIZE,
                                             CLAY_MAX_IMAGE_SIZE);
      clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (private->offset_se), 1,
                                             -CLAY_MAX_IMAGE_SIZE,
                                             CLAY_MAX_IMAGE_SIZE);

      clay_size_entry_set_size (CLAY_SIZE_ENTRY (private->offset_se), 0,
                                0, clay_image_get_width  (image));
      clay_size_entry_set_size (CLAY_SIZE_ENTRY (private->offset_se), 1,
                                0, clay_image_get_height (image));

      if (layer)
        {
          clay_size_entry_set_refval (CLAY_SIZE_ENTRY (private->offset_se), 0,
                                      clay_item_get_offset_x (CLAY_ITEM (layer)));
          clay_size_entry_set_refval (CLAY_SIZE_ENTRY (private->offset_se), 1,
                                      clay_item_get_offset_y (CLAY_ITEM (layer)));
        }
      else
        {
          clay_size_entry_set_refval (CLAY_SIZE_ENTRY (private->offset_se), 0, 0);
          clay_size_entry_set_refval (CLAY_SIZE_ENTRY (private->offset_se), 1, 0);
        }

      row += 2;
    }

  if (layer)
    {
      GtkWidget     *left_vbox = item_options_dialog_get_vbox (dialog);
      GtkWidget     *frame;
      ClayContainer *filters;
      GtkWidget     *view;

      frame = clay_frame_new (_("Active Filters"));
      gtk_box_pack_start (GTK_BOX (left_vbox), frame, TRUE, TRUE, 0);
      gtk_widget_set_visible (frame, TRUE);

      filters = clay_drawable_get_filters (CLAY_DRAWABLE (layer));

      view = clay_container_list_view_new (filters, context,
                                           CLAY_VIEW_SIZE_SMALL, 0);
      gtk_container_add (GTK_CONTAINER (frame), view);
      gtk_widget_set_visible (view, TRUE);

      if (clay_item_is_link_layer (CLAY_ITEM (layer)))
        {
          GtkWidget *open_dialog;
          ClayLink  *link;

          /* File chooser dialog. */
          open_dialog = clay_open_dialog_new (private->clay);
          gtk_window_set_title (GTK_WINDOW (open_dialog),
                                _("Select Linked Image"));

          /* File chooser button. */
          file_select = gtk_file_chooser_button_new_with_dialog (open_dialog);
          link = clay_link_layer_get_link (CLAY_LINK_LAYER (layer));
          gtk_file_chooser_set_file (GTK_FILE_CHOOSER (file_select),
                                     clay_link_get_file (link, NULL, NULL),
                                     NULL);
          gtk_widget_set_visible (file_select, TRUE);
          clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    _("_Linked image:"), 0.0, 0.5,
                                    file_select, 1);

          g_signal_connect (file_select, "file-set",
                            G_CALLBACK (layer_options_file_set),
                            private);

          private->link = clay_link_duplicate (link);

          /* Absolute path checkbox. */
          button = gtk_check_button_new_with_mnemonic (_("S_tore with absolute path"));
          clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    NULL, 0.0, 0.5,
                                    button, 2);
          g_object_bind_property (G_OBJECT (private->link), "absolute-path",
                                  G_OBJECT (button),        "active",
                                  G_BINDING_SYNC_CREATE |
                                  G_BINDING_BIDIRECTIONAL);
          gtk_widget_set_visible (button, TRUE);
        }
      else if (clay_item_is_vector_layer (CLAY_ITEM (layer)))
        {
          combo = clay_container_combo_box_new (clay_image_get_paths (image),
                                                context, CLAY_VIEW_SIZE_SMALL, 1);
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (combo),
                                              CLAY_VIEWABLE (clay_vector_layer_get_path (CLAY_VECTOR_LAYER (layer))));
          g_signal_connect (combo, "selection-changed",
                            G_CALLBACK (layer_options_dialog_path_selected),
                            private);
          clay_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    _("_Associated path:"), 0.0, 0.5,
                                    combo, 2);
          gtk_widget_set_visible (combo, TRUE);
        }
    }
  else
    {
      /*  The fill type  */
      combo = clay_enum_combo_box_new (CLAY_TYPE_FILL_TYPE);
      clay_grid_attach_aligned (GTK_GRID (grid), 0, row,
                                _("_Fill with:"), 0.0, 0.5,
                                combo, 1);
      clay_int_combo_box_connect (CLAY_INT_COMBO_BOX (combo),
                                  private->fill_type,
                                  G_CALLBACK (clay_int_combo_box_get_active),
                                  &private->fill_type, NULL);
    }

  button = item_options_dialog_get_lock_position (dialog);

  if (private->size_se)
    g_object_bind_property (G_OBJECT (button),           "active",
                            G_OBJECT (private->size_se), "sensitive",
                            G_BINDING_SYNC_CREATE |
                            G_BINDING_INVERT_BOOLEAN);

  if (private->offset_se)
    g_object_bind_property (G_OBJECT (button),             "active",
                            G_OBJECT (private->offset_se), "sensitive",
                            G_BINDING_SYNC_CREATE |
                            G_BINDING_INVERT_BOOLEAN);

  button = item_options_dialog_add_switch (dialog,
                                           CLAY_ICON_LOCK_ALPHA,
                                           _("Lock _alpha"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->lock_alpha);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (clay_toggle_button_update),
                    &private->lock_alpha);

  /*  For text layers add a toggle to control "auto-rename"  */
  if (layer && clay_item_is_text_layer (CLAY_ITEM (layer)))
    {
      button = item_options_dialog_add_switch (dialog,
                                               CLAY_ICON_TOOL_TEXT,
                                               _("Set name from _text"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    private->rename_text_layers);
      g_signal_connect (button, "toggled",
                        G_CALLBACK (clay_toggle_button_update),
                        &private->rename_text_layers);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (layer_options_dialog_rename_toggled),
                        private);
    }

  return dialog;
}


/*  private functions  */

static void
layer_options_dialog_free (LayerOptionsDialog *private)
{
  /* If not cleared already, it means we cancel.
   * Let's revert silently.
   */
  if (private->initial_path)
    {
      ClayLayer *layer = private->layer;

      g_return_if_fail (CLAY_IS_VECTOR_LAYER (layer));

      clay_vector_layer_set_path (CLAY_VECTOR_LAYER (layer), private->initial_path, FALSE);
      g_clear_object (&private->initial_path);
    }
  g_clear_object (&private->link);

  g_slice_free (LayerOptionsDialog, private);
}

static void
layer_options_dialog_callback (GtkWidget    *dialog,
                               ClayImage    *image,
                               ClayItem     *item,
                               ClayContext  *context,
                               const gchar  *item_name,
                               gboolean      item_visible,
                               ClayColorTag  item_color_tag,
                               gboolean      item_lock_content,
                               gboolean      item_lock_position,
                               gboolean      item_lock_visibility,
                               gpointer      user_data)
{
  LayerOptionsDialog *private  = user_data;
  ClayPath           *path     = NULL;
  gint                width    = 0;
  gint                height   = 0;
  gint                offset_x = 0;
  gint                offset_y = 0;

  if (private->size_se)
    {
      width =
        RINT (clay_size_entry_get_refval (CLAY_SIZE_ENTRY (private->size_se),
                                          0));
      height =
        RINT (clay_size_entry_get_refval (CLAY_SIZE_ENTRY (private->size_se),
                                          1));
    }

  if (private->offset_se)
    {
      offset_x =
        RINT (clay_size_entry_get_refval (CLAY_SIZE_ENTRY (private->offset_se),
                                          0));
      offset_y =
        RINT (clay_size_entry_get_refval (CLAY_SIZE_ENTRY (private->offset_se),
                                          1));
    }

  if (clay_item_is_vector_layer (item) && private->initial_path)
    {
      path = clay_vector_layer_get_path (CLAY_VECTOR_LAYER (item));
      clay_vector_layer_set_path (CLAY_VECTOR_LAYER (item), private->initial_path, FALSE);
      g_clear_object (&private->initial_path);
    }

  private->callback (dialog,
                     image,
                     CLAY_LAYER (item),
                     context,
                     item_name,
                     private->mode,
                     private->blend_space,
                     private->composite_space,
                     private->composite_mode,
                     private->opacity / 100.0,
                     private->fill_type,
                     private->link,
                     path,
                     width,
                     height,
                     offset_x,
                     offset_y,
                     item_visible,
                     item_color_tag,
                     item_lock_content,
                     item_lock_position,
                     item_lock_visibility,
                     private->lock_alpha,
                     private->rename_text_layers,
                     private->user_data);
}

static void
layer_options_dialog_update_mode_sensitivity (LayerOptionsDialog *private)
{
  gboolean mutable;

  mutable = clay_layer_mode_is_blend_space_mutable (private->mode);
  gtk_widget_set_sensitive (private->blend_space_combo, mutable);

  mutable = clay_layer_mode_is_composite_space_mutable (private->mode);
  gtk_widget_set_sensitive (private->composite_space_combo, mutable);

  mutable = clay_layer_mode_is_composite_mode_mutable (private->mode);
  gtk_widget_set_sensitive (private->composite_mode_combo, mutable);
}

static void
layer_options_dialog_mode_notify (GtkWidget          *widget,
                                  const GParamSpec   *pspec,
                                  LayerOptionsDialog *private)
{
  private->mode = clay_layer_mode_box_get_mode (CLAY_LAYER_MODE_BOX (widget));

  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (private->blend_space_combo),
                                 CLAY_LAYER_COLOR_SPACE_AUTO);
  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (private->composite_space_combo),
                                 CLAY_LAYER_COLOR_SPACE_AUTO);
  clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (private->composite_mode_combo),
                                 CLAY_LAYER_COMPOSITE_AUTO);

  layer_options_dialog_update_mode_sensitivity (private);
}

static void
layer_options_dialog_rename_toggled (GtkWidget          *widget,
                                     LayerOptionsDialog *private)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      clay_item_is_text_layer (CLAY_ITEM (private->layer)))
    {
      ClayTextLayer *text_layer = CLAY_TEXT_LAYER (private->layer);
      ClayText      *text       = clay_text_layer_get_text (text_layer);

      if (text && text->text)
        {
          GtkWidget *dialog;
          GtkWidget *name_entry;
          gchar     *name = clay_utf8_strtrim (text->text, 30);

          dialog = gtk_widget_get_toplevel (widget);

          name_entry = item_options_dialog_get_name_entry (dialog);

          gtk_entry_set_text (GTK_ENTRY (name_entry), name);

          g_free (name);
        }
    }
}

static void
layer_options_file_set (GtkFileChooserButton *widget,
                        LayerOptionsDialog   *private)
{
  GFile *file;

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (widget));
  if (file)
    {
      gint width  = 0;
      gint height = 0;

      if (private->layer)
        {
          width  = clay_item_get_width (CLAY_ITEM (private->layer));
          height = clay_item_get_height (CLAY_ITEM (private->layer));

          if (width == 0 || height == 0)
            {
              ClayImage *image = clay_item_get_image (CLAY_ITEM (private->layer));

              width  = clay_image_get_width (image);
              height = clay_image_get_height (image);
            }
        }

      clay_link_set_file (private->link, file, width, height, TRUE, NULL, NULL);
      if (clay_link_is_broken (private->link))
        {
          clay_link_set_file (private->link, NULL, width, height, TRUE, NULL, NULL);
          g_signal_handlers_block_by_func (widget,
                                           G_CALLBACK (layer_options_file_set),
                                           private);
          gtk_file_chooser_unselect_file (GTK_FILE_CHOOSER (widget), file);
          g_signal_handlers_unblock_by_func (widget,
                                             G_CALLBACK (layer_options_file_set),
                                             private);
        }
    }
  g_clear_object (&file);
}

static void
layer_options_dialog_path_selected (ClayContainerView  *view,
                                    LayerOptionsDialog *private)
{
  ClayViewable    *item     = clay_container_view_get_1_selected (view);
  ClayPath        *new_path = NULL;
  ClayPath        *path;
  ClayVectorLayer *layer;

  g_return_if_fail (CLAY_IS_VECTOR_LAYER (private->layer));

  layer = CLAY_VECTOR_LAYER (private->layer);

  if (item)
    new_path = CLAY_PATH (item);

  path = clay_vector_layer_get_path (CLAY_VECTOR_LAYER (private->layer));

  if (new_path && new_path != path)
    {
      g_return_if_fail (CLAY_IS_PATH (new_path));
      clay_vector_layer_set_path (layer, new_path, FALSE);

      if (private->initial_path == new_path)
        g_clear_object (&private->initial_path);
      else if (private->initial_path == NULL)
        private->initial_path = g_object_ref (path);

      clay_vector_layer_refresh (layer);
    }
}
