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

#include "clay.h"
#include "claygrid.h"
#include "clayguide.h"
#include "clayimage.h"
#include "claylayer.h"
#include "clayimage-grid.h"
#include "clayimage-guides.h"
#include "clayimage-snap.h"

#include "path/claypath.h"
#include "path/claystroke.h"

#include "clay-intl.h"


static gboolean  clay_image_snap_distance (const gdouble  unsnapped,
                                           const gdouble  nearest,
                                           const gdouble  epsilon,
                                           gdouble       *mindist,
                                           gdouble       *target);



/*  public functions  */

gboolean
clay_image_snap_x (ClayImage         *image,
                   ClayImageSnapData *snapping_data,
                   gdouble            x,
                   gdouble           *tx,
                   gdouble            epsilon_x,
                   gboolean           snap_to_guides,
                   gboolean           snap_to_grid,
                   gboolean           snap_to_canvas,
                   gboolean           snap_to_bbox,
                   gboolean           snap_to_equidistance,
                   ClayAlignmentType  alignment_side)
{
  gdouble   mindist   = G_MAXDOUBLE;
  gdouble   mindist_t = G_MAXDOUBLE;
  gboolean  snapped   = FALSE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);

  *tx = x;

  if (! clay_image_get_guides (image)) snap_to_guides = FALSE;
  if (! clay_image_get_grid (image))   snap_to_grid   = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_bbox || snap_to_equidistance))
    return FALSE;

  if (x < -epsilon_x || x >= (clay_image_get_width (image) + epsilon_x))
    return FALSE;

  if (snap_to_guides)
    {
      GList *list;

      for (list = clay_image_get_guides (image); list; list = g_list_next (list))
        {
          ClayGuide *guide    = list->data;
          gint       position = clay_guide_get_position (guide);

          if (clay_guide_is_custom (guide))
            continue;

          if (clay_guide_get_orientation (guide) == CLAY_ORIENTATION_VERTICAL)
            {
              snapped |= clay_image_snap_distance (x, position,
                                                   epsilon_x,
                                                   &mindist, tx);
            }
        }
    }

  if (snap_to_grid)
    {
      ClayGrid *grid = clay_image_get_grid (image);
      gdouble   xspacing;
      gdouble   xoffset;

      clay_grid_get_spacing (grid, &xspacing, NULL);
      clay_grid_get_offset  (grid, &xoffset,  NULL);

      if (xspacing > 0.0)
        {
          gdouble nearest;

          nearest = xoffset + RINT ((x - xoffset) / xspacing) * xspacing;

          snapped |= clay_image_snap_distance (x, nearest,
                                               epsilon_x,
                                               &mindist, tx);
        }
    }

  if (snap_to_canvas)
    {
      snapped |= clay_image_snap_distance (x, 0,
                                           epsilon_x,
                                           &mindist, tx);
      snapped |= clay_image_snap_distance (x, clay_image_get_width (image),
                                           epsilon_x,
                                           &mindist, tx);
    }

  if (snap_to_bbox)
    {
      GList    *selected_layers_list;
      GList    *layers_list;
      gdouble   gcx;
      gint      gx, gy, gw, gh;
      gboolean  not_in_selected_set;
      gboolean  snapped_2;

      selected_layers_list = clay_image_get_selected_layers (image);
      layers_list          = clay_image_get_layer_list (image);
      not_in_selected_set  = TRUE;
      snapped_2            = FALSE;

      for (GList *iter = layers_list; iter; iter = iter->next)
        {
          if (!clay_item_is_visible (iter->data))
            continue;

          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);
          gcx = (gdouble) gx + (gdouble) gw/2.0;
          not_in_selected_set = TRUE;

          for (GList *iter2 = selected_layers_list; iter2; iter2 = iter2->next)
            {
              if (iter2->data == iter->data)
                not_in_selected_set = FALSE;
            }

          if ((gint) x >= gx && (gint) x <= (gx+gw) && not_in_selected_set)
            {
              snapped_2 |= clay_image_snap_distance (x, (gdouble) gx,
                                                     epsilon_x,
                                                     &mindist, tx);

              snapped_2 |= clay_image_snap_distance (x, (gdouble) gx+gw,
                                                     epsilon_x,
                                                     &mindist, tx);

              snapped_2 |= clay_image_snap_distance (x, gcx,
                                                     epsilon_x,
                                                     &mindist, tx);

              if (snapped_2)
                {
                  snapped |= snapped_2;
                  snapped_2 = FALSE;

                  snapping_data->snapped_layer_horizontal = iter->data;
                  snapping_data->snapped_side_horizontal = alignment_side;
                }
            }
        }

      mindist_t = mindist;

      g_list_free (layers_list);
    }

  if (snap_to_equidistance)
    {
      GList     *layers_list          = clay_image_get_layer_list (image);
      GList     *selected_layers_list = clay_image_get_selected_layers (image);
      ClayLayer *near_layer1          = NULL;
      gint       gx, gy, gw, gh;
      gint       selected_set_y1      = G_MAXINT;
      gint       selected_set_y2      = G_MININT;
      gint       left_box_x2          = G_MININT;
      gint       left_box_x1          = 0;
      gint       left_box2_x2         = G_MININT;
      gint       right_box_x1         = G_MAXINT;
      gint       right_box_x2         = 0;
      gint       right_box2_x1        = G_MAXINT;
      gint       near_box_y1          = 0;
      gint       near_box_y2          = 0;

      for (GList *iter = selected_layers_list; iter; iter = iter->next)
        {
          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);

          if (gy < selected_set_y1)
            selected_set_y1 = gy;
          if ((gy+gh) > selected_set_y2)
            selected_set_y2 = gy+gh;
        }

      for (GList *iter = layers_list; iter; iter = iter->next)
        {
          if (!clay_item_is_visible (iter->data))
            continue;

          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);

          if ((gy >= selected_set_y1 && gy <= selected_set_y2)           ||
              ((gy+gh) >= selected_set_y1 && (gy+gh) <= selected_set_y2) ||
              (selected_set_y1 >= gy && selected_set_y1 <= (gy+gh))      ||
              (selected_set_y2 >= gy && selected_set_y2 <= (gy+gh)))
            {
              if (alignment_side == CLAY_ALIGN_LEFT &&
                  (gx+gw) < (gint) x && (gx+gw) > left_box_x2)
                {
                  left_box_x2 = gx+gw;
                  left_box_x1 = gx;
                  near_box_y1 = gy;
                  near_box_y2 = gy+gh;

                  near_layer1 = iter->data;
                }
              else if (alignment_side == CLAY_ALIGN_RIGHT &&
                       gx > (gint) x && gx < right_box_x1)
                {
                  right_box_x1 = gx;
                  right_box_x2 = gx+gw;
                  near_box_y1 = gy;
                  near_box_y2 = gy+gh;

                  near_layer1 = iter->data;
                }
            }
        }

      if (left_box_x2 != G_MININT || right_box_x1 != G_MAXINT)
        {
          for (GList *iter = layers_list; iter; iter = iter->next)
            {
              if (!clay_item_is_visible (iter->data))
                continue;

              clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
              clay_item_get_offset (iter->data, &gx, &gy);

              if ((gy >= near_box_y1 && gy <= near_box_y2)           ||
                  ((gy+gh) >= near_box_y1 && (gy+gh) <= near_box_y2) ||
                  (near_box_y1 >= gy && near_box_y1 <= (gy+gh))      ||
                  (near_box_y2 >= gy && near_box_y2 <= (gy+gh)))
                {
                  if (alignment_side == CLAY_ALIGN_LEFT && (gx+gw) < left_box_x1)
                    {
                      left_box2_x2 = gx+gw;

                      if (clay_image_snap_distance (x, (gdouble) (left_box_x2 + (left_box_x1 - left_box2_x2)),
                                                    epsilon_x,
                                                    &mindist, tx) ||
                          *tx == (left_box_x2 + (left_box_x1 - left_box2_x2)))
                        {
                          snapped |= TRUE;
                          snapping_data->equidistance_side_horizontal = CLAY_ALIGN_LEFT;

                          snapping_data->near_layer_horizontal1 = near_layer1;
                          snapping_data->near_layer_horizontal2 = iter->data;
                          break;
                        }
                    }
                  else if (alignment_side == CLAY_ALIGN_RIGHT && gx > right_box_x2)
                    {
                      right_box2_x1 = gx;

                      if (clay_image_snap_distance (x, (gdouble) (right_box_x1 - (right_box2_x1 - right_box_x2)),
                                                    epsilon_x,
                                                    &mindist, tx) ||
                          *tx == (right_box_x1 - (right_box2_x1 - right_box_x2)))
                        {
                          snapped |= TRUE;
                          snapping_data->equidistance_side_horizontal = CLAY_ALIGN_RIGHT;

                          snapping_data->near_layer_horizontal1 = near_layer1;
                          snapping_data->near_layer_horizontal2 = iter->data;
                          break;
                        }
                    }
                }
            }
        }

      g_list_free (layers_list);
    }

  if (mindist_t != G_MAXDOUBLE && mindist_t > mindist)
    snapping_data->snapped_side_horizontal = CLAY_ARRANGE_HFILL;

  return snapped;
}

