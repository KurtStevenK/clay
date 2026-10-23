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


gdouble         clay_curve_map_value         (ClayCurve     *curve,
                                              gdouble        value);
void            clay_curve_map_pixels        (ClayCurve     *curve_colors,
                                              ClayCurve     *curve_red,
                                              ClayCurve     *curve_green,
                                              ClayCurve     *curve_blue,
                                              ClayCurve     *curve_alpha,
                                              gfloat        *src,
                                              gfloat        *dest,
                                              glong          samples);
