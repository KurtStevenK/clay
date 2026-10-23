/* xyz.kainzmayer.ClayDesign
 *
 * claycageconfig.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"

#include "operations-types.h"

#include "claycageconfig.h"


/*#define DEBUG_CAGE */

/* This DELTA is aimed to not have handle on exact pixel during computation,
 * to avoid particular case. It shouldn't be so useful, but it's a double
 * safety. */
#define DELTA             0.010309278351


static void   clay_cage_config_finalize               (GObject        *object);
static void   clay_cage_config_get_property           (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void   clay_cage_config_set_property           (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);

static void   clay_cage_config_compute_scaling_factor (ClayCageConfig *gcc);
static void   clay_cage_config_compute_edges_normal   (ClayCageConfig *gcc);


G_DEFINE_TYPE_WITH_CODE (ClayCageConfig, clay_cage_config,
                         CLAY_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                NULL))

#define parent_class clay_cage_config_parent_class

#ifdef DEBUG_CAGE
static void
print_cage (ClayCageConfig *gcc)
{
  gint i;
  GeglRectangle bounding_box;
  ClayCagePoint *point;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  bounding_box = clay_cage_config_get_bounding_box (gcc);

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, ClayCagePoint, i);
      g_printerr ("cgx: %.0f    cgy: %.0f    cvdx: %.0f    cvdy: %.0f  sf: %.2f  normx: %.2f  normy: %.2f %s\n",
                  point->src_point.x + ((gcc->cage_mode==CLAY_CAGE_MODE_CAGE_CHANGE)?gcc->displacement_x:0),
                  point->src_point.y + ((gcc->cage_mode==CLAY_CAGE_MODE_CAGE_CHANGE)?gcc->displacement_y:0),
                  point->dest_point.x + ((gcc->cage_mode==CLAY_CAGE_MODE_DEFORM)?gcc->displacement_x:0),
                  point->dest_point.y + ((gcc->cage_mode==CLAY_CAGE_MODE_DEFORM)?gcc->displacement_y:0),
                  point->edge_scaling_factor,
                  point->edge_normal.x,
                  point->edge_normal.y,
                  ((point->selected) ? "S" : "NS"));
    }

  g_printerr ("bounding box: x: %d  y: %d  width: %d  height: %d\n", bounding_box.x, bounding_box.y, bounding_box.width, bounding_box.height);
  g_printerr ("disp x: %f  disp y: %f\n", gcc->displacement_x, gcc->displacement_y);
  g_printerr ("done\n");
}
#endif

static void
clay_cage_config_class_init (ClayCageConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_cage_config_set_property;
  object_class->get_property = clay_cage_config_get_property;

  object_class->finalize     = clay_cage_config_finalize;
}

static void
clay_cage_config_init (ClayCageConfig *self)
{
  /*pre-allocation for 50 vertices for the cage.*/
  self->cage_points = g_array_sized_new (FALSE, FALSE, sizeof(ClayCagePoint), 50);
}

