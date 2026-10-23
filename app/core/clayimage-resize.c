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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-guides.h"
#include "clayimage-item-list.h"
#include "clayimage-resize.h"
#include "clayimage-sample-points.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"
#include "claysamplepoint.h"

#include "text/claytextlayer.h"

#include "path/clayvectorlayer.h"

#include "clay-intl.h"


void
clay_image_resize (ClayImage    *image,
                   ClayContext  *context,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y,
                   ClayProgress *progress)
{
  clay_image_resize_with_layers (image, context, CLAY_FILL_TRANSPARENT,
                                 new_width, new_height, offset_x, offset_y,
                                 CLAY_ITEM_SET_NONE, TRUE,
                                 progress);
}

void
clay_image_resize_with_layers (ClayImage    *image,
                               ClayContext  *context,
                               ClayFillType  fill_type,
                               gint          new_width,
                               gint          new_height,
                               gint          offset_x,
                               gint          offset_y,
                               ClayItemSet   layer_set,
                               gboolean      resize_text_layers,
                               ClayProgress *progress)
{
  ClayObjectQueue *queue;
  GList           *resize_layers;
  GList           *list;
  ClayItem        *item;
  gint             old_width, old_height;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (new_width > 0 && new_height > 0);
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  clay_set_busy (image->clay);

  g_object_freeze_notify (G_OBJECT (image));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_RESIZE,
                               C_("undo-type", "Resize Image"));

  resize_layers = clay_image_item_list_get_list (image,
                                                 CLAY_ITEM_TYPE_LAYERS,
                                                 layer_set);

  old_width  = clay_image_get_width  (image);
  old_height = clay_image_get_height (image);

  /*  Push the image size to the stack  */
  clay_image_undo_push_image_size (image, NULL,
                                   -offset_x, -offset_y,
                                   new_width, new_height);

  /*  Set the new width and height  */
  g_object_set (image,
                "width",  new_width,
                "height", new_height,
                NULL);

  /*  Reposition all layers  */
  for (list = clay_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      ClayItem *item = list->data;

      if (clay_item_is_vector_layer (item))
        continue;

      clay_item_translate (item, offset_x, offset_y, TRUE);
    }

  queue    = clay_object_queue_new (progress);
  progress = CLAY_PROGRESS (queue);

  for (list = resize_layers; list; list = g_list_next (list))
    {
      ClayItem *item = list->data;

      /*  group layers can't be resized here  */
      if (clay_viewable_get_children (CLAY_VIEWABLE (item)))
        continue;

      if (! resize_text_layers && clay_item_is_text_layer (item))
        continue;

      /* note that we call clay_item_start_move(), and not
       * clay_item_start_transform().  see the comment in clay_item_resize()
       * for more information.
       */
      clay_item_start_move (item, TRUE);

      clay_object_queue_push (queue, item);
    }

  g_list_free (resize_layers);

  clay_object_queue_push (queue, clay_image_get_mask (image));
  clay_object_queue_push_container (queue, clay_image_get_channels (image));
  clay_object_queue_push_container (queue, clay_image_get_paths (image));

  /*  Resize all resize_layers, channels (including selection mask), and
   *  paths
   */
  while ((item = clay_object_queue_pop (queue)))
    {
      if (CLAY_IS_LAYER (item))
        {
          gint old_offset_x;
          gint old_offset_y;

          if (clay_item_is_vector_layer (item))
            continue;

          clay_item_get_offset (item, &old_offset_x, &old_offset_y);

          clay_item_resize (item, context, fill_type,
                            new_width, new_height,
                            old_offset_x, old_offset_y);

          clay_item_end_move (item, TRUE);
        }
      else
        {
          clay_item_resize (item, context, CLAY_FILL_TRANSPARENT,
                            new_width, new_height, offset_x, offset_y);
        }
    }

  /*  Reposition or remove all guides  */
  list = clay_image_get_guides (image);

  while (list)
    {
      ClayGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       new_position = clay_guide_get_position (guide);

      list = g_list_next (list);

      switch (clay_guide_get_orientation (guide))
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          new_position += offset_y;
          if (new_position < 0 || new_position > new_height)
            remove_guide = TRUE;
          break;

        case CLAY_ORIENTATION_VERTICAL:
          new_position += offset_x;
          if (new_position < 0 || new_position > new_width)
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        clay_image_remove_guide (image, guide, TRUE);
      else if (new_position != clay_guide_get_position (guide))
        clay_image_move_guide (image, guide, new_position, TRUE);
    }

  /*  Reposition or remove sample points  */
  list = clay_image_get_sample_points (image);

  while (list)
    {
      ClaySamplePoint *sample_point        = list->data;
      gboolean         remove_sample_point = FALSE;
      gint             old_x;
      gint             old_y;
      gint             new_x;
      gint             new_y;

      list = g_list_next (list);

      clay_sample_point_get_position (sample_point, &old_x, &old_y);

      new_y = old_y + offset_y;
      if ((new_y < 0) || (new_y >= new_height))
        remove_sample_point = TRUE;

      new_x = old_x + offset_x;
      if ((new_x < 0) || (new_x >= new_width))
        remove_sample_point = TRUE;

      if (remove_sample_point)
        clay_image_remove_sample_point (image, sample_point, TRUE);
      else if (new_x != old_x || new_y != old_y)
        clay_image_move_sample_point (image, sample_point,
                                      new_x, new_y, TRUE);
    }

  g_object_unref (queue);

  clay_image_undo_group_end (image);

  clay_image_size_changed_detailed (image,
                                    offset_x, offset_y,
                                    old_width, old_height);

  g_object_thaw_notify (G_OBJECT (image));

  clay_unset_busy (image->clay);
}

void
clay_image_resize_to_layers (ClayImage    *image,
                             ClayContext  *context,
                             gint         *offset_x,
                             gint         *offset_y,
                             gint         *new_width,
                             gint         *new_height,
                             ClayProgress *progress)
{
  GList    *list;
  ClayItem *item;
  gint      x, y;
  gint      width, height;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  list = clay_image_get_layer_iter (image);

  if (! list)
    return;

  /* figure out starting values */
  item = list->data;

  x      = clay_item_get_offset_x (item);
  y      = clay_item_get_offset_y (item);
  width  = clay_item_get_width  (item);
  height = clay_item_get_height (item);

  /*  Respect all layers  */
  for (list = g_list_next (list); list; list = g_list_next (list))
    {
      item = list->data;

      clay_rectangle_union (x, y,
                            width, height,
                            clay_item_get_offset_x (item),
                            clay_item_get_offset_y (item),
                            clay_item_get_width  (item),
                            clay_item_get_height (item),
                            &x, &y,
                            &width, &height);
    }

  clay_image_resize (image, context,
                     width, height, -x, -y,
                     progress);
  if (offset_x)
    *offset_x = -x;
  if (offset_y)
    *offset_y = -y;
  if (new_width)
    *new_width = width;
  if (new_height)
    *new_height = height;
}

void
clay_image_resize_to_selection (ClayImage    *image,
                                ClayContext  *context,
                                ClayProgress *progress)
{
  ClayChannel *selection = clay_image_get_mask (image);
  gint         x, y, w, h;

  if (clay_item_bounds (CLAY_ITEM (selection), &x, &y, &w, &h))
    {
      clay_image_resize (image, context,
                         w, h, -x, -y,
                         progress);
    }
}
