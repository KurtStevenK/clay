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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claydrawable-equalize.h"
#include "core/claydrawable-levels.h"
#include "core/claydrawable-operation.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayitemundo.h"
#include "core/claylayermask.h"
#include "core/clayprogress.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "drawable-commands.h"

#include "clay-intl.h"


/*  public functions  */

void
drawable_equalize_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage *image;
  GList     *drawables;
  GList     *iter;

  return_if_no_drawables (image, drawables, data);

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_start (image,
                                 CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Equalize"));

  for (iter = drawables; iter; iter = iter->next)
    clay_drawable_equalize (iter->data, TRUE);

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}

void
drawable_levels_stretch_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage    *image;
  GList        *drawables;
  GList        *iter;
  ClayDisplay  *display;
  GtkWidget    *widget;

  return_if_no_drawables (image, drawables, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  for (iter = drawables; iter; iter = iter->next)
    {
      if (! clay_drawable_is_rgb (iter->data))
        {
          clay_message_literal (image->clay,
                                G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                                _("White Balance operates only on RGB color "
                                  "layers."));
          return;
        }
    }

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_start (image,
                                 CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Levels"));

  for (iter = drawables; iter; iter = iter->next)
    clay_drawable_levels_stretch (iter->data, CLAY_PROGRESS (display));

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}

void
drawable_visible_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage    *image;
  GList        *drawables;
  GList        *iter;
  ClayUndo     *undo;
  gboolean      push_undo = TRUE;
  gboolean      visible;

  return_if_no_drawables (image, drawables, data);

  visible = g_variant_get_boolean (value);

  if (CLAY_IS_LAYER_MASK (drawables->data))
    {
      ClayLayerMask *mask = CLAY_LAYER_MASK (drawables->data);

      g_list_free (drawables);
      drawables = g_list_prepend (NULL, clay_layer_mask_get_layer (mask));
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      if (visible && clay_item_get_visible (iter->data))
        {
          /* If any of the drawables are already visible, we don't
           * toggle the selection visibility. This prevents the
           * SET_ACTIVE() in drawables-actions.c to toggle visibility
           * unexpectedly.
           */
          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    if (visible != clay_item_get_visible (iter->data))
      break;

  if (! iter)
    {
      g_list_free (drawables);
      return;
    }

  if (g_list_length (drawables) == 1)
    {
      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_ITEM_VISIBILITY);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (drawables->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_VISIBILITY,
                                   "Item visibility");
    }

  for (; iter; iter = iter->next)
    clay_item_set_visible (iter->data, visible, push_undo);

  if (g_list_length (drawables) != 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}

void
drawable_lock_content_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage *image;
  GList     *drawables;
  GList     *iter;
  gboolean   locked;
  gboolean   push_undo = TRUE;

  return_if_no_drawables (image, drawables, data);

  locked = g_variant_get_boolean (value);

  if (CLAY_IS_LAYER_MASK (drawables->data))
    {
      ClayLayerMask *mask = CLAY_LAYER_MASK (drawables->data);

      g_list_free (drawables);
      drawables = g_list_prepend (NULL, clay_layer_mask_get_layer (mask));
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      if (! locked && ! clay_item_get_lock_content (iter->data))
        {
          /* If any of the drawables are already unlocked, we don't toggle the
           * lock. This prevents the SET_ACTIVE() in drawables-actions.c to
           * toggle locks unexpectedly.
           */
          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    if (locked != clay_item_get_lock_content (iter->data))
      break;

  if (g_list_length (drawables) == 1)
    {
      ClayUndo *undo;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_ITEM_LOCK_CONTENT);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (drawables->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                                   _("Lock/Unlock content"));
    }

  for (; iter; iter = iter->next)
    clay_item_set_lock_content (iter->data, locked, push_undo);

  if (g_list_length (drawables) != 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}

void
drawable_lock_position_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  GList     *drawables;
  GList     *iter;
  gboolean   locked;
  gboolean   push_undo = TRUE;

  return_if_no_drawables (image, drawables, data);

  locked = g_variant_get_boolean (value);

  if (CLAY_IS_LAYER_MASK (drawables->data))
    {
      ClayLayerMask *mask = CLAY_LAYER_MASK (drawables->data);

      g_list_free (drawables);
      drawables = g_list_prepend (NULL, clay_layer_mask_get_layer (mask));
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      if (! locked && ! clay_item_get_lock_position (iter->data))
        {
          /* If any of the drawables are already unlocked, we don't toggle the
           * lock. This prevents the SET_ACTIVE() in drawables-actions.c to
           * toggle locks unexpectedly.
           */
          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    if (locked != clay_item_get_lock_position (iter->data))
      break;

  if (g_list_length (drawables) == 1)
    {
      ClayUndo *undo;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_ITEM_LOCK_POSITION);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (drawables->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_LOCK_POSITION,
                                   _("Lock/Unlock position"));
    }

  for (; iter; iter = iter->next)
    clay_item_set_lock_position (iter->data, locked, push_undo);

  if (g_list_length (drawables) != 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}

void
drawable_flip_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage           *image;
  GList               *drawables;
  GList               *iter;
  ClayContext         *context;
  gint                 off_x, off_y;
  gdouble              axis = 0.0;
  ClayOrientationType  orientation;

  return_if_no_drawables (image, drawables, data);
  return_if_no_context (context, data);

  orientation = (ClayOrientationType) g_variant_get_int32 (value);

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_start (image,
                                 CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Flip Drawables"));

  for (iter = drawables; iter; iter = iter->next)
    {
      ClayItem *item;

      item = CLAY_ITEM (iter->data);

      clay_item_get_offset (item, &off_x, &off_y);

      switch (orientation)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          axis = ((gdouble) off_x + (gdouble) clay_item_get_width (item) / 2.0);
          break;

        case CLAY_ORIENTATION_VERTICAL:
          axis = ((gdouble) off_y + (gdouble) clay_item_get_height (item) / 2.0);
          break;

        default:
          break;
        }

      clay_item_flip (item, context, orientation, axis,
                      clay_item_get_clip (item, FALSE));
    }

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}

void
drawable_rotate_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage        *image;
  GList            *drawables;
  GList            *iter;
  ClayContext      *context;
  ClayRotationType  rotation_type;

  return_if_no_drawables (image, drawables, data);
  return_if_no_context (context, data);

  rotation_type = (ClayRotationType) g_variant_get_int32 (value);

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_start (image,
                                 CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                 _("Rotate Drawables"));

  for (iter = drawables; iter; iter = iter->next)
    {
      ClayItem *item;
      gint      off_x, off_y;
      gdouble   center_x, center_y;

      item = CLAY_ITEM (iter->data);

      clay_item_get_offset (item, &off_x, &off_y);

      center_x = ((gdouble) off_x + (gdouble) clay_item_get_width  (item) / 2.0);
      center_y = ((gdouble) off_y + (gdouble) clay_item_get_height (item) / 2.0);

      clay_item_rotate (item, context,
                        rotation_type, center_x, center_y,
                        clay_item_get_clip (item, FALSE));
    }

  if (g_list_length (drawables) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
  g_list_free (drawables);
}
