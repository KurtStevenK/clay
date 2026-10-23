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

#include "claybrushtool.h"


#define CLAY_TYPE_SOURCE_TOOL            (clay_source_tool_get_type ())
#define CLAY_SOURCE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SOURCE_TOOL, ClaySourceTool))
#define CLAY_SOURCE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SOURCE_TOOL, ClaySourceToolClass))
#define CLAY_IS_SOURCE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SOURCE_TOOL))
#define CLAY_IS_SOURCE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SOURCE_TOOL))
#define CLAY_SOURCE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SOURCE_TOOL, ClaySourceToolClass))

#define CLAY_SOURCE_TOOL_GET_OPTIONS(t)  (CLAY_SOURCE_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClaySourceTool      ClaySourceTool;
typedef struct _ClaySourceToolClass ClaySourceToolClass;

struct _ClaySourceTool
{
  ClayBrushTool        parent_instance;

  ClayDisplay         *src_display;
  gint                 src_x;
  gint                 src_y;

  gboolean             show_source_outline;
  ClayCursorPrecision  saved_precision;

  ClayCanvasItem      *src_handle;
  ClayCanvasItem      *src_outline;

  const gchar         *status_paint;
  const gchar         *status_set_source;
  const gchar         *status_set_source_ctrl;
  const gchar         *status_set_source_ctrl_click;
};

struct _ClaySourceToolClass
{
  ClayBrushToolClass  parent_class;
};


GType   clay_source_tool_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClaySourceTool, g_object_unref);
