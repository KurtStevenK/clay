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
#include <gexiv2/gexiv2.h>

#include "core-types.h"

#include "config/claydialogconfig.h"

#include "clay.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-flip.h"
#include "clayimage-metadata.h"
#include "clayimage-rotate.h"
#include "clayimage-guides.h"
#include "clayimage-sample-points.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitem.h"
#include "claylayer.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"
#include "claysamplepoint.h"

#include "path/claypath.h"
#include "path/clayvectorlayer.h"


static void  clay_image_rotate_item_offset   (ClayImage         *image,
                                              ClayRotationType   rotate_type,
                                              ClayItem          *item,
                                              gint               off_x,
                                              gint               off_y);
static void  clay_image_rotate_guides        (ClayImage         *image,
                                              ClayRotationType   rotate_type);
static void  clay_image_rotate_sample_points (ClayImage         *image,
                                              ClayRotationType   rotate_type);

static void  clay_image_metadata_rotate      (ClayImage         *image,
                                              ClayContext       *context,
                                              GExiv2Orientation  orientation,
                                              ClayProgress      *progress);


/* Public Functions */

void
clay_image_rotate (ClayImage        *image,
                   ClayContext      *context,
                   ClayRotationType  rotate_type,
                   ClayProgress     *progress)
{
  ClayObjectQueue *queue;
  ClayItem        *item;
  GList           *list;
  gdouble          center_x;
  gdouble          center_y;
  gint             new_image_width;
  gint             new_image_height;
  gint             previous_image_width;
  gint             previous_image_height;
  gint             offset_x;
  gint             offset_y;
  gboolean         size_changed;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  previous_image_width  = clay_image_get_width  (image);
  previous_image_height = clay_image_get_height (image);

  center_x              = previous_image_width  / 2.0;
  center_y              = previous_image_height / 2.0;

  /*  Resize the image (if needed)  */
  switch (rotate_type)
    {
    case CLAY_ROTATE_DEGREES90:
    case CLAY_ROTATE_DEGREES270:
      new_image_width  = clay_image_get_height (image);
      new_image_height = clay_image_get_width  (image);
      size_changed     = TRUE;
      offset_x         = (clay_image_get_width  (image) - new_image_width)  / 2;
      offset_y         = (clay_image_get_height (image) - new_image_height) / 2;
      break;

    case CLAY_ROTATE_DEGREES180:
      new_image_width  = clay_image_get_width  (image);
      new_image_height = clay_image_get_height (image);
      size_changed     = FALSE;
      offset_x         = 0;
      offset_y         = 0;
      break;

    default:
      g_return_if_reached ();
      return;
    }

  clay_set_busy (image->clay);

  queue    = clay_object_queue_new (progress);
  progress = CLAY_PROGRESS (queue);

  clay_object_queue_push_container (queue, clay_image_get_layers (image));
  clay_object_queue_push (queue, clay_image_get_mask (image));
  clay_object_queue_push_container (queue, clay_image_get_channels (image));
  clay_object_queue_push_container (queue, clay_image_get_paths (image));

  g_object_freeze_notify (G_OBJECT (image));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ROTATE, NULL);

  /*  Rotate all layers, channels (including selection mask), and path  */
  while ((item = clay_object_queue_pop (queue)))
    {
      gint off_x;
      gint off_y;

      /* Non-rasterized vector layers will be rotated when their path is */
      if (clay_item_is_vector_layer (item))
        continue;

      clay_item_get_offset (item, &off_x, &off_y);

      clay_item_rotate (item, context, rotate_type, center_x, center_y, FALSE);

      if (CLAY_IS_LAYER (item))
        {
          clay_image_rotate_item_offset (image, rotate_type, item, off_x, off_y);
        }
      else
        {
          clay_item_set_offset (item, 0, 0);

          if (CLAY_IS_PATH (item))
            {
              clay_item_set_size (item, new_image_width, new_image_height);

              clay_item_translate (item,
                                   (new_image_width  - clay_image_get_width  (image)) / 2,
                                   (new_image_height - clay_image_get_height (image)) / 2,
                                   FALSE);
            }
        }

      clay_progress_set_value (progress, 1.0);
    }

  /*  Rotate all Guides  */
  clay_image_rotate_guides (image, rotate_type);

  /*  Rotate all sample points  */
  clay_image_rotate_sample_points (image, rotate_type);

  /*  Resize the image (if needed)  */
  if (size_changed)
    {
      gdouble xres;
      gdouble yres;

      clay_image_undo_push_image_size (image,
                                       NULL,
                                       offset_x,
                                       offset_y,
                                       new_image_width,
                                       new_image_height);

      g_object_set (image,
                    "width",  new_image_width,
                    "height", new_image_height,
                    NULL);

      clay_image_get_resolution (image, &xres, &yres);

      if (xres != yres)
        clay_image_set_resolution (image, yres, xres);
    }

  /*  Notify guide movements  */
  for (list = clay_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      clay_image_guide_moved (image, list->data);
    }

  /*  Notify sample point movements  */
  for (list = clay_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      clay_image_sample_point_moved (image, list->data);
    }

  clay_image_undo_group_end (image);

  g_object_unref (queue);

  if (size_changed)
    clay_image_size_changed_detailed (image,
                                      -offset_x,
                                      -offset_y,
                                      previous_image_width,
                                      previous_image_height);

  g_object_thaw_notify (G_OBJECT (image));

  clay_unset_busy (image->clay);
}

