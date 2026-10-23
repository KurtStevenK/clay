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


#define CLAY_BRUSH_FILE_EXTENSION        ".gbr"
#define CLAY_BRUSH_PIXMAP_FILE_EXTENSION ".gpb"
#define CLAY_BRUSH_PS_FILE_EXTENSION     ".abr"
#define CLAY_BRUSH_PSP_FILE_EXTENSION    ".jbr"


GList     * clay_brush_load        (ClayContext   *context,
                                    GFile         *file,
                                    GInputStream  *input,
                                    GError       **error);
ClayBrush * clay_brush_load_brush  (ClayContext   *context,
                                    GFile         *file,
                                    GInputStream  *input,
                                    GError       **error);

GList     * clay_brush_load_abr    (ClayContext   *context,
                                    GFile         *file,
                                    GInputStream  *input,
                                    GError       **error);
