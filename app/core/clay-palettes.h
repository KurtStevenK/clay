/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * clay-palettes.h
 * Copyright (C) 2014 Michael Natterer  <mitch@gimp.org>
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


void          clay_palettes_init              (Clay      *clay);

void          clay_palettes_load              (Clay      *clay);
void          clay_palettes_save              (Clay      *clay);

ClayPalette * clay_palettes_get_color_history (Clay      *clay);
void          clay_palettes_add_color_history (Clay      *clay,
                                               GeglColor *color);
