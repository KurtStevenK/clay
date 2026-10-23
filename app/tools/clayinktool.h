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

#include "claypainttool.h"


#define CLAY_TYPE_INK_TOOL (clay_ink_tool_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayInkTool,
                          clay_ink_tool,
                          CLAY, INK_TOOL,
                          ClayPaintTool)

#define CLAY_INK_TOOL_GET_OPTIONS(t) (CLAY_INK_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


struct _ClayInkToolClass
{
  ClayPaintToolClass parent_class;
};


void   clay_ink_tool_register (ClayToolRegisterCallback  callback,
                               gpointer                  data);