gboolean
clay_image_snap_y (ClayImage         *image,
                   ClayImageSnapData *snapping_data,
                   gdouble            y,
                   gdouble           *ty,
                   gdouble            epsilon_y,
                   gboolean           snap_to_guides,
                   gboolean           snap_to_grid,
                   gboolean           snap_to_canvas,
                   gboolean           snap_to_bbox,
                   gboolean           snap_to_equidistance,
                   ClayAlignmentType  alignment_side)
{
  gdouble    mindist   = G_MAXDOUBLE;
  gdouble    mindist_t = G_MAXDOUBLE;
  gboolean   snapped   = FALSE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  *ty = y;

  if (! clay_image_get_guides (image)) snap_to_guides = FALSE;
  if (! clay_image_get_grid (image))   snap_to_grid   = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_bbox || snap_to_equidistance))
    return FALSE;

  if (y < -epsilon_y || y >= (clay_image_get_height (image) + epsilon_y))
    return FALSE;

  if (snap_to_guides)
    {
      GList *list;

      for (list = clay_image_get_guides (image); list; list = g_list_next (list))
        {
          ClayGuide *guide    = list->data;
          gint       position = clay_guide_get_position (guide);

          if (clay_guide_is_custom (guide))
            continue;

          if (clay_guide_get_orientation (guide) == CLAY_ORIENTATION_HORIZONTAL)
            {
              snapped |= clay_image_snap_distance (y, position,
                                                   epsilon_y,
                                                   &mindist, ty);
            }
        }
    }

  if (snap_to_grid)
    {
      ClayGrid *grid = clay_image_get_grid (image);
      gdouble   yspacing;
      gdouble   yoffset;

      clay_grid_get_spacing (grid, NULL, &yspacing);
      clay_grid_get_offset  (grid, NULL, &yoffset);

      if (yspacing > 0.0)
        {
          gdouble nearest;

          nearest = yoffset + RINT ((y - yoffset) / yspacing) * yspacing;

          snapped |= clay_image_snap_distance (y, nearest,
                                               epsilon_y,
                                               &mindist, ty);
        }
    }

  if (snap_to_canvas)
    {
      snapped |= clay_image_snap_distance (y, 0,
                                           epsilon_y,
                                           &mindist, ty);
      snapped |= clay_image_snap_distance (y, clay_image_get_height (image),
                                           epsilon_y,
                                           &mindist, ty);
    }

  if (snap_to_bbox)
    {
      GList    *selected_layers_list;
      GList    *layers_list;
      gdouble   gcy;
      gint      gx, gy, gw, gh;
      gboolean  not_in_selected_set;
      gboolean  snapped_2;

      selected_layers_list = clay_image_get_selected_layers (image);
      layers_list          = clay_image_get_layer_list (image);
      not_in_selected_set  = TRUE;
      snapped_2            = FALSE;

      for (GList *iter = layers_list; iter; iter = iter->next)
        {
          if (!clay_item_is_visible (iter->data))
            continue;

          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);
          gcy = (gdouble) gy + (gdouble) gh/2.0;
          not_in_selected_set = TRUE;

          for (GList *iter2 = selected_layers_list; iter2; iter2 = iter2->next)
            {
              if (iter2->data == iter->data)
                not_in_selected_set = FALSE;
            }

          if ((gint) y >= gy && (gint) y <= (gy+gh) && not_in_selected_set)
            {
              snapped_2 |= clay_image_snap_distance (y, (gdouble) gy,
                                                     epsilon_y,
                                                     &mindist, ty);

              snapped_2 |= clay_image_snap_distance (y, (gdouble) gy+gh,
                                                     epsilon_y,
                                                     &mindist, ty);

              snapped_2 |= clay_image_snap_distance (y, gcy,
                                                     epsilon_y,
                                                     &mindist, ty);

              if (snapped_2)
                {
                  snapped |= snapped_2;
                  snapped_2 = FALSE;

                  snapping_data->snapped_layer_vertical = iter->data;
                  snapping_data->snapped_side_vertical = alignment_side;
                }
            }
        }

      mindist_t = mindist;

      g_list_free (layers_list);
    }

  if (snap_to_equidistance)
    {
      GList     *layers_list          = clay_image_get_layer_list (image);
      GList     *selected_layers_list = clay_image_get_selected_layers (image);
      ClayLayer *near_layer1          = NULL;
      gint       gx, gy, gw, gh;
      gint       selected_set_x1      = G_MAXINT;
      gint       selected_set_x2      = G_MININT;
      gint       up_box_y2            = G_MININT;
      gint       up_box_y1            = 0;
      gint       up_box2_y2           = G_MININT;
      gint       down_box_y1          = G_MAXINT;
      gint       down_box_y2          = 0;
      gint       down_box2_y1         = G_MAXINT;
      gint       near_box_x1          = 0;
      gint       near_box_x2          = 0;

      for (GList *iter = selected_layers_list; iter; iter = iter->next)
        {
          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);

          if (gx < selected_set_x1)
            selected_set_x1 = gx;
          if ((gx+gw) > selected_set_x2)
            selected_set_x2 = gx+gw;
        }

      for (GList *iter = layers_list; iter; iter = iter->next)
        {
          if (!clay_item_is_visible (iter->data))
            continue;

          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);

          if ((gx >= selected_set_x1 && gx <= selected_set_x2)           ||
              ((gx+gw) >= selected_set_x1 && (gx+gw) <= selected_set_x2) ||
              (selected_set_x1 >= gx && selected_set_x1 <= (gx+gw))      ||
              (selected_set_x2 >= gx && selected_set_x2 <= (gx+gw)))
            {
              if (alignment_side == CLAY_ALIGN_TOP &&
                  (gy+gh) < (gint) y && (gy+gh) > up_box_y2)
                {
                  up_box_y2 = gy+gh;
                  up_box_y1 = gy;
                  near_box_x1 = gx;
                  near_box_x2 = gx+gw;

                  near_layer1 = iter->data;
                }
              else if (alignment_side == CLAY_ALIGN_BOTTOM &&
                       gy > (gint) y && gy < down_box_y1)
                {
                  down_box_y1 = gy;
                  down_box_y2 = gy+gh;
                  near_box_x1 = gx;
                  near_box_x2 = gx+gw;

                  near_layer1 = iter->data;
                }
            }
        }

      if (up_box_y2 != G_MININT || down_box_y1 != G_MAXINT)
        {
          for (GList *iter = layers_list; iter; iter = iter->next)
            {
              if (!clay_item_is_visible (iter->data))
                continue;

              clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
              clay_item_get_offset (iter->data, &gx, &gy);

              if ((gx >= near_box_x1 && gx <= near_box_x2)           ||
                  ((gx+gw) >= near_box_x1 && (gx+gw) <= near_box_x2) ||
                  (near_box_x1 >= gx && near_box_x1 <= (gx+gw))      ||
                  (near_box_x2 >= gx && near_box_x2 <= (gx+gw)))
                {
                  if (alignment_side == CLAY_ALIGN_TOP && (gy+gh) < up_box_y1)
                    {
                      up_box2_y2 = gy+gh;

                      if (clay_image_snap_distance (y, (gdouble) (up_box_y2 + (up_box_y1 - up_box2_y2)),
                                                    epsilon_y,
                                                    &mindist, ty) ||
                          *ty == (up_box_y2 + (up_box_y1 - up_box2_y2)))
                        {
                          snapped |= TRUE;
                          snapping_data->equidistance_side_vertical = CLAY_ALIGN_TOP;

                          snapping_data->near_layer_vertical1 = near_layer1;
                          snapping_data->near_layer_vertical2 = iter->data;
                          break;
                        }
                    }
                  else if (alignment_side == CLAY_ALIGN_BOTTOM && gy > down_box_y2)
                    {
                      down_box2_y1 = gy;

                      if (clay_image_snap_distance (y, (gdouble) (down_box_y1 - (down_box2_y1 - down_box_y2)),
                                                    epsilon_y,
                                                    &mindist, ty) ||
                          *ty == (down_box_y1 - (down_box2_y1 - down_box_y2)))
                        {
                          snapped |= TRUE;
                          snapping_data->equidistance_side_vertical = CLAY_ALIGN_BOTTOM;

                          snapping_data->near_layer_vertical1 = near_layer1;
                          snapping_data->near_layer_vertical2 = iter->data;
                          break;
                        }
                    }
                }
            }
        }

      g_list_free (layers_list);
    }

  if (mindist_t != G_MAXDOUBLE && mindist_t > mindist)
    snapping_data->snapped_side_vertical = CLAY_ARRANGE_HFILL;

  return snapped;
}