void
clay_image_import_rotation_metadata (ClayImage    *image,
                                     ClayContext  *context,
                                     ClayProgress *progress,
                                     gboolean      interactive)
{
  ClayMetadata *metadata;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  metadata = clay_image_get_metadata (image);

  if (metadata)
    {
      ClayMetadataRotationPolicy policy;

      policy = CLAY_DIALOG_CONFIG (image->clay->config)->metadata_rotation_policy;
      if (policy == CLAY_METADATA_ROTATION_POLICY_ASK)
        {
          if (interactive)
            {
              gboolean dont_ask = FALSE;

              policy = clay_query_rotation_policy (image->clay, image,
                                                   context, &dont_ask);

              if (dont_ask)
                {
                  g_object_set (G_OBJECT (image->clay->config),
                                "metadata-rotation-policy", policy,
                                NULL);
                }
            }
          else
            {
              policy = CLAY_METADATA_ROTATION_POLICY_ROTATE;
            }
        }

      if (policy == CLAY_METADATA_ROTATION_POLICY_ROTATE)
        clay_image_metadata_rotate (image, context,
                                    gexiv2_metadata_try_get_orientation (GEXIV2_METADATA (metadata), NULL),
                                    progress);

      gexiv2_metadata_try_set_orientation (GEXIV2_METADATA (metadata),
                                           GEXIV2_ORIENTATION_NORMAL,
                                           NULL);
    }
}

void
clay_image_apply_metadata_orientation (ClayImage         *image,
                                       ClayContext       *context,
                                       ClayMetadata      *metadata,
                                       ClayProgress      *progress)
{
  clay_image_metadata_rotate (image, context,
                              gexiv2_metadata_try_get_orientation (GEXIV2_METADATA (metadata), NULL),
                              progress);
}


/* Private Functions */

static void
clay_image_rotate_item_offset (ClayImage        *image,
                               ClayRotationType  rotate_type,
                               ClayItem         *item,
                               gint              off_x,
                               gint              off_y)
{
  gint x = 0;
  gint y = 0;

  switch (rotate_type)
    {
    case CLAY_ROTATE_DEGREES90:
      x = clay_image_get_height (image) - off_y - clay_item_get_width (item);
      y = off_x;
      break;

    case CLAY_ROTATE_DEGREES270:
      x = off_y;
      y = clay_image_get_width (image) - off_x - clay_item_get_height (item);
      break;

    case CLAY_ROTATE_DEGREES180:
      return;

    default:
      g_return_if_reached ();
    }

  clay_item_get_offset (item, &off_x, &off_y);

  x -= off_x;
  y -= off_y;

  if (x || y)
    clay_item_translate (item, x, y, FALSE);
}

