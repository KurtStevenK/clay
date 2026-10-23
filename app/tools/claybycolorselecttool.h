/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybycolorselectool.h
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

#include "clayregionselecttool.h"


#define CLAY_TYPE_BY_COLOR_SELECT_TOOL (clay_by_color_select_tool_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayByColorSelectTool,
                          clay_by_color_select_tool,
                          CLAY, BY_COLOR_SELECT_TOOL,
                          ClayRegionSelectTool)


struct _ClayByColorSelectToolClass
{
  ClayRegionSelectToolClass  parent_class;
};


void   clay_by_color_select_tool_register (ClayToolRegisterCallback  callback,
                                           gpointer                  data);
