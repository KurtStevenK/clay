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


gboolean   clay_image_item_list_bounds    (ClayImage              *image,
                                           GList                  *list,
                                           gint                   *x,
                                           gint                   *y,
                                           gint                   *width,
                                           gint                   *height);

void       clay_image_item_list_translate (ClayImage              *image,
                                           GList                  *list,
                                           gint                    offset_x,
                                           gint                    offset_y,
                                           gboolean                push_undo);
void       clay_image_item_list_flip      (ClayImage              *image,
                                           GList                  *list,
                                           ClayContext            *context,
                                           ClayOrientationType     flip_type,
                                           gdouble                 axis,
                                           ClayTransformResize     expected_clip_result);
void       clay_image_item_list_rotate    (ClayImage              *image,
                                           GList                  *list,
                                           ClayContext            *context,
                                           ClayRotationType        rotate_type,
                                           gdouble                 center_x,
                                           gdouble                 center_y,
                                           gboolean                clip_result);
void       clay_image_item_list_transform (ClayImage              *image,
                                           GList                  *list,
                                           ClayContext            *context,
                                           const ClayMatrix3      *matrix,
                                           ClayTransformDirection  direction,
                                           ClayInterpolationType   interpolation_type,
                                           ClayTransformResize     clip_result,
                                           ClayProgress           *progress);

GList    * clay_image_item_list_get_list  (ClayImage              *image,
                                           ClayItemTypeMask        type,
                                           ClayItemSet             set);

GList    * clay_image_item_list_filter    (GList                  *list);
