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

#include "claypolygonselecttool.h"


#define CLAY_TYPE_FREE_SELECT_TOOL            (clay_free_select_tool_get_type ())
#define CLAY_FREE_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FREE_SELECT_TOOL, ClayFreeSelectTool))
#define CLAY_FREE_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FREE_SELECT_TOOL, ClayFreeSelectToolClass))
#define CLAY_IS_FREE_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FREE_SELECT_TOOL))
#define CLAY_IS_FREE_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FREE_SELECT_TOOL))
#define CLAY_FREE_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FREE_SELECT_TOOL, ClayFreeSelectToolClass))


typedef struct _ClayFreeSelectTool        ClayFreeSelectTool;
typedef struct _ClayFreeSelectToolPrivate ClayFreeSelectToolPrivate;
typedef struct _ClayFreeSelectToolClass   ClayFreeSelectToolClass;

struct _ClayFreeSelectTool
{
  ClayPolygonSelectTool      parent_instance;

  ClayFreeSelectToolPrivate *priv;
};

struct _ClayFreeSelectToolClass
{
  ClayPolygonSelectToolClass  parent_class;
};


void    clay_free_select_tool_register     (ClayToolRegisterCallback  callback,
                                            gpointer                  data);

GType   clay_free_select_tool_get_type     (void);

