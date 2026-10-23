/* xyz.kainzmayer.ClayDesign
 *
 * claywarptool.h
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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


#define CLAY_TYPE_WARP_TOOL            (clay_warp_tool_get_type ())
#define CLAY_WARP_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_WARP_TOOL, ClayWarpTool))
#define CLAY_WARP_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_WARP_TOOL, ClayWarpToolClass))
#define CLAY_IS_WARP_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_WARP_TOOL))
#define CLAY_IS_WARP_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_WARP_TOOL))
#define CLAY_WARP_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_WARP_TOOL, ClayWarpToolClass))

#define CLAY_WARP_TOOL_GET_OPTIONS(t)  (CLAY_WARP_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayWarpTool      ClayWarpTool;
typedef struct _ClayWarpToolClass ClayWarpToolClass;

struct _ClayWarpTool
{
  ClayDrawTool        parent_instance;

  gboolean            show_cursor;
  gboolean            draw_brush;
  gboolean            snap_brush;

  ClayVector2         cursor_pos;    /* Hold the cursor position */

  GeglBuffer         *coords_buffer; /* Buffer where coordinates are stored */

  GeglNode           *graph;         /* Top level GeglNode */
  GeglNode           *render_node;   /* Node to render the transformation */

  GeglPath           *current_stroke;
  guint               stroke_timer;

  ClayVector2         last_pos;
  gdouble             total_dist;

  ClayDrawableFilter *filter;

  GList              *redo_stack;
};

struct _ClayWarpToolClass
{
  ClayDrawToolClass parent_class;
};


void    clay_warp_tool_register (ClayToolRegisterCallback  callback,
                                 gpointer                  data);

GType   clay_warp_tool_get_type (void);
