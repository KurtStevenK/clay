/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayDialogConfig class
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "config/clayguiconfig.h"


/* We don't want to include stuff from core/ here, instead do the next
 * less ugly hack...
 */
typedef struct _ClayFillOptions   ClayFillOptions;
typedef struct _ClayStrokeOptions ClayStrokeOptions;


#define CLAY_TYPE_DIALOG_CONFIG            (clay_dialog_config_get_type ())
#define CLAY_DIALOG_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DIALOG_CONFIG, ClayDialogConfig))
#define CLAY_DIALOG_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DIALOG_CONFIG, ClayDialogConfigClass))
#define CLAY_IS_DIALOG_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DIALOG_CONFIG))
#define CLAY_IS_DIALOG_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DIALOG_CONFIG))


typedef struct _ClayDialogConfigClass ClayDialogConfigClass;

struct _ClayDialogConfig
{
  ClayGuiConfig             parent_instance;

  ClayColorProfilePolicy     color_profile_policy;
  ClayMetadataRotationPolicy metadata_rotation_policy;

  gchar                    *color_profile_path;

  ClayColorRenderingIntent  image_convert_profile_intent;
  gboolean                  image_convert_profile_bpc;

  GeglDitherMethod          image_convert_precision_layer_dither_method;
  GeglDitherMethod          image_convert_precision_text_layer_dither_method;
  GeglDitherMethod          image_convert_precision_channel_dither_method;

  ClayConvertPaletteType    image_convert_indexed_palette_type;
  gint                      image_convert_indexed_max_colors;
  gboolean                  image_convert_indexed_remove_duplicates;
  ClayConvertDitherType     image_convert_indexed_dither_type;
  gboolean                  image_convert_indexed_dither_alpha;
  gboolean                  image_convert_indexed_dither_text_layers;

  ClayFillType              image_resize_fill_type;
  ClayItemSet               image_resize_layer_set;
  gboolean                  image_resize_resize_text_layers;

  gchar                    *layer_new_name;
  ClayLayerMode             layer_new_mode;
  ClayLayerColorSpace       layer_new_blend_space;
  ClayLayerColorSpace       layer_new_composite_space;
  ClayLayerCompositeMode    layer_new_composite_mode;
  gdouble                   layer_new_opacity;
  ClayFillType              layer_new_fill_type;

  ClayFillType              layer_resize_fill_type;

  ClayAddMaskType           layer_add_mask_type;
  gboolean                  layer_add_mask_invert;
  gboolean                  layer_add_mask_edit_mask;

  ClayMergeType             layer_merge_type;
  gboolean                  layer_merge_active_group_only;
  gboolean                  layer_merge_discard_invisible;

  gchar                    *channel_new_name;
  GeglColor                *channel_new_color;

  gchar                    *path_new_name;

  gchar                    *path_export_path;
  gboolean                  path_export_active_only;

  gchar                    *path_import_path;
  gboolean                  path_import_merge;
  gboolean                  path_import_scale;

  gdouble                   selection_feather_radius;
  gboolean                  selection_feather_edge_lock;

  gdouble                   selection_grow_radius;

  gdouble                   selection_shrink_radius;
  gboolean                  selection_shrink_edge_lock;

  gdouble                   selection_border_radius;
  gboolean                  selection_border_edge_lock;
  ClayChannelBorderStyle    selection_border_style;

  ClayFillOptions          *fill_options;
  ClayStrokeOptions        *stroke_options;
};

struct _ClayDialogConfigClass
{
  ClayGuiConfigClass  parent_class;
};


GType  clay_dialog_config_get_type (void);
