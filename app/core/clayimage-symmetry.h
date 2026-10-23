/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimage-symmetry.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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


GList        * clay_image_symmetry_list       (void);

ClaySymmetry * clay_image_symmetry_new        (ClayImage    *image,
                                               GType         type);
void           clay_image_symmetry_add        (ClayImage    *image,
                                               ClaySymmetry *sym);
void           clay_image_symmetry_remove     (ClayImage    *image,
                                               ClaySymmetry *sym);
GList        * clay_image_symmetry_get        (ClayImage    *image);

gboolean       clay_image_set_active_symmetry (ClayImage    *image,
                                               GType         type);
ClaySymmetry * clay_image_get_active_symmetry (ClayImage    *image);
