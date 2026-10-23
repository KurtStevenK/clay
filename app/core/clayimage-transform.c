/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimage-transform.c
 * Copyright (C) 2019 Ell
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

#include "path/claypath.h"
#include "path/clayvectorlayer.h"

#include "clay.h"
#include "clay-transform-resize.h"
#include "clay-transform-utils.h"
#include "claychannel.h"
#include "claycontext.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-guides.h"
#include "clayimage-sample-points.h"
#include "clayimage-transform.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitem.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"
#include "claysamplepoint.h"


#define EPSILON 1e-6


/*  local function prototypes  */

static void    clay_image_transform_guides        (ClayImage           *image,
                                                   const ClayMatrix3   *matrix,
                                                   const GeglRectangle *old_bounds);
static void    clay_image_transform_sample_points (ClayImage           *image,
                                                   const ClayMatrix3   *matrix,
                                                   const GeglRectangle *old_bounds);


/*  private functions  */

static void
clay_image_transform_guides (ClayImage           *image,
                             const ClayMatrix3   *matrix,
                             const GeglRectangle *old_bounds)
{
  GList *iter;

  for (iter = clay_image_get_guides (image); iter;)
    {
      ClayGuide           *guide           = iter->data;
      ClayOrientationType  old_orientation = clay_guide_get_orientation (guide);
      gint                 old_position    = clay_guide_get_position (guide);
      ClayOrientationType  new_orientation;
      gint                 new_position;
      ClayVector2          vertices[2];
      gint                 n_vertices;
      ClayVector2          diff;

      iter = g_list_next (iter);

      switch (old_orientation)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          vertices[0].x = old_bounds->x;
          vertices[0].y = old_bounds->y + old_position;

          vertices[1].x = old_bounds->x + old_bounds->width / 2.0;
          vertices[1].y = old_bounds->y + old_position;
          break;

        case CLAY_ORIENTATION_VERTICAL:
          vertices[0].x = old_bounds->x + old_position;
          vertices[0].y = old_bounds->y;

          vertices[1].x = old_bounds->x + old_position;
          vertices[1].y = old_bounds->y + old_bounds->height / 2.0;
          break;

        case CLAY_ORIENTATION_UNKNOWN:
          g_return_if_reached ();
        }

      clay_transform_polygon (matrix,
                              vertices, 2, FALSE,
                              vertices, &n_vertices);

      if (n_vertices < 2)
        {
          clay_image_remove_guide (image, guide, TRUE);

          continue;
        }

      clay_vector2_sub (&diff, &vertices[1], &vertices[0]);

      if (clay_vector2_length (&diff) <= EPSILON)
        {
          clay_image_remove_guide (image, guide, TRUE);

          continue;
        }

      if (fabs (diff.x) >= fabs (diff.y))
        {
          new_orientation = CLAY_ORIENTATION_HORIZONTAL;
          new_position    = SIGNED_ROUND (vertices[1].y);

          if (new_position < 0 || new_position > clay_image_get_height (image))
            {
              clay_image_remove_guide (image, guide, TRUE);

              continue;
            }
        }
      else
        {
          new_orientation = CLAY_ORIENTATION_VERTICAL;
          new_position    = SIGNED_ROUND (vertices[1].x);

          if (new_position < 0 || new_position > clay_image_get_width (image))
            {
              clay_image_remove_guide (image, guide, TRUE);

              continue;
            }
        }

      if (new_orientation != old_orientation ||
          new_position    != old_position)
        {
          clay_image_undo_push_guide (image, NULL, guide);

          clay_guide_set_orientation (guide, new_orientation);
          clay_guide_set_position    (guide, new_position);

          clay_image_guide_moved (image, guide);
        }
    }
}

static void
clay_image_transform_sample_points (ClayImage           *image,
                                    const ClayMatrix3   *matrix,
                                    const GeglRectangle *old_bounds)
{
  GList *iter;

  for (iter = clay_image_get_sample_points (image); iter;)
    {
      ClaySamplePoint     *sample_point = iter->data;
      gint                 old_x;
      gint                 old_y;
      gint                 new_x;
      gint                 new_y;
      ClayVector2          vertices[1];
      gint                 n_vertices;

      iter = g_list_next (iter);

      clay_sample_point_get_position (sample_point, &old_x, &old_y);

      vertices[0].x = old_x;
      vertices[0].y = old_y;

      clay_transform_polygon (matrix,
                              vertices, 1, FALSE,
                              vertices, &n_vertices);

      if (n_vertices < 1)
        {
          clay_image_remove_sample_point (image, sample_point, TRUE);

          continue;
        }

      new_x = SIGNED_ROUND (vertices[0].x);
      new_y = SIGNED_ROUND (vertices[0].y);

      if (new_x < 0 || new_x >= clay_image_get_width  (image) ||
          new_y < 0 || new_y >= clay_image_get_height (image))
        {
          clay_image_remove_sample_point (image, sample_point, TRUE);

          continue;
        }

      if (new_x != old_x || new_y != old_y)
        clay_image_move_sample_point (image, sample_point, new_x, new_y, TRUE);
    }
}


