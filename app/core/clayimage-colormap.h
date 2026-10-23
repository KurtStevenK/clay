/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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


#define CLAY_IMAGE_COLORMAP_SIZE 768


void           clay_image_colormap_init            (ClayImage     *image);
void           clay_image_colormap_dispose         (ClayImage     *image);
void           clay_image_colormap_free            (ClayImage     *image);

void           clay_image_colormap_update_formats  (ClayImage     *image);

const Babl   * clay_image_colormap_get_rgb_format  (ClayImage     *image);
const Babl   * clay_image_colormap_get_rgba_format (ClayImage     *image);

ClayPalette  * clay_image_get_colormap_palette     (ClayImage     *image);
void           clay_image_set_colormap_palette     (ClayImage     *image,
                                                    ClayPalette   *palette,
                                                    gboolean       push_undo);

/* Only internally */
guchar *       _clay_image_get_colormap            (ClayImage     *image,
                                                    gint          *n_colors);
void           _clay_image_set_colormap             (ClayImage     *image,
                                                    const guchar  *colormap,
                                                    gint           n_colors,
                                                    gboolean       push_undo);
void           clay_image_unset_colormap           (ClayImage     *image,
                                                    gboolean       push_undo);

gboolean       clay_image_colormap_is_index_used   (ClayImage     *image,
                                                    gint           color_index);

GeglColor    * clay_image_get_colormap_entry       (ClayImage     *image,
                                                    gint           color_index);
void           clay_image_set_colormap_entry       (ClayImage     *image,
                                                    gint           color_index,
                                                    GeglColor     *color,
                                                    gboolean       push_undo);

void           clay_image_add_colormap_entry       (ClayImage     *image,
                                                    GeglColor     *color);
gboolean       clay_image_delete_colormap_entry    (ClayImage     *image,
                                                    gint           color_index,
                                                    gboolean       push_undo);