static void
clay_image_rotate_guides (ClayImage        *image,
                          ClayRotationType  rotate_type)
{
  GList *list;

  /*  Rotate all Guides  */
  for (list = clay_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      ClayGuide           *guide       = list->data;
      ClayOrientationType  orientation = clay_guide_get_orientation (guide);
      gint                 position    = clay_guide_get_position (guide);

      switch (rotate_type)
        {
        case CLAY_ROTATE_DEGREES90:
          switch (orientation)
            {
            case CLAY_ORIENTATION_HORIZONTAL:
              clay_image_undo_push_guide (image, NULL, guide);
              clay_guide_set_orientation (guide, CLAY_ORIENTATION_VERTICAL);
              clay_guide_set_position (guide,
                                       clay_image_get_height (image) - position);
              break;

            case CLAY_ORIENTATION_VERTICAL:
              clay_image_undo_push_guide (image, NULL, guide);
              clay_guide_set_orientation (guide, CLAY_ORIENTATION_HORIZONTAL);
              break;

            default:
              break;
            }
          break;

        case CLAY_ROTATE_DEGREES180:
          switch (orientation)
            {
            case CLAY_ORIENTATION_HORIZONTAL:
              clay_image_move_guide (image, guide,
                                     clay_image_get_height (image) - position,
                                     TRUE);
              break;

            case CLAY_ORIENTATION_VERTICAL:
              clay_image_move_guide (image, guide,
                                     clay_image_get_width (image) - position,
                                     TRUE);
              break;

            default:
              break;
            }
          break;

        case CLAY_ROTATE_DEGREES270:
          switch (orientation)
            {
            case CLAY_ORIENTATION_HORIZONTAL:
              clay_image_undo_push_guide (image, NULL, guide);
              clay_guide_set_orientation (guide, CLAY_ORIENTATION_VERTICAL);
              break;

            case CLAY_ORIENTATION_VERTICAL:
              clay_image_undo_push_guide (image, NULL, guide);
              clay_guide_set_orientation (guide, CLAY_ORIENTATION_HORIZONTAL);
              clay_guide_set_position (guide,
                                       clay_image_get_width (image) - position);
              break;

            default:
              break;
            }
          break;
        }
    }
}


static void
clay_image_rotate_sample_points (ClayImage        *image,
                                 ClayRotationType  rotate_type)
{
  GList *list;

  /*  Rotate all sample points  */
  for (list = clay_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      ClaySamplePoint *sample_point = list->data;
      gint             old_x;
      gint             old_y;

      clay_image_undo_push_sample_point (image, NULL, sample_point);

      clay_sample_point_get_position (sample_point, &old_x, &old_y);

      switch (rotate_type)
        {
        case CLAY_ROTATE_DEGREES90:
          clay_sample_point_set_position (sample_point,
                                          clay_image_get_height (image) - old_y,
                                          old_x);
          break;

        case CLAY_ROTATE_DEGREES180:
          clay_sample_point_set_position (sample_point,
                                          clay_image_get_width  (image) - old_x,
                                          clay_image_get_height (image) - old_y);
          break;

        case CLAY_ROTATE_DEGREES270:
          clay_sample_point_set_position (sample_point,
                                          old_y,
                                          clay_image_get_width (image) - old_x);
          break;
        }
    }
}

static void
clay_image_metadata_rotate (ClayImage         *image,
                            ClayContext       *context,
                            GExiv2Orientation  orientation,
                            ClayProgress      *progress)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      clay_image_flip (image, context, CLAY_ORIENTATION_HORIZONTAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      clay_image_rotate (image, context, CLAY_ROTATE_DEGREES180, progress);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      clay_image_flip (image, context, CLAY_ORIENTATION_VERTICAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      clay_image_rotate (image, context, CLAY_ROTATE_DEGREES90, progress);
      clay_image_flip (image, context, CLAY_ORIENTATION_HORIZONTAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      clay_image_rotate (image, context, CLAY_ROTATE_DEGREES90, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      clay_image_rotate (image, context, CLAY_ROTATE_DEGREES90, progress);
      clay_image_flip (image, context, CLAY_ORIENTATION_VERTICAL, progress);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      clay_image_rotate (image, context, CLAY_ROTATE_DEGREES270, progress);
      break;

    default: /* shouldn't happen */
      break;
    }
}
