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


#define CLAY_TYPE_SAMPLE_POINT_TOOL            (clay_sample_point_tool_get_type ())
#define CLAY_SAMPLE_POINT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SAMPLE_POINT_TOOL, ClaySamplePointTool))
#define CLAY_SAMPLE_POINT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SAMPLE_POINT_TOOL, ClaySamplePointToolClass))
#define CLAY_IS_SAMPLE_POINT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SAMPLE_POINT_TOOL))
#define CLAY_IS_SAMPLE_POINT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SAMPLE_POINT_TOOL))
#define CLAY_SAMPLE_POINT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SAMPLE_POINT_TOOL, ClaySamplePointToolClass))


typedef struct _ClaySamplePointTool      ClaySamplePointTool;
typedef struct _ClaySamplePointToolClass ClaySamplePointToolClass;

struct _ClaySamplePointTool
{
  ClayDrawTool     parent_instance;

  ClaySamplePoint *sample_point;
  gint             sample_point_old_x;
  gint             sample_point_old_y;
  gint             sample_point_x;
  gint             sample_point_y;
};

struct _ClaySamplePointToolClass
{
  ClayDrawToolClass  parent_class;
};


GType   clay_sample_point_tool_get_type   (void);

void    clay_sample_point_tool_start_new  (ClayTool        *parent_tool,
                                           ClayDisplay     *display);
void    clay_sample_point_tool_start_edit (ClayTool        *parent_tool,
                                           ClayDisplay     *display,
                                           ClaySamplePoint *sample_point);
