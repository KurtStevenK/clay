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

#include "claygrouplayer.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-pick-item.h"
#include "clayimage-private.h"
#include "claypickable.h"
#include "claysamplepoint.h"

#include "path/claypath.h"
#include "path/claystroke.h"

#include "text/claytextlayer.h"

/* The opacity threshold over which we don't consider a pixel to be
 * visible.
 * After trying an initial higher value, we decided to go with a near
 * epsilon-ish value. Even a low alpha is still visible on canvas and it
 * doesn't make sense UX-wise to not be able to pick the pixel.
 */
#define CLAY_PICK_OPACITY_THRESHOLD 0.001


ClayLayer *
clay_image_pick_layer (ClayImage *image,
                       gint       x,
                       gint       y,
                       ClayLayer *previously_picked)
{
  GList *all_layers;
  GList *list;
  gint   off_x, off_y;
  gint   tries = 1;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  all_layers = clay_image_get_layer_list (image);

  if (previously_picked)
    {
      clay_item_get_offset (CLAY_ITEM (previously_picked), &off_x, &off_y);
      if (clay_pickable_get_opacity_at (CLAY_PICKABLE (previously_picked),
                                        x - off_x, y - off_y) <= CLAY_PICK_OPACITY_THRESHOLD)
        previously_picked = NULL;
      else
        tries++;
    }

  while (tries)
    {
      for (list = all_layers; list; list = g_list_next (list))
        {
          ClayLayer *layer = list->data;

          if (previously_picked)
            {
              /* Take the first layer with a pixel at given coordinates
               * after the previously picked one.
               */
              if (layer == previously_picked)
                previously_picked = NULL;
              continue;
            }

          clay_item_get_offset (CLAY_ITEM (layer), &off_x, &off_y);

          if (clay_pickable_get_opacity_at (CLAY_PICKABLE (layer),
                                            x - off_x, y - off_y) > CLAY_PICK_OPACITY_THRESHOLD)
            {
              g_list_free (all_layers);

              return layer;
            }
        }
      tries--;
    }

  g_list_free (all_layers);

  return NULL;
}

ClayLayer *
clay_image_pick_layer_by_bounds (ClayImage *image,
                                 gint       x,
                                 gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  all_layers = clay_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      ClayLayer *layer = list->data;

      if (clay_item_is_visible (CLAY_ITEM (layer)))
        {
          gint off_x, off_y;
          gint width, height;

          clay_item_get_offset (CLAY_ITEM (layer), &off_x, &off_y);
          width  = clay_item_get_width  (CLAY_ITEM (layer));
          height = clay_item_get_height (CLAY_ITEM (layer));

          if (x >= off_x        &&
              y >= off_y        &&
              x < off_x + width &&
              y < off_y + height)
            {
              g_list_free (all_layers);

              return layer;
            }
        }
    }

  g_list_free (all_layers);

  return NULL;
}

ClayTextLayer *
clay_image_pick_text_layer (ClayImage *image,
                            gint       x,
                            gint       y)
{
  GList *all_layers;
  GList *list;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  all_layers = clay_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      ClayLayer *layer = list->data;
      gint       off_x, off_y;

      clay_item_get_offset (CLAY_ITEM (layer), &off_x, &off_y);

      if (CLAY_IS_TEXT_LAYER (layer) &&
          x >= off_x &&
          y >= off_y &&
          x <  off_x + clay_item_get_width  (CLAY_ITEM (layer)) &&
          y <  off_y + clay_item_get_height (CLAY_ITEM (layer)) &&
          clay_item_is_visible (CLAY_ITEM (layer)))
        {
          g_list_free (all_layers);

          return CLAY_TEXT_LAYER (layer);
        }
      else if (clay_pickable_get_opacity_at (CLAY_PICKABLE (layer),
                                             x - off_x, y - off_y) > 0.25)
        {
          /*  a normal layer covers any possible text layers below,
           *  bail out
           */

          break;
        }
    }

  g_list_free (all_layers);

  return NULL;
}

