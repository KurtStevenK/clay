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


#define CLAY_TYPE_OPERATION_TOOL            (clay_operation_tool_get_type ())
#define CLAY_OPERATION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_TOOL, ClayOperationTool))
#define CLAY_OPERATION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_OPERATION_TOOL, ClayOperationToolClass))
#define CLAY_IS_OPERATION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_TOOL))
#define CLAY_IS_OPERATION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_OPERATION_TOOL))
#define CLAY_OPERATION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_OPERATION_TOOL, ClayOperationToolClass))


typedef struct _ClayOperationTool      ClayOperationTool;
typedef struct _ClayOperationToolClass ClayOperationToolClass;

struct _ClayOperationTool
{
  ClayFilterTool  parent_instance;

  gchar          *operation;
  gchar          *description;

  GList          *aux_inputs;

  /* dialog */
  GWeakRef        options_sw_ref;
  GWeakRef        options_box_ref;
  GWeakRef        options_gui_ref;
};

struct _ClayOperationToolClass
{
  ClayFilterToolClass  parent_class;
};


void    clay_operation_tool_register      (ClayToolRegisterCallback  callback,
                                           gpointer                  data);

GType   clay_operation_tool_get_type      (void);

void    clay_operation_tool_set_operation (ClayOperationTool        *op_tool,
                                           ClayDrawableFilter       *filter,
                                           const gchar              *operation,
                                           const gchar              *title,
                                           const gchar              *description,
                                           const gchar              *undo_desc,
                                           const gchar              *icon_name,
                                           const gchar              *help_id);
