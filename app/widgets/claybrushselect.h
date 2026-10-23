/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrushselect.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "claypdbdialog.h"


#define CLAY_TYPE_BRUSH_SELECT (clay_brush_select_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayBrushSelect,
                          clay_brush_select,
                          CLAY, BRUSH_SELECT,
                          ClayPdbDialog)


struct _ClayBrushSelectClass
{
  ClayPdbDialogClass  parent_class;
};