static void
clay_cage_config_finalize (GObject *object)
{
  ClayCageConfig *gcc = CLAY_CAGE_CONFIG (object);

  g_array_free (gcc->cage_points, TRUE);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_cage_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_cage_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * clay_cage_config_get_n_points:
 * @gcc: the cage config
 *
 * Returns: the number of points of the cage
 */
guint
clay_cage_config_get_n_points (ClayCageConfig *gcc)
{
  return gcc->cage_points->len;
}

/**
 * clay_cage_config_add_cage_point:
 * @gcc: the cage config
 * @x: x value of the new point
 * @y: y value of the new point
 *
 * Add a new point in the last index of the polygon of the cage.
 * Point is added in both source and destination cage
 */
void
clay_cage_config_add_cage_point (ClayCageConfig  *gcc,
                                 gdouble          x,
                                 gdouble          y)
{
  clay_cage_config_insert_cage_point (gcc, gcc->cage_points->len, x, y);
}

/**
 * clay_cage_config_insert_cage_point:
 * @gcc: the cage config
 * @point_number: index where the point will be inserted
 * @x: x value of the new point
 * @y: y value of the new point
 *
 * Insert a new point in the polygon of the cage at the given index.
 * Point is added in both source and destination cage
 */
void
clay_cage_config_insert_cage_point (ClayCageConfig  *gcc,
                                    gint             point_number,
                                    gdouble          x,
                                    gdouble          y)
{
  ClayCagePoint point;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number <= gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  point.src_point.x = x + DELTA;
  point.src_point.y = y + DELTA;

  point.dest_point.x = x + DELTA;
  point.dest_point.y = y + DELTA;

  g_array_insert_val (gcc->cage_points, point_number, point);

  clay_cage_config_compute_scaling_factor (gcc);
  clay_cage_config_compute_edges_normal (gcc);
}

/**
 * clay_cage_config_remove_last_cage_point:
 * @gcc: the cage config
 *
 * Remove the last point of the cage, in both source and destination cage
 */
void
clay_cage_config_remove_last_cage_point (ClayCageConfig  *gcc)
{
  clay_cage_config_remove_cage_point (gcc, gcc->cage_points->len - 1);
}

/**
 * clay_cage_config_remove_cage_point:
 * @gcc: the cage config
 * @point_number: the index of the point to remove
 *
 * Remove the given point from the cage
 */
void
clay_cage_config_remove_cage_point (ClayCageConfig *gcc,
                                    gint            point_number)
{
  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  if (gcc->cage_points->len > 0)
    g_array_remove_index (gcc->cage_points, gcc->cage_points->len - 1);

  clay_cage_config_compute_scaling_factor (gcc);
  clay_cage_config_compute_edges_normal (gcc);
}

/**
 * clay_cage_config_remove_selected_points:
 * @gcc: the cage config
 *
 * Remove all the selected points from the cage
 */
void
clay_cage_config_remove_selected_points (ClayCageConfig  *gcc)
{
  gint           i;
  ClayCagePoint *point;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, ClayCagePoint, i);

      if (point->selected)
        {
          g_array_remove_index (gcc->cage_points, i);
          i--;
        }
    }

  clay_cage_config_compute_scaling_factor (gcc);
  clay_cage_config_compute_edges_normal (gcc);
}

/**
 * clay_cage_config_get_point_coordinate:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, CLAY_CAGE_MODE_CAGE_CHANGE or CLAY_CAGE_MODE_DEFORM
 * @point_number: the index of the point to return
 *
 * Returns: the real position of the given point, as a ClayVector2
 */
ClayVector2
clay_cage_config_get_point_coordinate (ClayCageConfig *gcc,
                                       ClayCageMode    mode,
                                       gint            point_number)
{
  ClayVector2     result = { 0.0, 0.0 };
  ClayCagePoint  *point;

  g_return_val_if_fail (CLAY_IS_CAGE_CONFIG (gcc), result);
  g_return_val_if_fail (point_number < gcc->cage_points->len, result);
  g_return_val_if_fail (point_number >= 0, result);

  point = &g_array_index (gcc->cage_points, ClayCagePoint, point_number);

  if (point->selected)
    {
      if (mode == CLAY_CAGE_MODE_CAGE_CHANGE)
        {
          result.x = point->src_point.x + gcc->displacement_x;
          result.y = point->src_point.y + gcc->displacement_y;
        }
      else
        {
          result.x = point->dest_point.x + gcc->displacement_x;
          result.y = point->dest_point.y + gcc->displacement_y;
        }
    }
  else
    {
      if (mode == CLAY_CAGE_MODE_CAGE_CHANGE)
        {
          result.x = point->src_point.x;
          result.y = point->src_point.y;
        }
      else
        {
          result.x = point->dest_point.x;
          result.y = point->dest_point.y;
        }
    }

  return result;
}

/**
 * clay_cage_config_add_displacement:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, CLAY_CAGE_MODE_CAGE_CHANGE or CLAY_CAGE_MODE_DEFORM
 * @point_number: the point of the cage to move
 * @x: x displacement value
 * @y: y displacement value
 *
 * Add a displacement for all selected points of the cage.
 * This displacement need to be committed to become effective.
 */
void
clay_cage_config_add_displacement (ClayCageConfig *gcc,
                                   ClayCageMode    mode,
                                   gdouble         x,
                                   gdouble         y)
{
  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  gcc->cage_mode = mode;
  gcc->displacement_x = x;
  gcc->displacement_y = y;

  #ifdef DEBUG_CAGE
    print_cage (gcc);
  #endif
}

