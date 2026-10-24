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

#include "config/claycoreconfig.h"

#include "clay.h"
#include "clay-utils.h"
#include "clayimage.h"
#include "clayimage-private.h"
#include "clayimage-undo.h"
#include "clayitem.h"
#include "claylist.h"
#include "clayundostack.h"


/*  local function prototypes  */

static void          clay_image_undo_pop_stack       (ClayImage     *image,
                                                      ClayUndoStack *undo_stack,
                                                      ClayUndoStack *redo_stack,
                                                      ClayUndoMode   undo_mode);
static void          clay_image_undo_free_space      (ClayImage     *image);
static void          clay_image_undo_free_redo       (ClayImage     *image);

static ClayDirtyMask clay_image_undo_dirty_from_type (ClayUndoType   undo_type);


/*  public functions  */

gboolean
clay_image_undo_is_enabled (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return (CLAY_IMAGE_GET_PRIVATE (image)->undo_freeze_count == 0);
}

gboolean
clay_image_undo_enable (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  Free all undo steps as they are now invalidated  */
  clay_image_undo_free (image);

  /*  Match file_open_sanitize_image(): nested freeze without matching
   *  thaw leaves undo recording off and the history dock empty.
   */
  while (private->undo_freeze_count > 0)
    clay_image_undo_thaw (image);

  return TRUE;
}

gboolean
clay_image_undo_disable (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return clay_image_undo_freeze (image);
}

gboolean
clay_image_undo_freeze (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->undo_freeze_count++;

  if (private->undo_freeze_count == 1)
    clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_FREEZE, NULL);

  return TRUE;
}

gboolean
clay_image_undo_thaw (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->undo_freeze_count > 0, FALSE);

  private->undo_freeze_count--;

  if (private->undo_freeze_count == 0)
    clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_THAW, NULL);

  return TRUE;
}

gboolean
clay_image_undo (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == CLAY_UNDO_GROUP_NONE,
                        FALSE);

  clay_image_undo_pop_stack (image,
                             private->undo_stack,
                             private->redo_stack,
                             CLAY_UNDO_MODE_UNDO);

  return TRUE;
}

gboolean
clay_image_redo (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == CLAY_UNDO_GROUP_NONE,
                        FALSE);

  clay_image_undo_pop_stack (image,
                             private->redo_stack,
                             private->undo_stack,
                             CLAY_UNDO_MODE_REDO);

  return TRUE;
}

/*
 * this function continues to undo as long as it only sees certain
 * undo types, in particular visibility changes.
 */
gboolean
clay_image_strong_undo (ClayImage *image)
{
  ClayImagePrivate *private;
  ClayUndo         *undo;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == CLAY_UNDO_GROUP_NONE,
                        FALSE);

  undo = clay_undo_stack_peek (private->undo_stack);

  clay_image_undo (image);

  while (clay_undo_is_weak (undo))
    {
      undo = clay_undo_stack_peek (private->undo_stack);
      if (clay_undo_is_weak (undo))
        clay_image_undo (image);
    }

  return TRUE;
}

/*
 * this function continues to redo as long as it only sees certain
 * undo types, in particular visibility changes.  Note that the
 * order of events is set up to make it exactly reverse
 * clay_image_strong_undo().
 */
gboolean
clay_image_strong_redo (ClayImage *image)
{
  ClayImagePrivate *private;
  ClayUndo         *undo;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == CLAY_UNDO_GROUP_NONE,
                        FALSE);

  undo = clay_undo_stack_peek (private->redo_stack);

  clay_image_redo (image);

  while (clay_undo_is_weak (undo))
    {
      undo = clay_undo_stack_peek (private->redo_stack);
      if (clay_undo_is_weak (undo))
        clay_image_redo (image);
    }

  return TRUE;
}

ClayUndoStack *
clay_image_get_undo_stack (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->undo_stack;
}

ClayUndoStack *
clay_image_get_redo_stack (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->redo_stack;
}

void
clay_image_undo_free (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  Emit the UNDO_FREE event before actually freeing everything
   *  so the views can properly detach from the undo items
   */
  clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_FREE, NULL);

  clay_undo_free (CLAY_UNDO (private->undo_stack), CLAY_UNDO_MODE_UNDO);
  clay_undo_free (CLAY_UNDO (private->redo_stack), CLAY_UNDO_MODE_REDO);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.
   */
  if (private->dirty < 0)
    private->dirty = 100000;

  /* The same applies to the case where the image would become clean
   * due to undo actions, but since user can't undo without an undo
   * stack, that's not so much a problem.
   */
}

