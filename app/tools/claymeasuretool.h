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

#include "claytransformtool.h"


#define CLAY_TYPE_MEASURE_TOOL            (clay_measure_tool_get_type ())
#define CLAY_MEASURE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MEASURE_TOOL, ClayMeasureTool))
#define CLAY_MEASURE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MEASURE_TOOL, ClayMeasureToolClass))
#define CLAY_IS_MEASURE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MEASURE_TOOL))
#define CLAY_IS_MEASURE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MEASURE_TOOL))
#define CLAY_MEASURE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MEASURE_TOOL, ClayMeasureToolClass))

#define CLAY_MEASURE_TOOL_GET_OPTIONS(t)  (CLAY_MEASURE_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayMeasureTool      ClayMeasureTool;
typedef struct _ClayMeasureToolClass ClayMeasureToolClass;

struct _ClayMeasureTool
{
  ClayTransformTool  parent_instance;

  ClayToolWidget    *widget;
  ClayToolWidget    *grab_widget;

  gboolean           supress_guides;

  gint               n_points;
  gint               x[3];
  gint               y[3];

  ClayToolGui       *gui;
  GtkWidget         *distance_label[2];
  GtkWidget         *angle_label[2];
  GtkWidget         *width_label[2];
  GtkWidget         *height_label[2];
  GtkWidget         *unit_label[4];
};

struct _ClayMeasureToolClass
{
  ClayTransformToolClass  parent_class;
};


void    clay_measure_tool_register (ClayToolRegisterCallback  callback,
                                    gpointer                  data);

GType   clay_measure_tool_get_type (void);
