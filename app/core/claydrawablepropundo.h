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

#include "clayitemundo.h"


#define CLAY_TYPE_DRAWABLE_PROP_UNDO            (clay_drawable_prop_undo_get_type ())
#define CLAY_DRAWABLE_PROP_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE_PROP_UNDO, ClayDrawablePropUndo))
#define CLAY_DRAWABLE_PROP_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAWABLE_PROP_UNDO, ClayDrawablePropUndoClass))
#define CLAY_IS_DRAWABLE_PROP_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE_PROP_UNDO))
#define CLAY_IS_DRAWABLE_PROP_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAWABLE_PROP_UNDO))
#define CLAY_DRAWABLE_PROP_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAWABLE_PROP_UNDO, ClayDrawablePropUndoClass))


typedef struct _ClayDrawablePropUndo      ClayDrawablePropUndo;
typedef struct _ClayDrawablePropUndoClass ClayDrawablePropUndoClass;

struct _ClayDrawablePropUndo
{
  ClayItemUndo  parent_instance;

  const Babl   *format;
};

struct _ClayDrawablePropUndoClass
{
  ClayItemUndoClass  parent_class;
};


GType   clay_drawable_prop_undo_get_type (void);
