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

#include "claytransformoptions.h"


#define CLAY_TYPE_FLIP_OPTIONS            (clay_flip_options_get_type ())
#define CLAY_FLIP_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FLIP_OPTIONS, ClayFlipOptions))
#define CLAY_FLIP_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FLIP_OPTIONS, ClayFlipOptionsClass))
#define CLAY_IS_FLIP_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FLIP_OPTIONS))
#define CLAY_IS_FLIP_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FLIP_OPTIONS))
#define CLAY_FLIP_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FLIP_OPTIONS, ClayFlipOptionsClass))


typedef struct _ClayFlipOptions      ClayFlipOptions;
typedef struct _ClayToolOptionsClass ClayFlipOptionsClass;

struct _ClayFlipOptions
{
  ClayTransformOptions  parent_instance;

  ClayOrientationType   flip_type;

  /*  options gui  */
  GtkWidget            *direction_frame;
};


GType       clay_flip_options_get_type (void);

GtkWidget * clay_flip_options_gui      (ClayToolOptions *tool_options);