gint
clay_image_get_undo_group_count (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), 0);

  return CLAY_IMAGE_GET_PRIVATE (image)->group_count;
}

gboolean
clay_image_undo_group_start (ClayImage    *image,
                             ClayUndoType  undo_type,
                             const gchar  *name)
{
  ClayImagePrivate *private;
  ClayUndoStack    *undo_group;
  ClayDirtyMask     dirty_mask;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (undo_type >  CLAY_UNDO_GROUP_FIRST &&
                        undo_type <= CLAY_UNDO_GROUP_LAST, FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (! name)
    name = clay_undo_type_to_name (undo_type);

  dirty_mask = clay_image_undo_dirty_from_type (undo_type);

  /* Notify listeners that the image will be modified */
  if (private->group_count == 0 && dirty_mask != CLAY_DIRTY_NONE)
    clay_image_dirty (image, dirty_mask);

  if (private->undo_freeze_count > 0)
    return FALSE;

  private->group_count++;

  /*  If we're already in a group...ignore  */
  if (private->group_count > 1)
    return TRUE;

  /*  nuke the redo stack  */
  clay_image_undo_free_redo (image);

  undo_group = clay_undo_stack_new (image);

  clay_object_set_name (CLAY_OBJECT (undo_group), name);
  CLAY_UNDO (undo_group)->undo_type  = undo_type;
  CLAY_UNDO (undo_group)->dirty_mask = dirty_mask;

  clay_undo_stack_push_undo (private->undo_stack, CLAY_UNDO (undo_group));

  private->pushing_undo_group = undo_type;

  return TRUE;
}

gboolean
clay_image_undo_group_end (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->undo_freeze_count > 0)
    return FALSE;

  g_return_val_if_fail (private->group_count > 0, FALSE);

  private->group_count--;

  if (private->group_count == 0)
    {
      private->pushing_undo_group = CLAY_UNDO_GROUP_NONE;

      /* Do it here, since undo_push doesn't emit this event while in
       * the middle of a group
       */
      clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_PUSHED,
                             clay_undo_stack_peek (private->undo_stack));

      clay_image_undo_free_space (image);
    }

  return TRUE;
}

ClayUndo *
clay_image_undo_push (ClayImage     *image,
                      GType          object_type,
                      ClayUndoType   undo_type,
                      const gchar   *name,
                      ClayDirtyMask  dirty_mask,
                      ...)
{
  ClayImagePrivate  *private;
  gint               n_properties = 0;
  gchar            **names        = NULL;
  GValue            *values       = NULL;
  va_list            args;
  ClayUndo          *undo;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (object_type, CLAY_TYPE_UNDO), NULL);
  g_return_val_if_fail (undo_type > CLAY_UNDO_GROUP_LAST, NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /* Does this undo dirty the image?  If so, we always want to mark
   * image dirty, even if we can't actually push the undo.
   */
  if (dirty_mask != CLAY_DIRTY_NONE)
    clay_image_dirty (image, dirty_mask);

  if (private->undo_freeze_count > 0)
    return NULL;

  if (! name)
    name = clay_undo_type_to_name (undo_type);

  names = clay_properties_append (object_type,
                                  &n_properties, names, &values,
                                  "name",       name,
                                  "image",      image,
                                  "undo-type",  undo_type,
                                  "dirty-mask", dirty_mask,
                                  NULL);

  va_start (args, dirty_mask);
  names = clay_properties_append_valist (object_type,
                                         &n_properties, names, &values,
                                         args);
  va_end (args);

  undo = (ClayUndo *) g_object_new_with_properties (object_type,
                                                    n_properties,
                                                    (const gchar **) names,
                                                    (const GValue *) values);

  clay_properties_free (n_properties, names, values);

  /*  nuke the redo stack  */
  clay_image_undo_free_redo (image);

  if (private->pushing_undo_group == CLAY_UNDO_GROUP_NONE)
    {
      clay_undo_stack_push_undo (private->undo_stack, undo);

      clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_PUSHED, undo);

      clay_image_undo_free_space (image);

      /*  freeing undo space may have freed the newly pushed undo  */
      if (clay_undo_stack_peek (private->undo_stack) == undo)
        return undo;
    }
  else
    {
      ClayUndoStack *undo_group;

      undo_group = CLAY_UNDO_STACK (clay_undo_stack_peek (private->undo_stack));

      clay_undo_stack_push_undo (undo_group, undo);

      return undo;
    }

  return NULL;
}