/**
 * clay_cage_config_commit_displacement:
 * @gcc: the cage config
 *
 * Apply the displacement to the cage
 */
void
clay_cage_config_commit_displacement (ClayCageConfig *gcc)
{
  gint  i;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      ClayCagePoint *point;
      point = &g_array_index (gcc->cage_points, ClayCagePoint, i);

      if (point->selected)
        {
          if (gcc->cage_mode == CLAY_CAGE_MODE_CAGE_CHANGE)
            {
              point->src_point.x += gcc->displacement_x;
              point->src_point.y += gcc->displacement_y;
              point->dest_point.x += gcc->displacement_x;
              point->dest_point.y += gcc->displacement_y;
            }
          else
            {
              point->dest_point.x += gcc->displacement_x;
              point->dest_point.y += gcc->displacement_y;
            }
        }
    }

  clay_cage_config_compute_scaling_factor (gcc);
  clay_cage_config_compute_edges_normal (gcc);
  clay_cage_config_reset_displacement (gcc);
}

/**
 * clay_cage_config_reset_displacement:
 * @gcc: the cage config
 *
 * Set the displacement to zero.
 */
void
clay_cage_config_reset_displacement (ClayCageConfig *gcc)
{
  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  gcc->displacement_x = 0.0;
  gcc->displacement_y = 0.0;
}

/**
 * clay_cage_config_get_bounding_box:
 * @gcc: the cage config
 *
 * Compute the bounding box of the source cage
 *
 * Returns: the bounding box of the source cage, as a GeglRectangle
 */
GeglRectangle
clay_cage_config_get_bounding_box (ClayCageConfig *gcc)
{
  GeglRectangle  bounding_box = { 0, 0, 0, 0};
  gint           i;
  ClayCagePoint *point;

  g_return_val_if_fail (CLAY_IS_CAGE_CONFIG (gcc), bounding_box);

  if (gcc->cage_points->len == 0)
    return bounding_box;

  point = &g_array_index (gcc->cage_points, ClayCagePoint, 0);

  if (point->selected)
    {
      bounding_box.x = point->src_point.x + gcc->displacement_x;
      bounding_box.y = point->src_point.y + gcc->displacement_y;
    }
  else
    {
      bounding_box.x = point->src_point.x;
      bounding_box.y = point->src_point.y;
    }

  for (i = 1; i < gcc->cage_points->len; i++)
    {
      gdouble x,y;
      point = &g_array_index (gcc->cage_points, ClayCagePoint, i);

      if (point->selected)
        {
          x = point->src_point.x + gcc->displacement_x;
          y = point->src_point.y + gcc->displacement_y;
        }
      else
        {
          x = point->src_point.x;
          y = point->src_point.y;
        }

      if (x < bounding_box.x)
        {
          bounding_box.width += bounding_box.x - x;
          bounding_box.x = x;
        }

      if (y < bounding_box.y)
        {
          bounding_box.height += bounding_box.y - y;
          bounding_box.y = y;
        }

      if (x > bounding_box.x + bounding_box.width)
        {
          bounding_box.width = x - bounding_box.x;
        }

      if (y > bounding_box.y + bounding_box.height)
        {
          bounding_box.height = y - bounding_box.y;
        }
    }

  return bounding_box;
}

/**
 * clay_cage_config_reverse_cage:
 * @gcc: the cage config
 *
 * When using non-simple cage (like a cage in 8), user may want to
 * manually inverse inside and outside of the cage. This function
 * reverse the cage
 */
void
clay_cage_config_reverse_cage (ClayCageConfig *gcc)
{
  ClayCagePoint temp;
  gint          i;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len / 2; i++)
    {
      temp = g_array_index (gcc->cage_points, ClayCagePoint, i);

      g_array_index (gcc->cage_points, ClayCagePoint, i) =
        g_array_index (gcc->cage_points, ClayCagePoint, gcc->cage_points->len - i - 1);

      g_array_index (gcc->cage_points, ClayCagePoint, gcc->cage_points->len - i - 1) = temp;
    }

  clay_cage_config_compute_scaling_factor (gcc);
  clay_cage_config_compute_edges_normal (gcc);
}

