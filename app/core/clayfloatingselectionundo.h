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


#define CLAY_TYPE_FLOATING_SELECTION_UNDO            (clay_floating_selection_undo_get_type ())
#define CLAY_FLOATING_SELECTION_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FLOATING_SELECTION_UNDO, ClayFloatingSelectionUndo))
#define CLAY_FLOATING_SELECTION_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FLOATING_SELECTION_UNDO, ClayFloatingSelectionUndoClass))
#define CLAY_IS_FLOATING_SELECTION_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FLOATING_SELECTION_UNDO))
#define CLAY_IS_FLOATING_SELECTION_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FLOATING_SELECTION_UNDO))
#define CLAY_FLOATING_SELECTION_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FLOATING_SELECTION_UNDO, ClayFloatingSelectionUndoClass))


typedef struct _ClayFloatingSelectionUndo      ClayFloatingSelectionUndo;
typedef struct _ClayFloatingSelectionUndoClass ClayFloatingSelectionUndoClass;

struct _ClayFloatingSelectionUndo
{
  ClayItemUndo  parent_instance;

  ClayDrawable *drawable;
};

struct _ClayFloatingSelectionUndoClass
{
  ClayItemUndoClass  parent_class;
};


GType   clay_floating_selection_undo_get_type (void);
