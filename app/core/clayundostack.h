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

#include "clayundo.h"


#define CLAY_TYPE_UNDO_STACK            (clay_undo_stack_get_type ())
#define CLAY_UNDO_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_UNDO_STACK, ClayUndoStack))
#define CLAY_UNDO_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_UNDO_STACK, ClayUndoStackClass))
#define CLAY_IS_UNDO_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_UNDO_STACK))
#define CLAY_IS_UNDO_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_UNDO_STACK))
#define CLAY_UNDO_STACK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_UNDO_STACK, ClayUndoStackClass))


typedef struct _ClayUndoStackClass ClayUndoStackClass;

struct _ClayUndoStack
{
  ClayUndo       parent_instance;

  ClayContainer *undos;
};

struct _ClayUndoStackClass
{
  ClayUndoClass  parent_class;
};


GType           clay_undo_stack_get_type    (void);

ClayUndoStack * clay_undo_stack_new         (ClayImage           *image);

void            clay_undo_stack_push_undo   (ClayUndoStack       *stack,
                                             ClayUndo            *undo);
ClayUndo      * clay_undo_stack_pop_undo    (ClayUndoStack       *stack,
                                             ClayUndoMode         undo_mode,
                                             ClayUndoAccumulator *accum);

ClayUndo      * clay_undo_stack_free_bottom (ClayUndoStack       *stack,
                                             ClayUndoMode         undo_mode);
ClayUndo      * clay_undo_stack_peek        (ClayUndoStack       *stack);
gint            clay_undo_stack_get_depth   (ClayUndoStack       *stack);