/*  public functions  */

void
clay_image_transform (ClayImage              *image,
                      ClayContext            *context,
                      const ClayMatrix3      *matrix,
                      ClayTransformDirection  direction,
                      ClayInterpolationType   interpolation_type,
                      ClayTransformResize     clip_result,
                      ClayProgress           *progress)
{
  ClayObjectQueue *queue;
  ClayItem        *item;
  ClayMatrix3      transform;
  GeglRectangle    old_bounds;
  GeglRectangle    new_bounds;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  clay_set_busy (image->clay);

  old_bounds.x      = 0;
  old_bounds.y      = 0;
  old_bounds.width  = clay_image_get_width  (image);
  old_bounds.height = clay_image_get_height (image);

  transform = *matrix;

  if (direction == CLAY_TRANSFORM_BACKWARD)
    clay_matrix3_invert (&transform);

  clay_transform_resize_boundary (&transform, clip_result,

                                  old_bounds.x,
                                  old_bounds.y,
                                  old_bounds.x + old_bounds.width,
                                  old_bounds.y + old_bounds.height,

                                  &new_bounds.x,
                                  &new_bounds.y,
                                  &new_bounds.width,
                                  &new_bounds.height);

  new_bounds.width  -= new_bounds.x;
  new_bounds.height -= new_bounds.y;

  clay_matrix3_translate (&transform,
                          old_bounds.x - new_bounds.x,
                          old_bounds.y - new_bounds.y);

  queue    = clay_object_queue_new (progress);
  progress = CLAY_PROGRESS (queue);

  clay_object_queue_push_container (queue, clay_image_get_layers (image));
  clay_object_queue_push (queue, clay_image_get_mask (image));
  clay_object_queue_push_container (queue, clay_image_get_channels (image));
  clay_object_queue_push_container (queue, clay_image_get_paths (image));

  g_object_freeze_notify (G_OBJECT (image));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_TRANSFORM, NULL);

  /*  Transform all layers, channels (including selection mask), and path  */
  while ((item = clay_object_queue_pop (queue)))
    {
      ClayTransformResize clip = CLAY_TRANSFORM_RESIZE_ADJUST;

      /* Non-rasterized vector layers will be transformed when their path is */
      if (clay_item_is_vector_layer (item))
        continue;

      if (CLAY_IS_CHANNEL (item))
        clip = clip_result;

      clay_item_transform (item,
                           context,
                           &transform, direction,
                           interpolation_type, clip,
                           progress);

      if (CLAY_IS_PATH (item))
        clay_item_set_size (item, new_bounds.width, new_bounds.height);
    }

  /*  Resize the image (if needed)  */
  if (! gegl_rectangle_equal (&new_bounds, &old_bounds))
    {
      clay_image_undo_push_image_size (image,
                                       NULL,
                                       new_bounds.x,
                                       new_bounds.y,
                                       new_bounds.width,
                                       new_bounds.height);

      g_object_set (image,
                    "width",  new_bounds.width,
                    "height", new_bounds.height,
                    NULL);
    }

  /*  Transform all Guides  */
  clay_image_transform_guides (image, &transform, &old_bounds);

  /*  Transform all sample points  */
  clay_image_transform_sample_points (image, &transform, &old_bounds);

  clay_image_undo_group_end (image);

  g_object_unref (queue);

  if (! gegl_rectangle_equal (&new_bounds, &old_bounds))
    {
      clay_image_size_changed_detailed (image,
                                        old_bounds.x - new_bounds.x,
                                        old_bounds.y - new_bounds.y,
                                        old_bounds.width,
                                        old_bounds.height);
    }

  g_object_thaw_notify (G_OBJECT (image));

  clay_unset_busy (image->clay);
}
