/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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


GeglBuffer          * clay_drawable_transform_buffer_affine      (ClayDrawable            *drawable,
                                                                  ClayContext             *context,
                                                                  GeglBuffer              *orig_buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  const ClayMatrix3       *matrix,
                                                                  ClayTransformDirection   direction,
                                                                  ClayInterpolationType    interpolation_type,
                                                                  ClayTransformResize      clip_result,
                                                                  ClayColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y,
                                                                  ClayProgress            *progress);
GeglBuffer          * clay_drawable_transform_buffer_flip        (ClayDrawable            *drawable,
                                                                  ClayContext             *context,
                                                                  GeglBuffer              *orig_buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  ClayOrientationType      flip_type,
                                                                  gdouble                  axis,
                                                                  gboolean                 clip_result,
                                                                  ClayColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y);

GeglBuffer          * clay_drawable_transform_buffer_rotate      (ClayDrawable            *drawable,
                                                                  ClayContext             *context,
                                                                  GeglBuffer              *buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  ClayRotationType         rotate_type,
                                                                  gdouble                  center_x,
                                                                  gdouble                  center_y,
                                                                  gboolean                 clip_result,
                                                                  ClayColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y);

ClayDrawable         * clay_drawable_transform_affine            (ClayDrawable            *drawable,
                                                                  ClayContext             *context,
                                                                  const ClayMatrix3       *matrix,
                                                                  ClayTransformDirection   direction,
                                                                  ClayInterpolationType    interpolation_type,
                                                                  ClayTransformResize      clip_result,
                                                                  ClayProgress            *progress);

ClayDrawable         * clay_drawable_transform_flip              (ClayDrawable            *drawable,
                                                                  ClayContext             *context,
                                                                  ClayOrientationType      flip_type,
                                                                  gdouble                  axis,
                                                                  gboolean                 clip_result);

ClayDrawable         * clay_drawable_transform_rotate            (ClayDrawable            *drawable,
                                                                  ClayContext             *context,
                                                                  ClayRotationType         rotate_type,
                                                                  gdouble                  center_x,
                                                                  gdouble                  center_y,
                                                                  gboolean                 clip_result);

gboolean               clay_drawable_transform_needs_cut         (ClayDrawable            *drawable,
                                                                  ClayChannel             *mask);
GeglBuffer           * clay_drawable_transform_cut               (GList                   *drawables,
                                                                  ClayContext             *context,
                                                                  gint                    *offset_x,
                                                                  gint                    *offset_y,
                                                                  gboolean                *new_layer);
ClayDrawable         * clay_drawable_transform_paste             (ClayDrawable            *drawable,
                                                                  GeglBuffer              *buffer,
                                                                  ClayColorProfile        *buffer_profile,
                                                                  gint                     offset_x,
                                                                  gint                     offset_y,
                                                                  gboolean                 new_layer,
                                                                  gboolean                 push_undo);