/**
 * clay_cage_config_reverse_cage_if_needed:
 * @gcc: the cage config
 *
 * Since the cage need to be defined counter-clockwise to have the
 * topological inside in the actual 'physical' inside of the cage,
 * this function compute if the cage is clockwise or not, and reverse
 * the cage if needed.
 *
 * This function does not take into account an eventual displacement
 */
void
clay_cage_config_reverse_cage_if_needed (ClayCageConfig *gcc)
{
  gint    i;
  gdouble sum;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  sum = 0.0;

  /* this is a bit crappy, but should works most of the case */
  /* we do the sum of the projection of each point to the previous
     segment, and see the final sign */
  for (i = 0; i < gcc->cage_points->len ; i++)
    {
      ClayVector2 P1, P2, P3;
      gdouble     z;

      P1 = (g_array_index (gcc->cage_points, ClayCagePoint, i)).src_point;
      P2 = (g_array_index (gcc->cage_points, ClayCagePoint, (i+1) % gcc->cage_points->len)).src_point;
      P3 = (g_array_index (gcc->cage_points, ClayCagePoint, (i+2) % gcc->cage_points->len)).src_point;

      z = P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y);

      sum += z;
    }

  /* sum > 0 mean a cage defined counter-clockwise, so we reverse it */
  if (sum > 0)
    {
      clay_cage_config_reverse_cage (gcc);
    }
}

/**
 * clay_cage_config_compute_scaling_factor:
 * @gcc: the cage config
 *
 * Update Green Coordinate scaling factor for the destination cage.
 * This function does not take into account an eventual displacement.
 */
static void
clay_cage_config_compute_scaling_factor (ClayCageConfig *gcc)
{
  ClayVector2    edge;
  gdouble        length, length_d;
  gint           i;
  ClayCagePoint *current, *last;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));
  if (gcc->cage_points->len < 2)
    return;

  last = &g_array_index (gcc->cage_points, ClayCagePoint, 0);

  for (i = 1; i <= gcc->cage_points->len; i++)
    {
      current = &g_array_index (gcc->cage_points, ClayCagePoint, i % gcc->cage_points->len);

      clay_vector2_sub (&edge,
                        &(last->src_point),
                        &(current->src_point));
      length = clay_vector2_length (&edge);

      clay_vector2_sub (&edge,
                        &(last->dest_point),
                        &(current->dest_point));
      length_d = clay_vector2_length (&edge);

      last->edge_scaling_factor = length_d / length;
      last = current;
    }
}

/**
 * clay_cage_config_compute_edges_normal:
 * @gcc: the cage config
 *
 * Update edges normal for the destination cage.
 * This function does not take into account an eventual displacement.
 */
static void
clay_cage_config_compute_edges_normal (ClayCageConfig *gcc)
{
  ClayVector2    normal;
  gint           i;
  ClayCagePoint *current, *last;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  last = &g_array_index (gcc->cage_points, ClayCagePoint, 0);

  for (i = 1; i <= gcc->cage_points->len; i++)
    {
      current = &g_array_index (gcc->cage_points, ClayCagePoint, i % gcc->cage_points->len);

      clay_vector2_sub (&normal,
                        &(current->dest_point),
                        &(last->dest_point));

      last->edge_normal = clay_vector2_normal (&normal);
      last = current;
    }
}

/**
 * clay_cage_config_point_inside:
 * @gcc: the cage config
 * @x: x coordinate of the point to test
 * @y: y coordinate of the point to test
 *
 * Check if the given point is inside the cage. This test is done in
 * the regard of the topological inside of the source cage.
 *
 * Returns: TRUE if the point is inside, FALSE if not.
 * This function does not take into account an eventual displacement.
 */
gboolean
clay_cage_config_point_inside (ClayCageConfig *gcc,
                               gfloat          x,
                               gfloat          y)
{
  ClayVector2   *last, *current;
  gboolean       inside = FALSE;
  gint           i;

  g_return_val_if_fail (CLAY_IS_CAGE_CONFIG (gcc), FALSE);

  last = &((g_array_index (gcc->cage_points, ClayCagePoint, gcc->cage_points->len - 1)).src_point);

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      current = &((g_array_index (gcc->cage_points, ClayCagePoint, i)).src_point);

      if ((((current->y <= y) && (y < last->y))
           || ((last->y <= y) && (y < current->y)))
          && (x < (last->x - current->x) * (y - current->y) / (last->y - current->y) + current->x))
        {
          inside = !inside;
        }

      last = current;
    }

  return inside;
}

