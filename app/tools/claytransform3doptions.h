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

#include "claytransformgridoptions.h"


#define CLAY_TYPE_TRANSFORM_3D_OPTIONS            (clay_transform_3d_options_get_type ())
#define CLAY_TRANSFORM_3D_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRANSFORM_3D_OPTIONS, ClayTransform3DOptions))
#define CLAY_TRANSFORM_3D_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRANSFORM_3D_OPTIONS, ClayTransform3DOptionsClass))
#define CLAY_IS_TRANSFORM_3D_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRANSFORM_3D_OPTIONS))
#define CLAY_IS_TRANSFORM_3D_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TRANSFORM_3D_OPTIONS))
#define CLAY_TRANSFORM_3D_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRANSFORM_3D_OPTIONS, ClayTransform3DOptionsClass))


typedef struct _ClayTransform3DOptions      ClayTransform3DOptions;
typedef struct _ClayTransform3DOptionsClass ClayTransform3DOptionsClass;

struct _ClayTransform3DOptions
{
  ClayTransformGridOptions  parent_instance;

  ClayTransform3DMode       mode;
  gboolean                  unified;

  gboolean                  constrain_axis;
  gboolean                  z_axis;
  gboolean                  local_frame;
};

struct _ClayTransform3DOptionsClass
{
  ClayTransformGridOptionsClass  parent_class;
};


GType       clay_transform_3d_options_get_type (void);

GtkWidget * clay_transform_3d_options_gui      (ClayToolOptions *tool_options);
