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


ClayMetadata * clay_image_get_metadata                    (ClayImage    *image);
void           clay_image_set_metadata                    (ClayImage    *image,
                                                           ClayMetadata *metadata,
                                                           gboolean      push_undo);

void           clay_image_metadata_update_pixel_size      (ClayImage    *image);
void           clay_image_metadata_update_bits_per_sample (ClayImage    *image);
void           clay_image_metadata_update_resolution      (ClayImage    *image);
void           clay_image_metadata_update_colorspace      (ClayImage    *image);

ClayImage    * clay_image_metadata_load_thumbnail         (Clay         *clay,
                                                           GFile        *file,
                                                           gint         *full_image_width,
                                                           gint         *full_image_height,
                                                           const Babl  **format,
                                                           GError      **error);