ClayUndo *
clay_image_undo_can_compress (ClayImage    *image,
                              GType         object_type,
                              ClayUndoType  undo_type)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (clay_image_is_dirty (image) &&
      ! clay_undo_stack_peek (private->redo_stack))
    {
      ClayUndo *undo = clay_undo_stack_peek (private->undo_stack);

      if (undo && undo->undo_type == undo_type &&
          g_type_is_a (G_TYPE_FROM_INSTANCE (undo), object_type))
        {
          return undo;
        }
    }

  return NULL;
}


/*  private functions  */

static void
clay_image_undo_pop_stack (ClayImage     *image,
                           ClayUndoStack *undo_stack,
                           ClayUndoStack *redo_stack,
                           ClayUndoMode   undo_mode)
{
  ClayUndo            *undo;
  ClayUndoAccumulator  accum = { 0, };

  g_object_freeze_notify (G_OBJECT (image));

  undo = clay_undo_stack_pop_undo (undo_stack, undo_mode, &accum);

  if (undo)
    {
      if (CLAY_IS_UNDO_STACK (undo))
        clay_list_reverse (CLAY_LIST (CLAY_UNDO_STACK (undo)->undos));

      clay_undo_stack_push_undo (redo_stack, undo);

      if (accum.mode_changed)
        clay_image_mode_changed (image);

      if (accum.precision_changed)
        clay_image_precision_changed (image);

      if (accum.size_changed)
        clay_image_size_changed_detailed (image,
                                          accum.previous_origin_x,
                                          accum.previous_origin_y,
                                          accum.previous_width,
                                          accum.previous_height);

      if (accum.resolution_changed)
        clay_image_resolution_changed (image);

      if (accum.unit_changed)
        clay_image_unit_changed (image);

      /* let others know that we just popped an action */
      clay_image_undo_event (image,
                             (undo_mode == CLAY_UNDO_MODE_UNDO) ?
                             CLAY_UNDO_EVENT_UNDO : CLAY_UNDO_EVENT_REDO,
                             undo);
    }

  g_object_thaw_notify (G_OBJECT (image));
}

static void
clay_image_undo_free_space (ClayImage *image)
{
  ClayImagePrivate *private = CLAY_IMAGE_GET_PRIVATE (image);
  ClayContainer    *container;
  gint              min_undo_levels;
  gint              max_undo_levels;
  gint64            undo_size;

  container = private->undo_stack->undos;

  min_undo_levels = image->clay->config->levels_of_undo;
  max_undo_levels = 1024; /* FIXME */
  undo_size       = image->clay->config->undo_size;

#ifdef DEBUG_IMAGE_UNDO
  g_printerr ("undo_steps: %d    undo_bytes: %ld\n",
              clay_container_get_n_children (container),
              (glong) clay_object_get_memsize (CLAY_OBJECT (container), NULL));
#endif

  /*  keep at least min_undo_levels undo steps  */
  if (clay_container_get_n_children (container) <= min_undo_levels)
    return;

  while ((clay_object_get_memsize (CLAY_OBJECT (container), NULL) > undo_size) ||
         (clay_container_get_n_children (container) > max_undo_levels))
    {
      ClayUndo *freed = clay_undo_stack_free_bottom (private->undo_stack,
                                                     CLAY_UNDO_MODE_UNDO);

#ifdef DEBUG_IMAGE_UNDO
      g_printerr ("freed one step: undo_steps: %d    undo_bytes: %ld\n",
                  clay_container_get_n_children (container),
                  (glong) clay_object_get_memsize (CLAY_OBJECT (container),
                                                   NULL));
#endif

      clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_EXPIRED, freed);

      g_object_unref (freed);

      if (clay_container_get_n_children (container) <= min_undo_levels)
        return;
    }
}

static void
clay_image_undo_free_redo (ClayImage *image)
{
  ClayImagePrivate *private   = CLAY_IMAGE_GET_PRIVATE (image);
  ClayContainer    *container = private->redo_stack->undos;

#ifdef DEBUG_IMAGE_UNDO
  g_printerr ("redo_steps: %d    redo_bytes: %ld\n",
              clay_container_get_n_children (container),
              (glong) clay_object_get_memsize (CLAY_OBJECT (container), NULL));
#endif

  if (clay_container_is_empty (container))
    return;

  while (clay_container_get_n_children (container) > 0)
    {
      ClayUndo *freed = clay_undo_stack_free_bottom (private->redo_stack,
                                                     CLAY_UNDO_MODE_REDO);

#ifdef DEBUG_IMAGE_UNDO
      g_printerr ("freed one step: redo_steps: %d    redo_bytes: %ld\n",
                  clay_container_get_n_children (container),
                  (glong )clay_object_get_memsize (CLAY_OBJECT (container),
                                                   NULL));
#endif

      clay_image_undo_event (image, CLAY_UNDO_EVENT_REDO_EXPIRED, freed);

      g_object_unref (freed);
    }

  /* We need to use <= here because the undo counter has already been
   * incremented at this point.
   */
  if (private->dirty <= 0)
    {
      /* If the image was dirty, but could become clean by redo-ing
       * some actions, then it should now become 'infinitely' dirty.
       * This is because we've just nuked the actions that would allow
       * the image to become clean again.
       */
      private->dirty = 100000;
    }
}

