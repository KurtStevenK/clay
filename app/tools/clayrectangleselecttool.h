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

#include "clayselectiontool.h"


#define CLAY_TYPE_RECTANGLE_SELECT_TOOL            (clay_rectangle_select_tool_get_type ())
#define CLAY_RECTANGLE_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_RECTANGLE_SELECT_TOOL, ClayRectangleSelectTool))
#define CLAY_RECTANGLE_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_RECTANGLE_SELECT_TOOL, ClayRectangleSelectToolClass))
#define CLAY_IS_RECTANGLE_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_RECTANGLE_SELECT_TOOL))
#define CLAY_IS_RECTANGLE_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_RECTANGLE_SELECT_TOOL))
#define CLAY_RECTANGLE_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_RECTANGLE_SELECT_TOOL, ClayRectangleSelectToolClass))

#define CLAY_RECTANGLE_SELECT_TOOL_GET_OPTIONS(t)  (CLAY_RECTANGLE_SELECT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayRectangleSelectTool        ClayRectangleSelectTool;
typedef struct _ClayRectangleSelectToolPrivate ClayRectangleSelectToolPrivate;
typedef struct _ClayRectangleSelectToolClass   ClayRectangleSelectToolClass;

struct _ClayRectangleSelectTool
{
  ClaySelectionTool               parent_instance;

  ClayRectangleSelectToolPrivate *private;
};

struct _ClayRectangleSelectToolClass
{
  ClaySelectionToolClass  parent_class;

  void (* select) (ClayRectangleSelectTool *rect_select,
                   ClayChannelOps           operation,
                   gint                     x,
                   gint                     y,
                   gint                     w,
                   gint                     h);

  gboolean draw_ellipse;
};


void    clay_rectangle_select_tool_register (ClayToolRegisterCallback  callback,
                                             gpointer                  data);

GType   clay_rectangle_select_tool_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayRectangleSelectTool, g_object_unref);