ClayPath *
clay_image_pick_path (ClayImage *image,
                      gdouble    x,
                      gdouble    y,
                      gdouble    epsilon_x,
                      gdouble    epsilon_y)
{
  ClayPath *ret = NULL;
  GList    *all_path;
  GList    *list;
  gdouble   mindist = G_MAXDOUBLE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  all_path = clay_image_get_path_list (image);

  for (list = all_path; list; list = g_list_next (list))
    {
      ClayPath *path = list->data;

      if (clay_item_is_visible (CLAY_ITEM (path)))
        {
          ClayStroke *stroke = NULL;
          ClayCoords  coords = CLAY_COORDS_DEFAULT_VALUES;

          while ((stroke = clay_path_stroke_get_next (path, stroke)))
            {
              gdouble dist;

              coords.x = x;
              coords.y = y;

              dist = clay_stroke_nearest_point_get (stroke, &coords, 1.0,
                                                    NULL, NULL, NULL, NULL);

              if (dist >= 0.0 &&
                  dist <  MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  ret     = path;
                }
            }
        }
    }

  g_list_free (all_path);

  return ret;
}

static ClayGuide *
clay_image_pick_guide_internal (ClayImage           *image,
                                gdouble              x,
                                gdouble              y,
                                gdouble              epsilon_x,
                                gdouble              epsilon_y,
                                ClayOrientationType  orientation)
{
  GList     *list;
  ClayGuide *ret     = NULL;
  gdouble    mindist = G_MAXDOUBLE;

  for (list = CLAY_IMAGE_GET_PRIVATE (image)->guides;
       list;
       list = g_list_next (list))
    {
      ClayGuide *guide    = list->data;
      gint       position = clay_guide_get_position (guide);
      gdouble    dist;

      switch (clay_guide_get_orientation (guide))
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          if (orientation == CLAY_ORIENTATION_HORIZONTAL ||
              orientation == CLAY_ORIENTATION_UNKNOWN)
            {
              dist = ABS (position - y);
              if (dist < MIN (epsilon_y, mindist))
                {
                  mindist = dist;
                  ret = guide;
                }
            }
          break;

        /* mindist always is in vertical resolution to make it comparable */
        case CLAY_ORIENTATION_VERTICAL:
          if (orientation == CLAY_ORIENTATION_VERTICAL ||
              orientation == CLAY_ORIENTATION_UNKNOWN)
            {
              dist = ABS (position - x);
              if (dist < MIN (epsilon_x, mindist / epsilon_y * epsilon_x))
                {
                  mindist = dist * epsilon_y / epsilon_x;
                  ret = guide;
                }
            }
          break;

        default:
          continue;
        }
    }

  return ret;
}

ClayGuide *
clay_image_pick_guide (ClayImage *image,
                       gdouble    x,
                       gdouble    y,
                       gdouble    epsilon_x,
                       gdouble    epsilon_y)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  return clay_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                         CLAY_ORIENTATION_UNKNOWN);
}

GList *
clay_image_pick_guides (ClayImage *image,
                        gdouble    x,
                        gdouble    y,
                        gdouble    epsilon_x,
                        gdouble    epsilon_y)
{
  ClayGuide *guide;
  GList     *result = NULL;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  guide = clay_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                          CLAY_ORIENTATION_HORIZONTAL);

  if (guide)
    result = g_list_append (result, guide);

  guide = clay_image_pick_guide_internal (image, x, y, epsilon_x, epsilon_y,
                                          CLAY_ORIENTATION_VERTICAL);

  if (guide)
    result = g_list_append (result, guide);

  return result;
}

ClaySamplePoint *
clay_image_pick_sample_point (ClayImage *image,
                              gdouble    x,
                              gdouble    y,
                              gdouble    epsilon_x,
                              gdouble    epsilon_y)
{
  GList           *list;
  ClaySamplePoint *ret     = NULL;
  gdouble          mindist = G_MAXDOUBLE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (epsilon_x > 0 && epsilon_y > 0, NULL);

  if (x < 0 || x >= clay_image_get_width  (image) ||
      y < 0 || y >= clay_image_get_height (image))
    {
      return NULL;
    }

  for (list = CLAY_IMAGE_GET_PRIVATE (image)->sample_points;
       list;
       list = g_list_next (list))
    {
      ClaySamplePoint *sample_point = list->data;
      gint             sp_x;
      gint             sp_y;
      gdouble          dist;

      clay_sample_point_get_position (sample_point, &sp_x, &sp_y);

      if (sp_x < 0 || sp_y < 0)
        continue;

      dist = hypot ((sp_x + 0.5) - x,
                    (sp_y + 0.5) - y);
      if (dist < MIN (epsilon_y, mindist))
        {
          mindist = dist;
          ret = sample_point;
        }
    }

  return ret;
}
