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


#define CLAY_TYPE_TRANSFORM_3D_TOOL           (clay_transform_3d_tool_get_type ())
#define CLAY_TRANSFORM_3D_TOOL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRANSFORM_3D_TOOL, ClayTransform3DTool))
#define CLAY_TRANSFORM_3D_TOOL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRANSFORM_3D_TOOL, ClayTransform3DToolClass))
#define CLAY_IS_TRANSFORM_3D_TOOL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRANSFORM_3D_TOOL))
#define CLAY_TRANSFORM_3D_TOOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRANSFORM_3D_TOOL, ClayTransform3DToolClass))

#define CLAY_TRANSFORM_3D_TOOL_GET_OPTIONS(t) (CLAY_TRANSFORM_3D_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayTransform3DTool      ClayTransform3DTool;
typedef struct _ClayTransform3DToolClass ClayTransform3DToolClass;

struct _ClayTransform3DTool
{
  ClayTransformGridTool  parent_instance;

  gboolean               updating;

  GtkWidget             *notebook;
  GtkWidget             *vanishing_point_se;
  GtkWidget             *lens_mode_combo;
  GtkWidget             *focal_length_se;
  GtkWidget             *angle_of_view_scale;
  GtkAdjustment         *angle_of_view_adj;
  GtkWidget             *offset_se;
  GtkWidget             *rotation_order_buttons[3];
  GtkAdjustment         *angle_adj[3];
  GtkWidget             *pivot_selector;
};

struct _ClayTransform3DToolClass
{
  ClayTransformGridToolClass parent_class;
};


void    clay_transform_3d_tool_register (ClayToolRegisterCallback  callback,
                                         gpointer                  data);

GType   clay_transform_3d_tool_get_type (void);

