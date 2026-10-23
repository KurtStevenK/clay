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


void   clay_image_resize              (ClayImage    *image,
                                       ClayContext  *context,
                                       gint          new_width,
                                       gint          new_height,
                                       gint          offset_x,
                                       gint          offset_y,
                                       ClayProgress *progress);

void   clay_image_resize_with_layers  (ClayImage    *image,
                                       ClayContext  *context,
                                       ClayFillType  fill_type,
                                       gint          new_width,
                                       gint          new_height,
                                       gint          offset_x,
                                       gint          offset_y,
                                       ClayItemSet   layer_set,
                                       gboolean      resize_text_layers,
                                       ClayProgress *progress);

void   clay_image_resize_to_layers    (ClayImage    *image,
                                       ClayContext  *context,
                                       gint         *offset_x,
                                       gint         *offset_y,
                                       gint         *new_width,
                                       gint         *new_height,
                                       ClayProgress *progress);
void   clay_image_resize_to_selection (ClayImage    *image,
                                       ClayContext  *context,
                                       ClayProgress *progress);
