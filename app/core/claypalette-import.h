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


ClayPalette * clay_palette_import_from_gradient      (ClayGradient *gradient,
                                                      ClayContext  *context,
                                                      gboolean      reverse,
                                                      ClayGradientBlendColorSpace  blend_color_space,
                                                      const gchar  *palette_name,
                                                      gint          n_colors);
ClayPalette * clay_palette_import_from_image         (ClayImage    *image,
                                                      ClayContext  *context,
                                                      const gchar  *palette_name,
                                                      gint          n_colors,
                                                      gint          threshold,
                                                      gboolean      selection_only);
ClayPalette * clay_palette_import_from_indexed_image (ClayImage    *image,
                                                      ClayContext  *context,
                                                      const gchar  *palette_name);
ClayPalette * clay_palette_import_from_drawables     (GList        *drawables,
                                                      ClayContext  *context,
                                                      const gchar  *palette_name,
                                                      gint          n_colors,
                                                      gint          threshold,
                                                      gboolean      selection_only);
ClayPalette * clay_palette_import_from_file          (ClayContext  *context,
                                                      GFile        *file,
                                                      const gchar  *palette_name,
                                                      GError      **error);
