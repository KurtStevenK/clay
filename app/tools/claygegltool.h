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

#include "clayoperationtool.h"


#define CLAY_TYPE_GEGL_TOOL            (clay_gegl_tool_get_type ())
#define CLAY_GEGL_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GEGL_TOOL, ClayGeglTool))
#define CLAY_GEGL_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GEGL_TOOL, ClayGeglToolClass))
#define CLAY_IS_GEGL_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GEGL_TOOL))
#define CLAY_IS_GEGL_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GEGL_TOOL))
#define CLAY_GEGL_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GEGL_TOOL, ClayGeglToolClass))


typedef struct _ClayGeglTool      ClayGeglTool;
typedef struct _ClayGeglToolClass ClayGeglToolClass;

struct _ClayGeglTool
{
  ClayOperationTool  parent_instance;

  /* dialog */
  GtkWidget         *operation_combo;
  GtkWidget         *description_label;
};

struct _ClayGeglToolClass
{
  ClayOperationToolClass  parent_class;
};


void    clay_gegl_tool_register (ClayToolRegisterCallback  callback,
                                 gpointer                  data);

GType   clay_gegl_tool_get_type (void);
