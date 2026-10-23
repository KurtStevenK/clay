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


typedef struct _ClayImageSnapData
{
  ClayAlignmentType  snapped_side_horizontal; /* if CLAY_ARRANGE_HFILL then no snapping happen */
  ClayLayer         *snapped_layer_horizontal;

  ClayAlignmentType  snapped_side_vertical; /* if CLAY_ARRANGE_HFILL then no snapping happen */
  ClayLayer         *snapped_layer_vertical;

  ClayAlignmentType  equidistance_side_horizontal;
  ClayLayer         *near_layer_horizontal1;
  ClayLayer         *near_layer_horizontal2;

  ClayAlignmentType  equidistance_side_vertical;
  ClayLayer         *near_layer_vertical1;
  ClayLayer         *near_layer_vertical2;
} ClayImageSnapData;


gboolean    clay_image_snap_x         (ClayImage         *image,
                                       ClayImageSnapData *snapping_data,
                                       gdouble            x,
                                       gdouble           *tx,
                                       gdouble            epsilon_x,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_bbox,
                                       gboolean           snap_to_equidistance,
                                       ClayAlignmentType  alignment_side);
gboolean    clay_image_snap_y         (ClayImage         *image,
                                       ClayImageSnapData *snapping_data,
                                       gdouble            y,
                                       gdouble           *ty,
                                       gdouble            epsilon_y,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_bbox,
                                       gboolean           snap_to_equidistance,
                                       ClayAlignmentType  alignment_side);
gboolean    clay_image_snap_point     (ClayImage         *image,
                                       gdouble            x,
                                       gdouble            y,
                                       gdouble           *tx,
                                       gdouble           *ty,
                                       gdouble            epsilon_x,
                                       gdouble            epsilon_y,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_path,
                                       gboolean           snap_to_bbox,
                                       gboolean           show_all);
gboolean   clay_image_snap_rectangle  (ClayImage         *image,
                                       ClayImageSnapData *snapping_data,
                                       gdouble            x1,
                                       gdouble            y1,
                                       gdouble            x2,
                                       gdouble            y2,
                                       gdouble           *tx1,
                                       gdouble           *ty1,
                                       gdouble            epsilon_x,
                                       gdouble            epsilon_y,
                                       gboolean           snap_to_guides,
                                       gboolean           snap_to_grid,
                                       gboolean           snap_to_canvas,
                                       gboolean           snap_to_path,
                                       gboolean           snap_to_bbox,
                                       gboolean           snap_to_equidistance);
