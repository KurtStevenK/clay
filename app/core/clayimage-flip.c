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

#include "libclaymath/claymath.h"

#include "core-types.h"

#include "path/clayvectorlayer.h"

#include "clay.h"
#include "claychannel.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-flip.h"
#include "clayimage-guides.h"
#include "clayimage-sample-points.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitem.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"
#include "claysamplepoint.h"


/*  local function prototypes  */

static void    clay_image_flip_guides        (ClayImage           *image,
                                              ClayOrientationType  flip_type,
                                              gdouble              axis);
static void    clay_image_flip_sample_points (ClayImage           *image,
                                              ClayOrientationType  flip_type,
                                              gdouble              axis);


/*  private functions  */

static void
clay_image_flip_guides (ClayImage           *image,
                        ClayOrientationType  flip_type,
                        gdouble              axis)
{
  gint   width  = clay_image_get_width  (image);
  gint   height = clay_image_get_height (image);
  GList *iter;

  for (iter = clay_image_get_guides (image); iter;)
    {
      ClayGuide *guide    = iter->data;
      gint       position = clay_guide_get_position (guide);

      iter = g_list_next (iter);

      position = SIGNED_ROUND (2.0 * axis - position);

      switch (clay_guide_get_orientation (guide))
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          if (flip_type == CLAY_ORIENTATION_VERTICAL)
            {
              if (position >= 0 && position <= height)
                clay_image_move_guide (image, guide, position, TRUE);
              else
                clay_image_remove_guide (image, guide, TRUE);
            }
          break;

        case CLAY_ORIENTATION_VERTICAL:
          if (flip_type == CLAY_ORIENTATION_HORIZONTAL)
            {
              if (position >= 0 && position <= width)
                clay_image_move_guide (image, guide, position, TRUE);
              else
                clay_image_remove_guide (image, guide, TRUE);
            }
          break;

        case CLAY_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }
    }
}

static void
clay_image_flip_sample_points (ClayImage           *image,
                               ClayOrientationType  flip_type,
                               gdouble              axis)
{
  gint   width  = clay_image_get_width  (image);
  gint   height = clay_image_get_height (image);
  GList *iter;

  for (iter = clay_image_get_sample_points (image); iter;)
    {
      ClaySamplePoint *sample_point = iter->data;
      gint             x;
      gint             y;

      iter = g_list_next (iter);

      clay_sample_point_get_position (sample_point, &x, &y);

      switch (flip_type)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          x = SIGNED_ROUND (2.0 * axis - x);
          break;

        case CLAY_ORIENTATION_VERTICAL:
          y = SIGNED_ROUND (2.0 * axis - y);
          break;

        case CLAY_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }

      if (x >= 0 && x < width &&
          y >= 0 && y < height)
        {
          clay_image_move_sample_point (image, sample_point, x, y, TRUE);
        }
      else
        {
          clay_image_remove_sample_point (image, sample_point, TRUE);
        }
    }
}


/*  public functions  */

void
clay_image_flip (ClayImage           *image,
                 ClayContext         *context,
                 ClayOrientationType  flip_type,
                 ClayProgress        *progress)
{
  gdouble axis = 0.0;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  switch (flip_type)
    {
    case CLAY_ORIENTATION_HORIZONTAL:
      axis = clay_image_get_width (image) / 2.0;
      break;

    case CLAY_ORIENTATION_VERTICAL:
      axis = clay_image_get_height (image) / 2.0;
      break;

    case CLAY_ORIENTATION_UNKNOWN:
      g_return_if_reached ();
    }

  clay_image_flip_full (image, context, flip_type, axis,
                        CLAY_TRANSFORM_RESIZE_CLIP, progress);
}

void
clay_image_flip_full (ClayImage           *image,
                      ClayContext         *context,
                      ClayOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result,
                      ClayProgress        *progress)
{
  ClayObjectQueue *queue;
  ClayItem        *item;
  gint             width;
  gint             height;
  gint             offset_x = 0;
  gint             offset_y = 0;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  width  = clay_image_get_width  (image);
  height = clay_image_get_height (image);

  if (! clip_result)
    {
      switch (flip_type)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          offset_x = SIGNED_ROUND (2.0 * axis - width);
          axis     = width / 2.0;
          break;

        case CLAY_ORIENTATION_VERTICAL:
          offset_y = SIGNED_ROUND (2.0 * axis - height);
          axis     = height / 2.0;
          break;

        case CLAY_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }
    }

  clay_set_busy (image->clay);

  queue    = clay_object_queue_new (progress);
  progress = CLAY_PROGRESS (queue);

  clay_object_queue_push_container (queue, clay_image_get_layers (image));
  clay_object_queue_push (queue, clay_image_get_mask (image));
  clay_object_queue_push_container (queue, clay_image_get_channels (image));
  clay_object_queue_push_container (queue, clay_image_get_paths (image));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_FLIP, NULL);

  /*  Flip all layers, channels (including selection mask), and paths  */
  while ((item = clay_object_queue_pop (queue)))
    {
      gboolean clip = FALSE;
      
      /* Non-rasterized vector layers will be transformed when their path is */
      if (clay_item_is_vector_layer (item))
        continue;

      if (CLAY_IS_CHANNEL (item))
        clip = clip_result;

      clay_item_flip (item, context, flip_type, axis, clip);

      clay_progress_set_value (progress, 1.0);
    }

  /*  Flip all Guides  */
  clay_image_flip_guides (image, flip_type, axis);

  /*  Flip all sample points  */
  clay_image_flip_sample_points (image, flip_type, axis);

  if (offset_x || offset_y)
    {
      clay_image_undo_push_image_size (image,
                                       NULL,
                                       offset_x,
                                       offset_y,
                                       width,
                                       height);
    }

  clay_image_undo_group_end (image);

  g_object_unref (queue);

  if (offset_x || offset_y)
    {
      clay_image_size_changed_detailed (image,
                                        -offset_x,
                                        -offset_y,
                                        width,
                                        height);
    }

  clay_unset_busy (image->clay);
}
