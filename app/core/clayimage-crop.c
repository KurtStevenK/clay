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

#include "path/clayvectorlayer.h"

#include "clay.h"
#include "claycontext.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-crop.h"
#include "clayimage-guides.h"
#include "clayimage-sample-points.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer.h"
#include "clayrasterizable.h"
#include "claysamplepoint.h"

#include "clay-intl.h"


/*  public functions  */

void
clay_image_crop (ClayImage    *image,
                 ClayContext  *context,
                 ClayFillType  fill_type,
                 gint          x,
                 gint          y,
                 gint          width,
                 gint          height,
                 gboolean      crop_layers)
{
  GList *list;
  gint   previous_width;
  gint   previous_height;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  previous_width  = clay_image_get_width  (image);
  previous_height = clay_image_get_height (image);

  /*  Make sure new width and height are non-zero  */
  if (width < 1 || height < 1)
    return;

  clay_set_busy (image->clay);

  g_object_freeze_notify (G_OBJECT (image));

  if (crop_layers)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_CROP,
                                 C_("undo-type", "Crop Image"));
  else
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_RESIZE,
                                 C_("undo-type", "Resize Image"));

  /*  Push the image size to the stack  */
  clay_image_undo_push_image_size (image, NULL,
                                   x, y, width, height);

  /*  Set the new width and height  */
  g_object_set (image,
                "width",  width,
                "height", height,
                NULL);

  /*  Resize all channels  */
  for (list = clay_image_get_channel_iter (image);
       list;
       list = g_list_next (list))
    {
      ClayItem *item = list->data;

      clay_item_resize (item, context, CLAY_FILL_TRANSPARENT,
                        width, height, -x, -y);
    }

  /*  Resize all paths  */
  for (list = clay_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      ClayItem *item = list->data;

      clay_item_resize (item, context, CLAY_FILL_TRANSPARENT,
                        width, height, -x, -y);
    }

  /*  Don't forget the selection mask!  */
  clay_item_resize (CLAY_ITEM (clay_image_get_mask (image)),
                    context, CLAY_FILL_TRANSPARENT,
                    width, height, -x, -y);

  /*  crop all layers  */
  list = clay_image_get_layer_iter (image);

  while (list)
    {
      ClayItem *item = list->data;

      list = g_list_next (list);

      /* Vector layers will implicitly sync to their path (see above)
       * so ignore them entirely unless rasterized.
       */
      if (clay_item_is_vector_layer (item) &&
          ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (item)))
        continue;

      clay_item_translate (item, -x, -y, TRUE);

      /* other non-raster types (e.g. text) get moved only & skip the rest */
      if (CLAY_IS_RASTERIZABLE (item) &&
          ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (item)))
        continue;

      if (crop_layers && ! clay_item_is_content_locked (item, NULL))
        {
          gint off_x, off_y;
          gint lx1, ly1, lx2, ly2;

          clay_item_get_offset (item, &off_x, &off_y);

          lx1 = CLAMP (off_x, 0, clay_image_get_width  (image));
          ly1 = CLAMP (off_y, 0, clay_image_get_height (image));
          lx2 = CLAMP (clay_item_get_width  (item) + off_x,
                       0, clay_image_get_width (image));
          ly2 = CLAMP (clay_item_get_height (item) + off_y,
                       0, clay_image_get_height (image));

          width  = lx2 - lx1;
          height = ly2 - ly1;

          clay_drawable_enable_resize_undo (CLAY_DRAWABLE (item));

          if (width > 0 && height > 0)
            {
              clay_item_resize (item, context, fill_type,
                                width, height,
                                -(lx1 - off_x),
                                -(ly1 - off_y));
            }
          else
            {
              clay_image_remove_layer (image, CLAY_LAYER (item),
                                       TRUE, NULL);
            }
        }
    }

  /*  Reposition or remove guides  */
  list = clay_image_get_guides (image);

  while (list)
    {
      ClayGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       position     = clay_guide_get_position (guide);

      list = g_list_next (list);

      switch (clay_guide_get_orientation (guide))
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          position -= y;
          if ((position < 0) || (position > height))
            remove_guide = TRUE;
          break;

        case CLAY_ORIENTATION_VERTICAL:
          position -= x;
          if ((position < 0) || (position > width))
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        clay_image_remove_guide (image, guide, TRUE);
      else if (position != clay_guide_get_position (guide))
        clay_image_move_guide (image, guide, position, TRUE);
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
      new_x = old_x;
      new_y = old_y;

      new_y -= y;
      if ((new_y < 0) || (new_y > height))
       remove_sample_point = TRUE;

      new_x -= x;
      if ((new_x < 0) || (new_x > width))
        remove_sample_point = TRUE;

      if (remove_sample_point)
        clay_image_remove_sample_point (image, sample_point, TRUE);
      else if (new_x != old_x || new_y != old_y)
        clay_image_move_sample_point (image, sample_point,
                                      new_x, new_y, TRUE);
    }

  clay_image_undo_group_end (image);

  clay_image_size_changed_detailed (image,
                                    -x, -y,
                                    previous_width, previous_height);

  g_object_thaw_notify (G_OBJECT (image));

  clay_unset_busy (image->clay);
}
