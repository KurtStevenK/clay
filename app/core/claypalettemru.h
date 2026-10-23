/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypalettemru.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#include "claypalette.h"


#define CLAY_TYPE_PALETTE_MRU (clay_palette_mru_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayPaletteMru,
                          clay_palette_mru,
                          CLAY, PALETTE_MRU,
                          ClayPalette)


struct _ClayPaletteMruClass
{
  ClayPaletteClass  parent_class;
};


ClayData * clay_palette_mru_new  (const gchar    *name);

void       clay_palette_mru_load (ClayPaletteMru *mru,
                                  GFile          *file);
void       clay_palette_mru_save (ClayPaletteMru *mru,
                                  GFile          *file);

void       clay_palette_mru_add  (ClayPaletteMru *mru,
                                  GeglColor      *color);
