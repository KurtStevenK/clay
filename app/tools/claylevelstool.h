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


#define CLAY_TYPE_LEVELS_TOOL            (clay_levels_tool_get_type ())
#define CLAY_LEVELS_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LEVELS_TOOL, ClayLevelsTool))
#define CLAY_LEVELS_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LEVELS_TOOL, ClayLevelsToolClass))
#define CLAY_IS_LEVELS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LEVELS_TOOL))
#define CLAY_IS_LEVELS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LEVELS_TOOL))
#define CLAY_LEVELS_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LEVELS_TOOL, ClayLevelsToolClass))


typedef struct _ClayLevelsTool      ClayLevelsTool;
typedef struct _ClayLevelsToolClass ClayLevelsToolClass;

struct _ClayLevelsTool
{
  ClayFilterTool  parent_instance;

  /* dialog */
  ClayHistogram  *histogram;
  ClayAsync      *histogram_async;

  GtkWidget      *channel_menu;

  GtkWidget      *histogram_view;

  GtkWidget      *input_bar;
  GtkWidget      *input_handle_bar;
  GtkWidget      *low_input_spinbutton;
  GtkWidget      *high_input_spinbutton;
  GtkWidget      *low_output_spinbutton;
  GtkWidget      *high_output_spinbutton;
  GtkAdjustment  *low_input;
  GtkAdjustment  *gamma;
  GtkAdjustment  *gamma_linear;
  GtkAdjustment  *high_input;

  GtkWidget      *output_bar;
  GtkWidget      *output_handle_bar;
};

struct _ClayLevelsToolClass
{
  ClayFilterToolClass  parent_class;
};


void    clay_levels_tool_register (ClayToolRegisterCallback  callback,
                                   gpointer                  data);

GType   clay_levels_tool_get_type (void);
