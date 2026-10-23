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

#include "core/claytooloptions.h"


#define CLAY_TYPE_TRANSFORM_OPTIONS            (clay_transform_options_get_type ())
#define CLAY_TRANSFORM_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRANSFORM_OPTIONS, ClayTransformOptions))
#define CLAY_TRANSFORM_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRANSFORM_OPTIONS, ClayTransformOptionsClass))
#define CLAY_IS_TRANSFORM_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRANSFORM_OPTIONS))
#define CLAY_IS_TRANSFORM_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TRANSFORM_OPTIONS))
#define CLAY_TRANSFORM_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRANSFORM_OPTIONS, ClayTransformOptionsClass))


typedef struct _ClayTransformOptions      ClayTransformOptions;
typedef struct _ClayTransformOptionsClass ClayTransformOptionsClass;

struct _ClayTransformOptions
{
  ClayToolOptions         parent_instance;

  ClayTransformType       type;
  ClayTransformDirection  direction;
  ClayInterpolationType   interpolation;
  ClayTransformResize     clip;

  /*  options gui  */
  GtkWidget              *type_box;
  GtkWidget              *direction_frame;
};

struct _ClayTransformOptionsClass
{
  ClayToolOptionsClass  parent_class;
};


GType       clay_transform_options_get_type (void);

GtkWidget * clay_transform_options_gui      (ClayToolOptions *tool_options,
                                             gboolean         direction,
                                             gboolean         interpolation,
                                             gboolean         clipping);
