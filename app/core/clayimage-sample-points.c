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

#include "clay.h"
#include "clayimage.h"
#include "clayimage-private.h"
#include "clayimage-sample-points.h"
#include "clayimage-undo-push.h"
#include "claysamplepoint.h"

#include "clay-intl.h"


/*  public functions  */

ClaySamplePoint *
clay_image_add_sample_point_at_pos (ClayImage *image,
                                    gint       x,
                                    gint       y,
                                    gboolean   push_undo)
{
  ClaySamplePoint *sample_point;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (x >= 0 && x < clay_image_get_width  (image), NULL);
  g_return_val_if_fail (y >= 0 && y < clay_image_get_height (image), NULL);

  sample_point = clay_sample_point_new (image->clay->next_sample_point_id++);

  if (push_undo)
    clay_image_undo_push_sample_point (image, C_("undo-type", "Add Sample Point"),
                                       sample_point);

  clay_image_add_sample_point (image, sample_point, x, y);
  g_object_unref (sample_point);

  return sample_point;
}

void
clay_image_add_sample_point (ClayImage       *image,
                             ClaySamplePoint *sample_point,
                             gint             x,
                             gint             y)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  private->sample_points = g_list_append (private->sample_points, sample_point);

  clay_sample_point_set_position (sample_point, x, y);
  g_object_ref (sample_point);

  clay_image_sample_point_added (image, sample_point);
}

void
clay_image_remove_sample_point (ClayImage       *image,
                                ClaySamplePoint *sample_point,
                                gboolean         push_undo)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    clay_image_undo_push_sample_point (image,
                                       C_("undo-type", "Remove Sample Point"),
                                       sample_point);

  private->sample_points = g_list_remove (private->sample_points, sample_point);
  clay_aux_item_removed (CLAY_AUX_ITEM (sample_point));

  clay_image_sample_point_removed (image, sample_point);

  clay_sample_point_set_position (sample_point,
                                  CLAY_SAMPLE_POINT_POSITION_UNDEFINED,
                                  CLAY_SAMPLE_POINT_POSITION_UNDEFINED);
  g_object_unref (sample_point);
}

void
clay_image_move_sample_point (ClayImage       *image,
                              ClaySamplePoint *sample_point,
                              gint             x,
                              gint             y,
                              gboolean         push_undo)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x < clay_image_get_width  (image));
  g_return_if_fail (y < clay_image_get_height (image));

  if (push_undo)
    clay_image_undo_push_sample_point (image,
                                       C_("undo-type", "Move Sample Point"),
                                       sample_point);

  clay_sample_point_set_position (sample_point, x, y);

  clay_image_sample_point_moved (image, sample_point);
}

void
clay_image_set_sample_point_pick_mode (ClayImage         *image,
                                       ClaySamplePoint   *sample_point,
                                       ClayColorPickMode  pick_mode,
                                       gboolean           push_undo)
{
  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  if (push_undo)
    clay_image_undo_push_sample_point (image,
                                       C_("undo-type",
                                          "Set Sample Point Pick Mode"),
                                       sample_point);

  clay_sample_point_set_pick_mode (sample_point, pick_mode);

  /* well... */
  clay_image_sample_point_moved (image, sample_point);
}

GList *
clay_image_get_sample_points (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->sample_points;
}

ClaySamplePoint *
clay_image_get_sample_point (ClayImage *image,
                             guint32    id)
{
  GList *sample_points;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  for (sample_points = CLAY_IMAGE_GET_PRIVATE (image)->sample_points;
       sample_points;
       sample_points = g_list_next (sample_points))
    {
      ClaySamplePoint *sample_point = sample_points->data;

      if (clay_aux_item_get_id (CLAY_AUX_ITEM (sample_point)) == id)
        return sample_point;
    }

  return NULL;
}

ClaySamplePoint *
clay_image_get_next_sample_point (ClayImage *image,
                                  guint32    id,
                                  gboolean  *sample_point_found)
{
  GList *sample_points;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (sample_point_found != NULL, NULL);

  if (id == 0)
    *sample_point_found = TRUE;
  else
    *sample_point_found = FALSE;

  for (sample_points = CLAY_IMAGE_GET_PRIVATE (image)->sample_points;
       sample_points;
       sample_points = g_list_next (sample_points))
    {
      ClaySamplePoint *sample_point = sample_points->data;

      if (*sample_point_found) /* this is the first guide after the found one */
        return sample_point;

      if (clay_aux_item_get_id (CLAY_AUX_ITEM (sample_point)) == id) /* found it, next one will be returned */
        *sample_point_found = TRUE;
    }

  return NULL;
}