static ClayDirtyMask
clay_image_undo_dirty_from_type (ClayUndoType undo_type)
{
  switch (undo_type)
    {
    case CLAY_UNDO_GROUP_IMAGE_SCALE:
    case CLAY_UNDO_GROUP_IMAGE_RESIZE:
    case CLAY_UNDO_GROUP_IMAGE_FLIP:
    case CLAY_UNDO_GROUP_IMAGE_ROTATE:
    case CLAY_UNDO_GROUP_IMAGE_TRANSFORM:
    case CLAY_UNDO_GROUP_IMAGE_CROP:
      return CLAY_DIRTY_IMAGE | CLAY_DIRTY_IMAGE_SIZE;

    case CLAY_UNDO_GROUP_IMAGE_CONVERT:
      return CLAY_DIRTY_IMAGE | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_IMAGE_COLORMAP_REMAP:
      return CLAY_DIRTY_IMAGE | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE:
      return CLAY_DIRTY_IMAGE_STRUCTURE | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_IMAGE_PATHS_MERGE:
      return CLAY_DIRTY_IMAGE_STRUCTURE | CLAY_DIRTY_PATH;

    case CLAY_UNDO_GROUP_IMAGE_QUICK_MASK: /* FIXME */
      return CLAY_DIRTY_IMAGE_STRUCTURE | CLAY_DIRTY_SELECTION;

    case CLAY_UNDO_GROUP_IMAGE_GRID:
    case CLAY_UNDO_GROUP_GUIDE:
      return CLAY_DIRTY_IMAGE_META;

    case CLAY_UNDO_GROUP_DRAWABLE:
    case CLAY_UNDO_GROUP_DRAWABLE_MOD:
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_MASK: /* FIXME */
      return CLAY_DIRTY_SELECTION;

    case CLAY_UNDO_GROUP_ITEM_VISIBILITY:
    case CLAY_UNDO_GROUP_ITEM_PROPERTIES:
      return CLAY_DIRTY_ITEM_META;

    case CLAY_UNDO_GROUP_ITEM_DISPLACE: /* FIXME */
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE | CLAY_DIRTY_PATH;

    case CLAY_UNDO_GROUP_ITEM_SCALE: /* FIXME */
    case CLAY_UNDO_GROUP_ITEM_RESIZE: /* FIXME */
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE | CLAY_DIRTY_PATH;

    case CLAY_UNDO_GROUP_LAYER_ADD_MASK:
    case CLAY_UNDO_GROUP_LAYER_APPLY_MASK:
      return CLAY_DIRTY_IMAGE_STRUCTURE;

    case CLAY_UNDO_GROUP_FS_TO_LAYER:
    case CLAY_UNDO_GROUP_FS_FLOAT:
    case CLAY_UNDO_GROUP_FS_ANCHOR:
      return CLAY_DIRTY_IMAGE_STRUCTURE;

    case CLAY_UNDO_GROUP_EDIT_PASTE:
      return CLAY_DIRTY_IMAGE_STRUCTURE;

    case CLAY_UNDO_GROUP_EDIT_CUT:
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_TEXT:
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_TRANSFORM: /* FIXME */
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE | CLAY_DIRTY_PATH;

    case CLAY_UNDO_GROUP_PAINT:
      return CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_PARASITE_ATTACH:
    case CLAY_UNDO_GROUP_PARASITE_REMOVE:
      return CLAY_DIRTY_IMAGE_META | CLAY_DIRTY_ITEM_META;

    case CLAY_UNDO_GROUP_PATHS_IMPORT:
      return CLAY_DIRTY_IMAGE_STRUCTURE | CLAY_DIRTY_PATH;

    case CLAY_UNDO_GROUP_FILTER_VISIBILITY:
      return CLAY_DIRTY_DRAWABLE;

    case CLAY_UNDO_GROUP_MISC:
      return CLAY_DIRTY_ALL;

    default:
      break;
    }

  return CLAY_DIRTY_ALL;
}
