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


#define CLAY_TYPE_PAINT_SELECT_TOOL            (clay_paint_select_tool_get_type ())
#define CLAY_PAINT_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAINT_SELECT_TOOL, ClayPaintSelectTool))
#define CLAY_PAINT_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAINT_SELECT_TOOL, ClayPaintSelectToolClass))
#define CLAY_IS_PAINT_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAINT_SELECT_TOOL))
#define CLAY_IS_PAINT_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAINT_SELECT_TOOL))
#define CLAY_PAINT_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAINT_SELECT_TOOL, ClayPaintSelectToolClass))

#define CLAY_PAINT_SELECT_TOOL_GET_OPTIONS(t)  (CLAY_PAINT_SELECT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayPaintSelectTool      ClayPaintSelectTool;
typedef struct _ClayPaintSelectToolClass ClayPaintSelectToolClass;

struct _ClayPaintSelectTool
{
  ClayDrawTool           parent_instance;

  ClayChannelOps         saved_op;       /*  selection mode before modifier press */

  ClayChannelOps         painting_op;    /*  selection mode at start of painting. */
  GeglBuffer            *trimap;
  GeglBuffer            *image_mask;
  GeglBuffer            *drawable;
  GeglBuffer            *scribble;

  gint                   drawable_off_x;
  gint                   drawable_off_y;
  gint                   drawable_width;
  gint                   drawable_height;

  GeglNode              *graph;
  GeglNode              *ps_node;
  GeglNode              *threshold_node;
  GeglNode              *render_node;

  ClayVector2            last_pos;

  gboolean               process;
};

struct _ClayPaintSelectToolClass
{
  ClayDrawToolClass  parent_class;
};


void    clay_paint_select_tool_register (ClayToolRegisterCallback  callback,
                                         gpointer                  data);

GType   clay_paint_select_tool_get_type (void);
