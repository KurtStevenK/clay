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

#include "core/clayitemundo.h"


#define CLAY_TYPE_PATH_UNDO            (clay_path_undo_get_type ())
#define CLAY_PATH_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATH_UNDO, ClayPathUndo))
#define CLAY_PATH_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PATH_UNDO, ClayPathUndoClass))
#define CLAY_IS_PATH_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATH_UNDO))
#define CLAY_IS_PATH_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PATH_UNDO))
#define CLAY_PATH_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PATH_UNDO, ClayPathUndoClass))


typedef struct _ClayPathUndo      ClayPathUndo;
typedef struct _ClayPathUndoClass ClayPathUndoClass;

struct _ClayPathUndo
{
  ClayItemUndo  parent_instance;

  ClayPath     *prev_parent;
  gint          prev_position;   /*  former position in list    */
  GList        *prev_paths;      /*  previous selected path  */
};

struct _ClayPathUndoClass
{
  ClayItemUndoClass  parent_class;
};


GType   clay_path_undo_get_type (void);
