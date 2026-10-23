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


#define CLAY_TYPE_THRESHOLD_TOOL            (clay_threshold_tool_get_type ())
#define CLAY_THRESHOLD_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_THRESHOLD_TOOL, ClayThresholdTool))
#define CLAY_THRESHOLD_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_THRESHOLD_TOOL, ClayThresholdToolClass))
#define CLAY_IS_THRESHOLD_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_THRESHOLD_TOOL))
#define CLAY_IS_THRESHOLD_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_THRESHOLD_TOOL))
#define CLAY_THRESHOLD_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_THRESHOLD_TOOL, ClayThresholdToolClass))


typedef struct _ClayThresholdTool      ClayThresholdTool;
typedef struct _ClayThresholdToolClass ClayThresholdToolClass;

struct _ClayThresholdTool
{
  ClayFilterTool    parent_instance;

  /*  dialog  */
  ClayHistogram    *histogram;
  ClayAsync        *histogram_async;
  GtkWidget        *channel_menu;
  ClayHistogramBox *histogram_box;
};

struct _ClayThresholdToolClass
{
  ClayFilterToolClass  parent_class;
};


void    clay_threshold_tool_register (ClayToolRegisterCallback  callback,
                                      gpointer                  data);

GType   clay_threshold_tool_get_type (void);
