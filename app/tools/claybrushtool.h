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


#define CLAY_TYPE_BRUSH_TOOL            (clay_brush_tool_get_type ())
#define CLAY_BRUSH_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_TOOL, ClayBrushTool))
#define CLAY_BRUSH_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_TOOL, ClayBrushToolClass))
#define CLAY_IS_BRUSH_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_TOOL))
#define CLAY_IS_BRUSH_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_TOOL))
#define CLAY_BRUSH_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_TOOL, ClayBrushToolClass))


typedef struct _ClayBrushToolClass ClayBrushToolClass;

struct _ClayBrushTool
{
  ClayPaintTool   parent_instance;

  ClayBezierDesc *boundary;
  gint            boundary_width;
  gint            boundary_height;
  gdouble         boundary_scale;
  gdouble         boundary_aspect_ratio;
  gdouble         boundary_angle;
  gboolean        boundary_reflect;
  gdouble         boundary_hardness;
};

struct _ClayBrushToolClass
{
  ClayPaintToolClass  parent_class;
};


GType            clay_brush_tool_get_type       (void);

ClayCanvasItem * clay_brush_tool_create_outline (ClayBrushTool *brush_tool,
                                                 ClayDisplay   *display,
                                                 gdouble        x,
                                                 gdouble        y);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayBrushTool, g_object_unref);
