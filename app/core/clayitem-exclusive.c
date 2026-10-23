/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitem-exclusive.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#include "claycontext.h"
#include "claydrawable.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitem.h"
#include "clayitem-exclusive.h"
#include "clayitemstack.h"
#include "clayitemtree.h"
#include "clayundostack.h"

#include "clay-intl.h"


static GList * clay_item_exclusive_get_ancestry (ClayItem              *item);
static void    clay_item_exclusive_get_lists    (ClayItem              *item,
                                                 ClayItemIsEnabledFunc  is_enabled,
                                                 ClayItemCanSetFunc     can_set,
                                                 ClayItemIsPropLocked   is_prop_locked,
                                                 gboolean               only_selected,
                                                 GList                **on,
                                                 GList                **off);


/*  public functions  */

void
clay_item_toggle_exclusive_visible (ClayItem    *item,
                                    gboolean     only_selected,
                                    ClayContext *context)
{
  clay_item_toggle_exclusive (item,
                              (ClayItemIsEnabledFunc) clay_item_is_visible,
                              (ClayItemSetFunc)       clay_item_set_visible,
                              NULL,
                              (ClayItemIsPropLocked)  clay_item_get_lock_visibility,
                              (ClayItemUndoPush)      clay_image_undo_push_item_visibility,
                              _("Set Item Exclusive Visibility"),
                              CLAY_UNDO_GROUP_ITEM_VISIBILITY,
                              only_selected, context);
}

void
clay_item_toggle_exclusive (ClayItem               *item,
                            ClayItemIsEnabledFunc   is_enabled,
                            ClayItemSetFunc         set_prop,
                            ClayItemCanSetFunc      can_set,
                            ClayItemIsPropLocked    is_prop_locked,
                            ClayItemUndoPush        undo_push,
                            const gchar            *undo_desc,
                            ClayUndoType            group_undo_type,
                            gboolean                only_selected,
                            ClayContext            *context)
{
  GList *ancestry;
  GList *on;
  GList *off;
  GList *list;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_is_attached (item));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  ancestry = clay_item_exclusive_get_ancestry (item);
  clay_item_exclusive_get_lists (item, is_enabled, can_set, is_prop_locked,
                                 only_selected, &on, &off);

  if (on || off || (! is_enabled (item) && (can_set == NULL || can_set (item))))
    {
      ClayImage *image = clay_item_get_image (item);
      ClayUndo  *undo;
      gboolean   push_undo = TRUE;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_UNDO_STACK,
                                           group_undo_type);

      /* Use the undo description as the undo object key, as we should
       * assume that a same undo description means the same exclusive
       * action.
       */
      if (undo && (g_object_get_data (G_OBJECT (undo), undo_desc) ==
                   (gpointer) item))
        push_undo = FALSE;

      if (push_undo)
        {
          if (clay_image_undo_group_start (image,
                                           group_undo_type,
                                           undo_desc))
            {
              undo = clay_image_undo_can_compress (image, CLAY_TYPE_UNDO_STACK,
                                                   group_undo_type);

              if (undo)
                g_object_set_data (G_OBJECT (undo), undo_desc, (gpointer) item);
            }

          for (list = ancestry; list; list = g_list_next (list))
            undo_push (image, NULL, list->data);

          for (list = on; list; list = g_list_next (list))
            undo_push (image, NULL, list->data);

          for (list = off; list; list = g_list_next (list))
            undo_push (image, NULL, list->data);

          clay_image_undo_group_end (image);
        }
      else if (context)
        {
          clay_undo_refresh_preview (undo, context);
        }

      for (list = ancestry; list; list = g_list_next (list))
        set_prop (list->data, TRUE, FALSE);

      if (on)
        {
          for (list = on; list; list = g_list_next (list))
            set_prop (list->data, FALSE, FALSE);
        }
      else if (off)
        {
          for (list = off; list; list = g_list_next (list))
            set_prop (list->data, TRUE, FALSE);
        }

      g_list_free (on);
      g_list_free (off);
    }

  g_list_free (ancestry);
}


/*  private functions  */

static GList *
clay_item_exclusive_get_ancestry (ClayItem *item)
{
  ClayViewable *parent;
  GList        *ancestry = NULL;

  for (parent = CLAY_VIEWABLE (item);
       parent;
       parent = clay_viewable_get_parent (parent))
    {
      ancestry = g_list_prepend (ancestry, parent);
    }

  return ancestry;
}

static void
clay_item_exclusive_get_lists (ClayItem              *item,
                               ClayItemIsEnabledFunc  is_enabled,
                               ClayItemCanSetFunc     can_set,
                               ClayItemIsPropLocked   is_prop_locked,
                               gboolean               only_selected,
                               GList                 **on,
                               GList                 **off)
{
  ClayImage    *image = NULL;
  ClayItemTree *tree;
  GList        *items;
  GList        *list;

  *on  = NULL;
  *off = NULL;

  tree = clay_item_get_tree (item);

  items = clay_item_stack_get_item_list (CLAY_ITEM_STACK (tree->container));

  if (only_selected)
    image = clay_item_get_image (item);

  for (list = items; list; list = g_list_next (list))
    {
      ClayItem *other = list->data;

      if (other != item                                                                &&
          /* Don't include item with visibility locks. */
          (is_prop_locked == NULL || ! is_prop_locked (other))                         &&
          /* Don't include item which can be changed. */
          (can_set == NULL || can_set (other))                                         &&
          /* Do we care only about selected drawables? */
          (! only_selected  || clay_image_is_selected_drawable (image,
                                                                CLAY_DRAWABLE (other))) &&
          /* We are only interested in same level items unless
           * @only_selected is TRUE. */
          (only_selected ||
           clay_viewable_get_parent (CLAY_VIEWABLE (other)) ==
           clay_viewable_get_parent (CLAY_VIEWABLE (item))))
        {
          if (is_enabled (other))
            *on = g_list_prepend (*on, other);
          else
            *off = g_list_prepend (*off, other);
        }
    }

  g_list_free (items);
}
