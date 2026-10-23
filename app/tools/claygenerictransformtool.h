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

#include "claytransformgridtool.h"


#define CLAY_TYPE_GENERIC_TRANSFORM_TOOL            (clay_generic_transform_tool_get_type ())
#define CLAY_GENERIC_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GENERIC_TRANSFORM_TOOL, ClayGenericTransformTool))
#define CLAY_GENERIC_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GENERIC_TRANSFORM_TOOL, ClayGenericTransformToolClass))
#define CLAY_IS_GENERIC_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GENERIC_TRANSFORM_TOOL))
#define CLAY_IS_GENERIC_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GENERIC_TRANSFORM_TOOL))
#define CLAY_GENERIC_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GENERIC_TRANSFORM_TOOL, ClayGenericTransformToolClass))


typedef struct _ClayGenericTransformToolClass ClayGenericTransformToolClass;

struct _ClayGenericTransformTool
{
  ClayTransformGridTool  parent_instance;

  ClayVector2            input_points[4];
  ClayVector2            output_points[4];

  GtkWidget             *matrix_label;
  GtkWidget             *invalid_label;
};

struct _ClayGenericTransformToolClass
{
  ClayTransformGridToolClass  parent_class;

  /*  virtual functions  */
  void   (* info_to_points) (ClayGenericTransformTool *generic);
};


GType   clay_generic_transform_tool_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayGenericTransformTool, g_object_unref);
