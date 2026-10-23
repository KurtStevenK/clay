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

#include "claybrushcore.h"


#define CLAY_TYPE_DODGE_BURN (clay_dodge_burn_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDodgeBurn,
                          clay_dodge_burn,
                          CLAY, DODGE_BURN,
                          ClayBrushCore)


struct _ClayDodgeBurnClass
{
  ClayBrushCoreClass  parent_class;
};


void   clay_dodge_burn_register (Clay                      *clay,
                                 ClayPaintRegisterCallback  callback);
