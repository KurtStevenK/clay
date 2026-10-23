/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycoords-interpolate.h
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


void       clay_coords_interpolate_bezier    (const ClayCoords  bezier_pt[4],
                                              gdouble           precision,
                                              GArray           *ret_coords,
                                              GArray           *ret_params);

void       clay_coords_interpolate_bezier_at (const ClayCoords  bezier_pt[4],
                                              gdouble           t,
                                              ClayCoords       *position,
                                              ClayCoords       *velocity);

gboolean   clay_coords_bezier_is_straight    (const ClayCoords  bezier_pt[4],
                                              gdouble           precision);

void       clay_coords_interpolate_catmull   (const ClayCoords  catmull_pt[4],
                                              gdouble           precision,
                                              GArray           *ret_coords,
                                              GArray           *ret_params);
