/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycoords.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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


void     clay_coords_mix            (const gdouble     amul,
                                     const ClayCoords *a,
                                     const gdouble     bmul,
                                     const ClayCoords *b,
                                     ClayCoords       *ret_val);
void     clay_coords_average        (const ClayCoords *a,
                                     const ClayCoords *b,
                                     ClayCoords       *ret_average);
void     clay_coords_add            (const ClayCoords *a,
                                     const ClayCoords *b,
                                     ClayCoords       *ret_add);
void     clay_coords_difference     (const ClayCoords *a,
                                     const ClayCoords *b,
                                     ClayCoords       *difference);
void     clay_coords_scale          (const gdouble     f,
                                     const ClayCoords *a,
                                     ClayCoords       *ret_product);

gdouble  clay_coords_scalarprod     (const ClayCoords *a,
                                     const ClayCoords *b);
gdouble  clay_coords_length         (const ClayCoords *a);
gdouble  clay_coords_length_squared (const ClayCoords *a);
gdouble  clay_coords_manhattan_dist (const ClayCoords *a,
                                     const ClayCoords *b);

gboolean clay_coords_equal          (const ClayCoords *a,
                                     const ClayCoords *b);

gdouble  clay_coords_direction      (const ClayCoords *a,
                                     const ClayCoords *b);
