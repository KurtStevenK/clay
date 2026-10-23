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


#define CLAY_TYPE_MOVE_OPTIONS            (clay_move_options_get_type ())
#define CLAY_MOVE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MOVE_OPTIONS, ClayMoveOptions))
#define CLAY_MOVE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MOVE_OPTIONS, ClayMoveOptionsClass))
#define CLAY_IS_MOVE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MOVE_OPTIONS))
#define CLAY_IS_MOVE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MOVE_OPTIONS))
#define CLAY_MOVE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MOVE_OPTIONS, ClayMoveOptionsClass))


typedef struct _ClayMoveOptions      ClayMoveOptions;
typedef struct _ClayToolOptionsClass ClayMoveOptionsClass;

struct _ClayMoveOptions
{
  ClayToolOptions    parent_instance;

  ClayTransformType  move_type;
  gboolean           move_current;

  /*  options gui  */
  GtkWidget         *type_box;
};


GType       clay_move_options_get_type (void);

GtkWidget * clay_move_options_gui      (ClayToolOptions *tool_options);