gboolean
clay_image_snap_point (ClayImage *image,
                       gdouble    x,
                       gdouble    y,
                       gdouble   *tx,
                       gdouble   *ty,
                       gdouble    epsilon_x,
                       gdouble    epsilon_y,
                       gboolean   snap_to_guides,
                       gboolean   snap_to_grid,
                       gboolean   snap_to_canvas,
                       gboolean   snap_to_path,
                       gboolean   snap_to_bbox,
                       gboolean   show_all)
{
  gdouble  mindist_x = G_MAXDOUBLE;
  gdouble  mindist_y = G_MAXDOUBLE;
  gboolean snapped   = FALSE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  *tx = x;
  *ty = y;

  if (! clay_image_get_guides (image))
    snap_to_guides  = FALSE;
  if (! clay_image_get_grid (image))
    snap_to_grid    = FALSE;
  if (! clay_image_get_selected_paths (image))
    snap_to_path = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_path || snap_to_bbox))
    return FALSE;

  if (! show_all &&
      (x < -epsilon_x || x >= (clay_image_get_width  (image) + epsilon_x) ||
       y < -epsilon_y || y >= (clay_image_get_height (image) + epsilon_y)))
    {
      /* Off-canvas grid is invisible unless "show all" option is
       * enabled. So let's not snap to the invisible grid.
       */
      snap_to_grid   = FALSE;
      snap_to_canvas = FALSE;
    }

  if (snap_to_guides)
    {
      GList *list;

      for (list = clay_image_get_guides (image); list; list = g_list_next (list))
        {
          ClayGuide *guide    = list->data;
          gint       position = clay_guide_get_position (guide);

          if (clay_guide_is_custom (guide))
            continue;

          switch (clay_guide_get_orientation (guide))
            {
            case CLAY_ORIENTATION_HORIZONTAL:
              snapped |= clay_image_snap_distance (y, position,
                                                   epsilon_y,
                                                   &mindist_y, ty);
              break;

            case CLAY_ORIENTATION_VERTICAL:
              snapped |= clay_image_snap_distance (x, position,
                                                   epsilon_x,
                                                   &mindist_x, tx);
              break;

            default:
              break;
            }
        }
    }

  if (snap_to_grid)
    {
      ClayGrid *grid = clay_image_get_grid (image);
      gdouble   xspacing, yspacing;
      gdouble   xoffset, yoffset;

      clay_grid_get_spacing (grid, &xspacing, &yspacing);
      clay_grid_get_offset  (grid, &xoffset,  &yoffset);

      if (xspacing > 0.0)
        {
          gdouble nearest;

          nearest = xoffset + RINT ((x - xoffset) / xspacing) * xspacing;

          snapped |= clay_image_snap_distance (x, nearest,
                                               epsilon_x,
                                               &mindist_x, tx);
        }

      if (yspacing > 0.0)
        {
          gdouble nearest;

          nearest = yoffset + RINT ((y - yoffset) / yspacing) * yspacing;

          snapped |= clay_image_snap_distance (y, nearest,
                                               epsilon_y,
                                               &mindist_y, ty);
        }
    }

  if (snap_to_canvas)
    {
      snapped |= clay_image_snap_distance (x, 0,
                                           epsilon_x,
                                           &mindist_x, tx);
      snapped |= clay_image_snap_distance (x, clay_image_get_width (image),
                                           epsilon_x,
                                           &mindist_x, tx);

      snapped |= clay_image_snap_distance (y, 0,
                                           epsilon_y,
                                           &mindist_y, ty);
      snapped |= clay_image_snap_distance (y, clay_image_get_height (image),
                                           epsilon_y,
                                           &mindist_y, ty);
    }

  if (snap_to_path)
    {
      GList      *selected_path = clay_image_get_selected_paths (image);
      GList      *iter;
      ClayStroke *stroke           = NULL;
      ClayCoords  coords           = { 0, 0, 0, 0, 0 };

      coords.x = x;
      coords.y = y;

      for (iter = selected_path; iter; iter = iter->next)
        {
          ClayPath *path = iter->data;

          while ((stroke = clay_path_stroke_get_next (path, stroke)))
            {
              ClayCoords nearest;

              if (clay_stroke_nearest_point_get (stroke, &coords, 1.0,
                                                 &nearest,
                                                 NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (x, nearest.x,
                                                       epsilon_x,
                                                       &mindist_x, tx);
                  snapped |= clay_image_snap_distance (y, nearest.y,
                                                       epsilon_y,
                                                       &mindist_y, ty);
                }
            }
        }
    }

  if (snap_to_bbox)
    {
      GList  *layers_list = clay_image_get_layer_list (image);
      gdouble gcx, gcy;
      gint    gx, gy, gw, gh;

      for (GList *iter = layers_list; iter; iter = iter->next)
        {
          if (!clay_item_is_visible (iter->data))
            continue;

          clay_item_bounds (iter->data, &gx, &gy, &gw, &gh);
          clay_item_get_offset (iter->data, &gx, &gy);
          gcx = (gdouble) gx + (gdouble) gw/2.0;
          gcy = (gdouble) gy + (gdouble) gh/2.0;

          if ((gint) x >= gx && (gint) x <= (gx+gw))
            {
              snapped |= clay_image_snap_distance (x, (gdouble) gx,
                                                   epsilon_x,
                                                   &mindist_x, tx);

              snapped |= clay_image_snap_distance (x, (gdouble) gx+gw,
                                                   epsilon_x,
                                                   &mindist_x, tx);

              snapped |= clay_image_snap_distance (x, gcx,
                                                   epsilon_x,
                                                   &mindist_x, tx);
            }

          if ((gint) y >= gy && (gint) y <= (gy+gh))
            {
              snapped |= clay_image_snap_distance (y, (gdouble) gy,
                                                   epsilon_y,
                                                   &mindist_y, ty);

              snapped |= clay_image_snap_distance (y, (gdouble) gy+gh,
                                                   epsilon_y,
                                                   &mindist_y, ty);

              snapped |= clay_image_snap_distance (y, gcy,
                                                   epsilon_y,
                                                   &mindist_y, ty);
            }
        }

      g_list_free (layers_list);
    }

  return snapped;
}

