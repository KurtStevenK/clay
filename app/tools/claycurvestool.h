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

#include "clayfiltertool.h"


#define CLAY_TYPE_CURVES_TOOL            (clay_curves_tool_get_type ())
#define CLAY_CURVES_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CURVES_TOOL, ClayCurvesTool))
#define CLAY_IS_CURVES_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CURVES_TOOL))
#define CLAY_CURVES_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CURVES_TOOL, ClayCurvesToolClass))
#define CLAY_IS_CURVES_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CURVES_TOOL))


typedef struct _ClayCurvesTool      ClayCurvesTool;
typedef struct _ClayCurvesToolClass ClayCurvesToolClass;

struct _ClayCurvesTool
{
  ClayFilterTool  parent_instance;

  /* dialog */
  gdouble           scale;
  gdouble           picked_color[5];

  GtkWidget        *channel_menu;
  GtkWidget        *xrange;
  GtkWidget        *yrange;
  GtkWidget        *graph;
  GtkWidget        *point_box;
  GtkWidget        *point_input;
  GtkWidget        *point_output;
  GtkWidget        *point_type;
  GtkWidget        *curve_type;
};

struct _ClayCurvesToolClass
{
  ClayFilterToolClass  parent_class;
};


void    clay_curves_tool_register (ClayToolRegisterCallback  callback,
                                   gpointer                  data);

GType   clay_curves_tool_get_type (void);