/**
 * clay_cage_config_select_point:
 * @gcc: the cage config
 * @point_number: the index of the point to select
 *
 * Select the given point of the cage, and deselect the others.
 */
void
clay_cage_config_select_point (ClayCageConfig  *gcc,
                               gint             point_number)
{
  gint           i;
  ClayCagePoint *point;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, ClayCagePoint, i);

      if (i == point_number)
        {
          point->selected = TRUE;
        }
      else
        {
          point->selected = FALSE;
        }
    }
}

/**
 * clay_cage_config_select_area:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, CLAY_CAGE_MODE_CAGE_CHANGE or CLAY_CAGE_MODE_DEFORM
 * @area: the area to select
 *
 * Select cage's point inside the given area and deselect others
 */
void
clay_cage_config_select_area  (ClayCageConfig  *gcc,
                               ClayCageMode     mode,
                               GeglRectangle    area)
{
  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  clay_cage_config_deselect_points (gcc);
  clay_cage_config_select_add_area (gcc, mode, area);
}

/**
 * clay_cage_config_select_add_area:
 * @gcc: the cage config
 * @mode: the actual mode of the cage, CLAY_CAGE_MODE_CAGE_CHANGE or CLAY_CAGE_MODE_DEFORM
 * @area: the area to select
 *
 * Select cage's point inside the given area. Already selected point stay selected.
 */
void
clay_cage_config_select_add_area (ClayCageConfig *gcc,
                                  ClayCageMode    mode,
                                  GeglRectangle   area)
{
  gint           i;
  ClayCagePoint *point;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      point = &g_array_index (gcc->cage_points, ClayCagePoint, i);

      if (mode == CLAY_CAGE_MODE_CAGE_CHANGE)
        {
          if (point->src_point.x >= area.x &&
              point->src_point.x <= area.x + area.width &&
              point->src_point.y >= area.y &&
              point->src_point.y <= area.y + area.height)
            {
              point->selected = TRUE;
            }
        }
      else
        {
          if (point->dest_point.x >= area.x &&
              point->dest_point.x <= area.x + area.width &&
              point->dest_point.y >= area.y &&
              point->dest_point.y <= area.y + area.height)
            {
              point->selected = TRUE;
            }
        }
    }
}

/**
 * clay_cage_config_toggle_point_selection:
 * @gcc: the cage config
 * @point_number: the index of the point to toggle selection
 *
 * Toggle the selection of the given cage point
 */
void
clay_cage_config_toggle_point_selection (ClayCageConfig *gcc,
                                         gint            point_number)
{
  ClayCagePoint *point;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));
  g_return_if_fail (point_number < gcc->cage_points->len);
  g_return_if_fail (point_number >= 0);

  point = &g_array_index (gcc->cage_points, ClayCagePoint, point_number);
  point->selected = ! point->selected;
}

/**
 * clay_cage_deselect_points:
 * @gcc: the cage config
 *
 * Deselect all cage points.
 */
void
clay_cage_config_deselect_points (ClayCageConfig *gcc)
{
  gint  i;

  g_return_if_fail (CLAY_IS_CAGE_CONFIG (gcc));

  for (i = 0; i < gcc->cage_points->len; i++)
    {
      (g_array_index (gcc->cage_points, ClayCagePoint, i)).selected = FALSE;
    }
}

/**
 * clay_cage_config_point_is_selected:
 * @gcc: the cage config
 * @point_number: the index of the point to test
 *
 * Returns: TRUE if the point is selected, FALSE otherwise.
 */
gboolean
clay_cage_config_point_is_selected (ClayCageConfig  *gcc,
                                    gint             point_number)
{
  ClayCagePoint *point;

  g_return_val_if_fail (CLAY_IS_CAGE_CONFIG (gcc), FALSE);
  g_return_val_if_fail (point_number < gcc->cage_points->len, FALSE);
  g_return_val_if_fail (point_number >= 0, FALSE);

  point = &(g_array_index (gcc->cage_points, ClayCagePoint, point_number));

  return point->selected;
}