gboolean
clay_image_snap_rectangle (ClayImage         *image,
                           ClayImageSnapData *snapping_data,
                           gdouble            x1,
                           gdouble            y1,
                           gdouble            x2,
                           gdouble            y2,
                           gdouble           *tx1,
                           gdouble           *ty1,
                           gdouble            epsilon_x,
                           gdouble            epsilon_y,
                           gboolean           snap_to_guides,
                           gboolean           snap_to_grid,
                           gboolean           snap_to_canvas,
                           gboolean           snap_to_path,
                           gboolean           snap_to_bbox,
                           gboolean           snap_to_equidistance)
{
  gdouble           nx, ny;
  gdouble           mindist_x  = G_MAXDOUBLE;
  gdouble           mindist_y  = G_MAXDOUBLE;
  gdouble           mindist_t  = G_MAXDOUBLE;
  gdouble           mindist_tx = G_MAXDOUBLE;
  gdouble           mindist_ty = G_MAXDOUBLE;
  gdouble           x_center   = (x1 + x2) / 2.0;
  gdouble           y_center   = (y1 + y2) / 2.0;
  gboolean          snapped    = FALSE;
  ClayAlignmentType alignment_side;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (tx1 != NULL, FALSE);
  g_return_val_if_fail (ty1 != NULL, FALSE);

  *tx1 = x1;
  *ty1 = y1;

  if (! clay_image_get_guides (image))
    snap_to_guides  = FALSE;
  if (! clay_image_get_grid (image))
    snap_to_grid    = FALSE;
  if (! clay_image_get_selected_paths (image))
    snap_to_path = FALSE;

  if (! (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_path || snap_to_bbox || snap_to_equidistance))
    return FALSE;

  /*  center, vertical  */
  if (clay_image_snap_x (image, snapping_data, x_center, &nx,
                         MIN (epsilon_x, mindist_x),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas,
                         snap_to_bbox,
                         FALSE,
                         CLAY_ALIGN_VCENTER))
    {
      mindist_x = ABS (nx - x_center);
      *tx1 = RINT (x1 + (nx - x_center));
      snapped = TRUE;
    }

  /*  left edge  */
  if (clay_image_snap_x (image, snapping_data, x1, &nx,
                         MIN (epsilon_x, mindist_x),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas,
                         snap_to_bbox,
                         snap_to_equidistance,
                         CLAY_ALIGN_LEFT))
    {
      mindist_x = ABS (nx - x1);
      mindist_t = mindist_x;
      *tx1 = nx;
      snapped = TRUE;
    }

  alignment_side = snapping_data->equidistance_side_horizontal;

  if (mindist_t > mindist_x && alignment_side == CLAY_ALIGN_VCENTER)
    snapping_data->equidistance_side_horizontal = CLAY_ARRANGE_HFILL;

  mindist_t = mindist_x;

  /*  right edge  */
  if (clay_image_snap_x (image, snapping_data, x2, &nx,
                         MIN (epsilon_x, mindist_x),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas,
                         snap_to_bbox,
                         snap_to_equidistance,
                         CLAY_ALIGN_RIGHT))
    {
      mindist_x = ABS (nx - x2);
      *tx1 = RINT (x1 + (nx - x2));
      snapped = TRUE;
    }

  alignment_side = snapping_data->equidistance_side_horizontal;
  if (mindist_t > mindist_x && (alignment_side == CLAY_ALIGN_LEFT || alignment_side == CLAY_ALIGN_VCENTER))
    snapping_data->equidistance_side_horizontal = CLAY_ARRANGE_HFILL;

  mindist_t = G_MAXDOUBLE;

  /*  center, horizontal  */
  if (clay_image_snap_y (image, snapping_data, y_center, &ny,
                         MIN (epsilon_y, mindist_y),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas,
                         snap_to_bbox,
                         FALSE,
                         CLAY_ALIGN_HCENTER))
    {
      mindist_y = ABS (ny - y_center);
      *ty1 = RINT (y1 + (ny - y_center));
      snapped = TRUE;
    }

  /*  top edge  */
  if (clay_image_snap_y (image, snapping_data, y1, &ny,
                         MIN (epsilon_y, mindist_y),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas,
                         snap_to_bbox,
                         snap_to_equidistance,
                         CLAY_ALIGN_TOP))
    {
      mindist_y = ABS (ny - y1);
      mindist_t = mindist_y;
      *ty1 = ny;
      snapped = TRUE;
    }

  alignment_side = snapping_data->equidistance_side_vertical;

  if (mindist_t > mindist_y && alignment_side == CLAY_ALIGN_HCENTER)
    snapping_data->equidistance_side_vertical = CLAY_ARRANGE_HFILL;

  mindist_t = mindist_y;

  /*  bottom edge  */
  if (clay_image_snap_y (image, snapping_data, y2, &ny,
                         MIN (epsilon_y, mindist_y),
                         snap_to_guides,
                         snap_to_grid,
                         snap_to_canvas,
                         snap_to_bbox,
                         snap_to_equidistance,
                         CLAY_ALIGN_BOTTOM))
    {
      mindist_y = ABS (ny - y2);
      *ty1 = RINT (y1 + (ny - y2));
      snapped = TRUE;
    }

  alignment_side = snapping_data->equidistance_side_vertical;
  if (mindist_t > mindist_y && (alignment_side == CLAY_ALIGN_HCENTER || alignment_side == CLAY_ALIGN_TOP))
    snapping_data->equidistance_side_vertical = CLAY_ARRANGE_HFILL;

  mindist_tx = mindist_x;
  mindist_ty = mindist_y;

  if (snap_to_path)
    {
      GList      *selected_path = clay_image_get_selected_paths (image);
      GList      *iter;
      ClayStroke *stroke           = NULL;
      ClayCoords  coords1          = CLAY_COORDS_DEFAULT_VALUES;
      ClayCoords  coords2          = CLAY_COORDS_DEFAULT_VALUES;

      for (iter = selected_path; iter; iter = iter->next)
        {
          ClayPath *path = iter->data;

          while ((stroke = clay_path_stroke_get_next (path, stroke)))
            {
              ClayCoords nearest;
              gdouble    dist;

              /*  top edge  */

              coords1.x = x1;
              coords1.y = y1;
              coords2.x = x2;
              coords2.y = y1;

              if (clay_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                                   1.0, &nearest,
                                                   NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (y1, nearest.y,
                                                       epsilon_y,
                                                       &mindist_y, ty1);
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (x1, nearest.x,
                                                       epsilon_x,
                                                       &mindist_x, tx1);
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  dist = ABS (nearest.x - x2);

                  if (dist < MIN (epsilon_x, mindist_x))
                    {
                      mindist_x = dist;
                      *tx1 = RINT (x1 + (nearest.x - x2));
                      snapped = TRUE;
                    }
                }

              /*  bottom edge  */

              coords1.x = x1;
              coords1.y = y2;
              coords2.x = x2;
              coords2.y = y2;

              if (clay_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                                   1.0, &nearest,
                                                   NULL, NULL, NULL) >= 0)
                {
                  dist = ABS (nearest.y - y2);

                  if (dist < MIN (epsilon_y, mindist_y))
                    {
                      mindist_y = dist;
                      *ty1 = RINT (y1 + (nearest.y - y2));
                      snapped = TRUE;
                    }
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (x1, nearest.x,
                                                       epsilon_x,
                                                       &mindist_x, tx1);
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  dist = ABS (nearest.x - x2);

                  if (dist < MIN (epsilon_x, mindist_x))
                    {
                      mindist_x = dist;
                      *tx1 = RINT (x1 + (nearest.x - x2));
                      snapped = TRUE;
                    }
                }

              /*  left edge  */

              coords1.x = x1;
              coords1.y = y1;
              coords2.x = x1;
              coords2.y = y2;

              if (clay_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                                   1.0, &nearest,
                                                   NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (x1, nearest.x,
                                                       epsilon_x,
                                                       &mindist_x, tx1);
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (y1, nearest.y,
                                                       epsilon_y,
                                                       &mindist_y, ty1);
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  dist = ABS (nearest.y - y2);

                  if (dist < MIN (epsilon_y, mindist_y))
                    {
                      mindist_y = dist;
                      *ty1 = RINT (y1 + (nearest.y - y2));
                      snapped = TRUE;
                    }
                }

              /*  right edge  */

              coords1.x = x2;
              coords1.y = y1;
              coords2.x = x2;
              coords2.y = y2;

              if (clay_stroke_nearest_tangent_get (stroke, &coords1, &coords2,
                                                   1.0, &nearest,
                                                   NULL, NULL, NULL) >= 0)
                {
                  dist = ABS (nearest.x - x2);

                  if (dist < MIN (epsilon_x, mindist_x))
                    {
                      mindist_x = dist;
                      *tx1 = RINT (x1 + (nearest.x - x2));
                      snapped = TRUE;
                    }
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords1, &coords2,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  snapped |= clay_image_snap_distance (y1, nearest.y,
                                                       epsilon_y,
                                                       &mindist_y, ty1);
                }

              if (clay_stroke_nearest_intersection_get (stroke, &coords2, &coords1,
                                                        1.0, &nearest,
                                                        NULL, NULL, NULL) >= 0)
                {
                  dist = ABS (nearest.y - y2);

                  if (dist < MIN (epsilon_y, mindist_y))
                    {
                      mindist_y = dist;
                      *ty1 = RINT (y1 + (nearest.y - y2));
                      snapped = TRUE;
                    }
                }

              /*  center  */

              coords1.x = x_center;
              coords1.y = y_center;

              if (clay_stroke_nearest_point_get (stroke, &coords1, 1.0,
                                                 &nearest,
                                                 NULL, NULL, NULL) >= 0)
                {
                  if (clay_image_snap_distance (x_center, nearest.x,
                                                epsilon_x,
                                                &mindist_x, &nx))
                    {
                      mindist_x = ABS (nx - x_center);
                      *tx1 = RINT (x1 + (nx - x_center));
                      snapped = TRUE;
                    }

                  if (clay_image_snap_distance (y_center, nearest.y,
                                                epsilon_y,
                                                &mindist_y, &ny))
                    {
                      mindist_y = ABS (ny - y_center);
                      *ty1 = RINT (y1 + (ny - y_center));
                      snapped = TRUE;
                    }
                }
            }
        }

      if (ROUND (mindist_y) < ROUND (mindist_ty))
        {
          snapping_data->equidistance_side_vertical = CLAY_ARRANGE_HFILL;
          snapping_data->snapped_side_vertical = CLAY_ARRANGE_HFILL;
        }

      if (ROUND (mindist_x) < ROUND (mindist_tx))
        {
          snapping_data->equidistance_side_horizontal = CLAY_ARRANGE_HFILL;
          snapping_data->snapped_side_horizontal = CLAY_ARRANGE_HFILL;
        }
    }

  return snapped;
}


/* private functions */

/**
 * clay_image_snap_distance:
 * @unsnapped: One coordinate of the unsnapped position
 * @nearest:  One coordinate of a snapping position candidate
 * @epsilon:  The snapping threshold
 * @mindist:  The distance to the currently closest snapping target
 * @target:   The currently closest snapping target
 *
 * Finds out if snapping occurs from position to a snapping candidate
 * and sets the target accordingly.
 *
 * Returns: %TRUE if snapping occurred, %FALSE otherwise
 */
static gboolean
clay_image_snap_distance (const gdouble  unsnapped,
                          const gdouble  nearest,
                          const gdouble  epsilon,
                          gdouble       *mindist,
                          gdouble       *target)
{
  const gdouble dist = ABS (nearest - unsnapped);

  if (dist < MIN (epsilon, *mindist))
    {
      *mindist = dist;
      *target = nearest;

      return TRUE;
    }

  return FALSE;
}
