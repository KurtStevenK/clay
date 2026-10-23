/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Path tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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


#define CLAY_TYPE_PATH_TOOL            (clay_path_tool_get_type ())
#define CLAY_PATH_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATH_TOOL, ClayPathTool))
#define CLAY_PATH_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PATH_TOOL, ClayPathToolClass))
#define CLAY_IS_PATH_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATH_TOOL))
#define CLAY_IS_PATH_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PATH_TOOL))
#define CLAY_PATH_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PATH_TOOL, ClayPathToolClass))

#define CLAY_PATH_TOOL_GET_OPTIONS(t)  (CLAY_PATH_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayPathTool      ClayPathTool;
typedef struct _ClayPathToolClass ClayPathToolClass;

struct _ClayPathTool
{
  ClayDrawTool     parent_instance;

  ClayImage       *current_image;
  ClayVectorLayer *current_vector_layer;

  ClayPath        *path;           /*  the current Path data   */
  ClayPathMode     saved_mode;     /*  used by modifier_key()  */

  ClayToolWidget  *widget;
  ClayToolWidget  *grab_widget;

  GtkWidget       *confirm_dialog;
};

struct _ClayPathToolClass
{
  ClayDrawToolClass  parent_class;
};


void    clay_path_tool_register (ClayToolRegisterCallback  callback,
                                 gpointer                  data);

GType   clay_path_tool_get_type (void);

void    clay_path_tool_set_path (ClayPathTool             *path_tool,
                                 ClayVectorLayer          *layer,
                                 ClayPath                 *path);
