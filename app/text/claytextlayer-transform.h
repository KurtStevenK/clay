/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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


void  clay_text_layer_scale     (ClayItem               *item,
                                 gint                    new_width,
                                 gint                    new_height,
                                 gint                    new_offset_x,
                                 gint                    new_offset_y,
                                 ClayInterpolationType   interpolation_type,
                                 ClayProgress           *progress);
void  clay_text_layer_flip      (ClayItem               *item,
                                 ClayContext            *context,
                                 ClayOrientationType     flip_type,
                                 gdouble                 axis,
                                 gboolean                clip_result);
void  clay_text_layer_rotate    (ClayItem               *item,
                                 ClayContext            *context,
                                 ClayRotationType        rotate_type,
                                 gdouble                 center_x,
                                 gdouble                 center_y,
                                 gboolean                clip_result);
void  clay_text_layer_transform (ClayItem               *item,
                                 ClayContext            *context,
                                 const ClayMatrix3      *matrix,
                                 ClayTransformDirection  direction,
                                 ClayInterpolationType   interpolation_type,
                                 ClayTransformResize     clip_result,
                                 ClayProgress           *progress,
                                 gboolean                push_undo);
