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

#include "claydrawtool.h"


#define CLAY_TYPE_MAGNIFY_TOOL            (clay_magnify_tool_get_type ())
#define CLAY_MAGNIFY_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MAGNIFY_TOOL, ClayMagnifyTool))
#define CLAY_MAGNIFY_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MAGNIFY_TOOL, ClayMagnifyToolClass))
#define CLAY_IS_MAGNIFY_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MAGNIFY_TOOL))
#define CLAY_IS_MAGNIFY_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MAGNIFY_TOOL))
#define CLAY_MAGNIFY_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MAGNIFY_TOOL, ClayMagnifyToolClass))

#define CLAY_MAGNIFY_TOOL_GET_OPTIONS(t)  (CLAY_MAGNIFY_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayMagnifyTool      ClayMagnifyTool;
typedef struct _ClayMagnifyToolClass ClayMagnifyToolClass;

struct _ClayMagnifyTool
{
  ClayDrawTool    parent_instance;

  gdouble         x, y;  /*  upper left hand coordinate  */
  gdouble         w, h;  /*  width and height            */

  ClayCanvasItem *rectangle;
};

struct _ClayMagnifyToolClass
{
  ClayDrawToolClass  parent_class;
};


void    clay_magnify_tool_register (ClayToolRegisterCallback  callback,
                                    gpointer                  data);

GType   clay_magnify_tool_get_type (void);
