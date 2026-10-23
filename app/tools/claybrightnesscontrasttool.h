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


#define CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL            (clay_brightness_contrast_tool_get_type ())
#define CLAY_BRIGHTNESS_CONTRAST_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL, ClayBrightnessContrastTool))
#define CLAY_BRIGHTNESS_CONTRAST_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL, ClayBrightnessContrastToolClass))
#define CLAY_IS_BRIGHTNESS_CONTRAST_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL))
#define CLAY_IS_BRIGHTNESS_CONTRAST_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL))
#define CLAY_BRIGHTNESS_CONTRAST_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRIGHTNESS_CONTRAST_TOOL, ClayBrightnessContrastToolClass))


typedef struct _ClayBrightnessContrastTool      ClayBrightnessContrastTool;
typedef struct _ClayBrightnessContrastToolClass ClayBrightnessContrastToolClass;

struct _ClayBrightnessContrastTool
{
  ClayFilterTool  parent_instance;

  gboolean        dragging;
  gdouble         x, y;
  gdouble         dx, dy;

  /* dialog */
  GtkWidget      *brightness_scale;
  GtkWidget      *contrast_scale;
};

struct _ClayBrightnessContrastToolClass
{
  ClayFilterToolClass  parent_class;
};


void    clay_brightness_contrast_tool_register (ClayToolRegisterCallback  callback,
                                                gpointer                  data);

GType   clay_brightness_contrast_tool_get_type (void);
