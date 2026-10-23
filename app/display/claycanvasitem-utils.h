/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasitem-utils.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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


gboolean   clay_canvas_item_on_handle           (ClayCanvasItem    *item,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 ClayHandleType     type,
                                                 gdouble            handle_x,
                                                 gdouble            handle_y,
                                                 gint               width,
                                                 gint               height,
                                                 ClayHandleAnchor   anchor);

gboolean   clay_canvas_item_on_path_handle      (ClayCanvasItem    *item,
                                                 ClayPath          *path,
                                                 const ClayCoords  *coord,
                                                 gint               width,
                                                 gint               height,
                                                 ClayAnchorType     preferred,
                                                 gboolean           exclusive,
                                                 ClayAnchor       **ret_anchor,
                                                 ClayStroke       **ret_stroke);
gboolean   clay_canvas_item_on_path_curve       (ClayCanvasItem    *item,
                                                 ClayPath          *path,
                                                 const ClayCoords  *coord,
                                                 gint               width,
                                                 gint               height,
                                                 ClayCoords        *ret_coords,
                                                 gdouble           *ret_pos,
                                                 ClayAnchor       **ret_segment_start,
                                                 ClayAnchor       **ret_segment_end,
                                                 ClayStroke       **ret_stroke);
gboolean   clay_canvas_item_on_path             (ClayCanvasItem    *item,
                                                 const ClayCoords  *coords,
                                                 gint               width,
                                                 gint               height,
                                                 ClayCoords        *ret_coords,
                                                 gdouble           *ret_pos,
                                                 ClayAnchor       **ret_segment_start,
                                                 ClayAnchor       **ret_segment_end,
                                                 ClayStroke       **ret_stroke,
                                                 ClayPath         **ret_path);

void       clay_canvas_item_shift_to_north_west (ClayHandleAnchor   anchor,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 gint               width,
                                                 gint               height,
                                                 gdouble           *shifted_x,
                                                 gdouble           *shifted_y);
void       clay_canvas_item_shift_to_center     (ClayHandleAnchor   anchor,
                                                 gdouble            x,
                                                 gdouble            y,
                                                 gint               width,
                                                 gint               height,
                                                 gdouble           *shifted_x,
                                                 gdouble           *shifted_y);
