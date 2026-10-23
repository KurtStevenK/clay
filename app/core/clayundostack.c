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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clayimage.h"
#include "claylist.h"
#include "clayundo.h"
#include "clayundostack.h"


static void    clay_undo_stack_finalize    (GObject             *object);

static gint64  clay_undo_stack_get_memsize (ClayObject          *object,
                                            gint64              *gui_size);

static void    clay_undo_stack_pop         (ClayUndo            *undo,
                                            ClayUndoMode         undo_mode,
                                            ClayUndoAccumulator *accum);
static void    clay_undo_stack_free        (ClayUndo            *undo,
                                            ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayUndoStack, clay_undo_stack, CLAY_TYPE_UNDO)

#define parent_class clay_undo_stack_parent_class


static void
clay_undo_stack_class_init (ClayUndoStackClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->finalize         = clay_undo_stack_finalize;

  clay_object_class->get_memsize = clay_undo_stack_get_memsize;

  undo_class->pop                = clay_undo_stack_pop;
  undo_class->free               = clay_undo_stack_free;
}

static void
clay_undo_stack_init (ClayUndoStack *stack)
{
  stack->undos = clay_list_new (CLAY_TYPE_UNDO, FALSE);
}

static void
clay_undo_stack_finalize (GObject *object)
{
  ClayUndoStack *stack = CLAY_UNDO_STACK (object);

  g_clear_object (&stack->undos);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_undo_stack_get_memsize (ClayObject *object,
                             gint64     *gui_size)
{
  ClayUndoStack *stack   = CLAY_UNDO_STACK (object);
  gint64         memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (stack->undos), gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_undo_stack_pop (ClayUndo            *undo,
                     ClayUndoMode         undo_mode,
                     ClayUndoAccumulator *accum)
{
  ClayUndoStack *stack = CLAY_UNDO_STACK (undo);
  GList         *list;

  for (list = CLAY_LIST (stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayUndo *child = list->data;

      clay_undo_pop (child, undo_mode, accum);
    }
}

static void
clay_undo_stack_free (ClayUndo     *undo,
                      ClayUndoMode  undo_mode)
{
  ClayUndoStack *stack = CLAY_UNDO_STACK (undo);
  GList         *list;

  for (list = CLAY_LIST (stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayUndo *child = list->data;

      clay_undo_free (child, undo_mode);
      g_object_unref (child);
    }

  clay_container_clear (stack->undos);
}

ClayUndoStack *
clay_undo_stack_new (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return g_object_new (CLAY_TYPE_UNDO_STACK,
                       "image", image,
                       NULL);
}

void
clay_undo_stack_push_undo (ClayUndoStack *stack,
                           ClayUndo      *undo)
{
  g_return_if_fail (CLAY_IS_UNDO_STACK (stack));
  g_return_if_fail (CLAY_IS_UNDO (undo));

  clay_container_add (stack->undos, CLAY_OBJECT (undo));
}

ClayUndo *
clay_undo_stack_pop_undo (ClayUndoStack       *stack,
                          ClayUndoMode         undo_mode,
                          ClayUndoAccumulator *accum)
{
  ClayUndo *undo;

  g_return_val_if_fail (CLAY_IS_UNDO_STACK (stack), NULL);
  g_return_val_if_fail (accum != NULL, NULL);

  undo = CLAY_UNDO (clay_container_get_first_child (stack->undos));

  if (undo)
    {
      clay_container_remove (stack->undos, CLAY_OBJECT (undo));
      clay_undo_pop (undo, undo_mode, accum);

      return undo;
    }

  return NULL;
}

ClayUndo *
clay_undo_stack_free_bottom (ClayUndoStack *stack,
                             ClayUndoMode   undo_mode)
{
  ClayUndo *undo;

  g_return_val_if_fail (CLAY_IS_UNDO_STACK (stack), NULL);

  undo = CLAY_UNDO (clay_container_get_last_child (stack->undos));

  if (undo)
    {
      clay_container_remove (stack->undos, CLAY_OBJECT (undo));
      clay_undo_free (undo, undo_mode);

      return undo;
    }

  return NULL;
}

ClayUndo *
clay_undo_stack_peek (ClayUndoStack *stack)
{
  g_return_val_if_fail (CLAY_IS_UNDO_STACK (stack), NULL);

  return CLAY_UNDO (clay_container_get_first_child (stack->undos));
}

gint
clay_undo_stack_get_depth (ClayUndoStack *stack)
{
  g_return_val_if_fail (CLAY_IS_UNDO_STACK (stack), 0);

  return clay_container_get_n_children (stack->undos);
}
