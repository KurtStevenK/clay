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

#pragma once


typedef void (* ClayLayerOptionsCallback) (GtkWidget              *dialog,
                                           ClayImage              *image,
                                           ClayLayer              *layer,
                                           ClayContext            *context,
                                           const gchar            *layer_name,
                                           ClayLayerMode           layer_mode,
                                           ClayLayerColorSpace     layer_blend_space,
                                           ClayLayerColorSpace     layer_composite_space,
                                           ClayLayerCompositeMode  layer_composite_mode,
                                           gdouble                 layer_opacity,
                                           ClayFillType            layer_fill_type,
                                           ClayLink               *link,
                                           ClayPath               *path,
                                           gint                    layer_width,
                                           gint                    layer_height,
                                           gint                    layer_offset_x,
                                           gint                    layer_offset_y,
                                           gboolean                layer_visible,
                                           ClayColorTag            layer_color_tag,
                                           gboolean                layer_lock_content,
                                           gboolean                layer_lock_position,
                                           gboolean                layer_lock_visibility,
                                           gboolean                layer_lock_alpha,
                                           gboolean                rename_text_layer,
                                           gpointer                user_data);


GtkWidget * layer_options_dialog_new (ClayImage                *image,
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
                                      gpointer                  user_data);
