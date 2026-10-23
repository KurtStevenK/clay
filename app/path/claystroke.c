/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claystroke.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "path-types.h"

#include "core/clay-memsize.h"
#include "core/claycoords.h"
#include "core/clayparamspecs.h"
#include "core/clay-transform-utils.h"

#include "clayanchor.h"
#include "claystroke.h"

enum
{
  PROP_0,
  PROP_CONTROL_POINTS,
  PROP_CLOSED
};

/* Prototypes */

static void    clay_stroke_set_property              (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void    clay_stroke_get_property              (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void    clay_stroke_finalize                  (GObject      *object);

static gint64  clay_stroke_get_memsize               (ClayObject   *object,
                                                      gint64       *gui_size);

static ClayAnchor * clay_stroke_real_anchor_get      (ClayStroke       *stroke,
                                                      const ClayCoords *coord);
static ClayAnchor * clay_stroke_real_anchor_get_next (ClayStroke       *stroke,
                                                      const ClayAnchor *prev);
static void         clay_stroke_real_anchor_select   (ClayStroke       *stroke,
                                                      ClayAnchor       *anchor,
                                                      gboolean          selected,
                                                      gboolean          exclusive);
static void    clay_stroke_real_anchor_move_relative (ClayStroke       *stroke,
                                                      ClayAnchor       *anchor,
                                                      const ClayCoords *delta,
                                                      ClayAnchorFeatureType feature);
static void    clay_stroke_real_anchor_move_absolute (ClayStroke       *stroke,
                                                      ClayAnchor       *anchor,
                                                      const ClayCoords *delta,
                                                      ClayAnchorFeatureType feature);
static void         clay_stroke_real_anchor_convert  (ClayStroke       *stroke,
                                                      ClayAnchor       *anchor,
                                                      ClayAnchorFeatureType  feature);
static void         clay_stroke_real_anchor_delete   (ClayStroke       *stroke,
                                                      ClayAnchor       *anchor);
static gboolean     clay_stroke_real_point_is_movable
                                              (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position);
static void         clay_stroke_real_point_move_relative
                                              (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position,
                                               const ClayCoords      *deltacoord,
                                               ClayAnchorFeatureType  feature);
static void         clay_stroke_real_point_move_absolute
                                              (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position,
                                               const ClayCoords      *coord,
                                               ClayAnchorFeatureType  feature);

static void         clay_stroke_real_close           (ClayStroke       *stroke);
static ClayStroke * clay_stroke_real_open            (ClayStroke       *stroke,
                                                      ClayAnchor       *end_anchor);
static gboolean     clay_stroke_real_anchor_is_insertable
                                                     (ClayStroke       *stroke,
                                                      ClayAnchor       *predec,
                                                      gdouble           position);
static ClayAnchor * clay_stroke_real_anchor_insert   (ClayStroke       *stroke,
                                                      ClayAnchor       *predec,
                                                      gdouble           position);

static gboolean     clay_stroke_real_is_extendable   (ClayStroke       *stroke,
                                                      ClayAnchor       *neighbor);

static ClayAnchor * clay_stroke_real_extend (ClayStroke           *stroke,
                                             const ClayCoords     *coords,
                                             ClayAnchor           *neighbor,
                                             ClayStrokeExtendMode  extend_mode);

gboolean     clay_stroke_real_connect_stroke (ClayStroke          *stroke,
                                              ClayAnchor          *anchor,
                                              ClayStroke          *extension,
                                              ClayAnchor          *neighbor);


static gboolean     clay_stroke_real_is_empty        (ClayStroke       *stroke);
static gboolean     clay_stroke_real_reverse         (ClayStroke       *stroke);
static gboolean     clay_stroke_real_shift_start     (ClayStroke       *stroke,
                                                      ClayAnchor       *anchor);

static gdouble      clay_stroke_real_get_length      (ClayStroke       *stroke,
                                                      gdouble           precision);
static gdouble      clay_stroke_real_get_distance    (ClayStroke       *stroke,
                                                      const ClayCoords *coord);
static GArray *     clay_stroke_real_interpolate     (ClayStroke       *stroke,
                                                      gdouble           precision,
                                                      gboolean         *closed);
static ClayStroke * clay_stroke_real_duplicate       (ClayStroke       *stroke);
static ClayBezierDesc * clay_stroke_real_make_bezier (ClayStroke       *stroke);

static void         clay_stroke_real_translate       (ClayStroke       *stroke,
                                                      gdouble           offset_x,
                                                      gdouble           offset_y);
static void         clay_stroke_real_scale           (ClayStroke       *stroke,
                                                      gdouble           scale_x,
                                                      gdouble           scale_y);
static void         clay_stroke_real_rotate          (ClayStroke       *stroke,
                                                      gdouble           center_x,
                                                      gdouble           center_y,
                                                      gdouble           angle);
static void         clay_stroke_real_flip            (ClayStroke       *stroke,
                                                      ClayOrientationType flip_type,
                                                      gdouble           axis);
static void         clay_stroke_real_flip_free       (ClayStroke       *stroke,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
static void         clay_stroke_real_transform       (ClayStroke        *stroke,
                                                      const ClayMatrix3 *matrix,
                                                      GQueue            *ret_strokes);

static GList    * clay_stroke_real_get_draw_anchors  (ClayStroke       *stroke);
static GList    * clay_stroke_real_get_draw_controls (ClayStroke       *stroke);
static GArray   * clay_stroke_real_get_draw_lines    (ClayStroke       *stroke);
static GArray *  clay_stroke_real_control_points_get (ClayStroke       *stroke,
                                                      gboolean         *ret_closed);
static gboolean   clay_stroke_real_get_point_at_dist (ClayStroke       *stroke,
                                                      gdouble           dist,
                                                      gdouble           precision,
                                                      ClayCoords       *position,
                                                      gdouble          *slope);


G_DEFINE_TYPE (ClayStroke, clay_stroke, CLAY_TYPE_OBJECT)

#define parent_class clay_stroke_parent_class


static void
clay_stroke_class_init (ClayStrokeClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  GParamSpec      *param_spec;

  object_class->finalize          = clay_stroke_finalize;
  object_class->get_property      = clay_stroke_get_property;
  object_class->set_property      = clay_stroke_set_property;

  clay_object_class->get_memsize  = clay_stroke_get_memsize;

  klass->changed                  = NULL;
  klass->removed                  = NULL;

  klass->anchor_get               = clay_stroke_real_anchor_get;
  klass->anchor_get_next          = clay_stroke_real_anchor_get_next;
  klass->anchor_select            = clay_stroke_real_anchor_select;
  klass->anchor_move_relative     = clay_stroke_real_anchor_move_relative;
  klass->anchor_move_absolute     = clay_stroke_real_anchor_move_absolute;
  klass->anchor_convert           = clay_stroke_real_anchor_convert;
  klass->anchor_delete            = clay_stroke_real_anchor_delete;

  klass->point_is_movable         = clay_stroke_real_point_is_movable;
  klass->point_move_relative      = clay_stroke_real_point_move_relative;
  klass->point_move_absolute      = clay_stroke_real_point_move_absolute;

  klass->nearest_point_get        = NULL;
  klass->nearest_tangent_get      = NULL;
  klass->nearest_intersection_get = NULL;
  klass->close                    = clay_stroke_real_close;
  klass->open                     = clay_stroke_real_open;
  klass->anchor_is_insertable     = clay_stroke_real_anchor_is_insertable;
  klass->anchor_insert            = clay_stroke_real_anchor_insert;
  klass->is_extendable            = clay_stroke_real_is_extendable;
  klass->extend                   = clay_stroke_real_extend;
  klass->connect_stroke           = clay_stroke_real_connect_stroke;

  klass->is_empty                 = clay_stroke_real_is_empty;
  klass->reverse                  = clay_stroke_real_reverse;
  klass->shift_start              = clay_stroke_real_shift_start;
  klass->get_length               = clay_stroke_real_get_length;
  klass->get_distance             = clay_stroke_real_get_distance;
  klass->get_point_at_dist        = clay_stroke_real_get_point_at_dist;
  klass->interpolate              = clay_stroke_real_interpolate;

  klass->duplicate                = clay_stroke_real_duplicate;
  klass->make_bezier              = clay_stroke_real_make_bezier;

  klass->translate                = clay_stroke_real_translate;
  klass->scale                    = clay_stroke_real_scale;
  klass->rotate                   = clay_stroke_real_rotate;
  klass->flip                     = clay_stroke_real_flip;
  klass->flip_free                = clay_stroke_real_flip_free;
  klass->transform                = clay_stroke_real_transform;


  klass->get_draw_anchors         = clay_stroke_real_get_draw_anchors;
  klass->get_draw_controls        = clay_stroke_real_get_draw_controls;
  klass->get_draw_lines           = clay_stroke_real_get_draw_lines;
  klass->control_points_get       = clay_stroke_real_control_points_get;

  param_spec = g_param_spec_boxed ("clay-anchor",
                                   "Clay Anchor",
                                   "The control points of a Stroke",
                                   CLAY_TYPE_ANCHOR,
                                   CLAY_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CONTROL_POINTS,
                                   clay_param_spec_value_array ("control-points",
                                                                "Control Points",
                                                                "This is an ValueArray "
                                                                "with the initial "
                                                                "control points of "
                                                                "the new Stroke",
                                                                param_spec,
                                                                CLAY_PARAM_WRITABLE |
                                                                G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         "Close Flag",
                                                         "this flag indicates "
                                                         "whether the stroke "
                                                         "is closed or not",
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_stroke_init (ClayStroke *stroke)
{
  stroke->anchors = g_queue_new ();
}

static void
clay_stroke_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ClayStroke     *stroke = CLAY_STROKE (object);
  ClayValueArray *val_array;
  gint            length;
  gint            i;

  switch (property_id)
    {
    case PROP_CLOSED:
      stroke->closed = g_value_get_boolean (value);
      break;

    case PROP_CONTROL_POINTS:
      g_return_if_fail (g_queue_is_empty (stroke->anchors));
      g_return_if_fail (value != NULL);

      val_array = g_value_get_boxed (value);

      if (val_array == NULL)
        return;

      length = clay_value_array_length (val_array);

      for (i = 0; i < length; i++)
        {
          GValue *item = clay_value_array_index (val_array, i);

          g_return_if_fail (G_VALUE_HOLDS (item, CLAY_TYPE_ANCHOR));
          g_queue_push_tail (stroke->anchors, g_value_dup_boxed (item));
        }

      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_stroke_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ClayStroke *stroke = CLAY_STROKE (object);

  switch (property_id)
    {
    case PROP_CLOSED:
      g_value_set_boolean (value, stroke->closed);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_stroke_finalize (GObject *object)
{
  ClayStroke *stroke = CLAY_STROKE (object);

  g_queue_free_full (stroke->anchors, (GDestroyNotify) clay_anchor_free);
  stroke->anchors = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_stroke_get_memsize (ClayObject *object,
                         gint64     *gui_size)
{
  ClayStroke *stroke  = CLAY_STROKE (object);
  gint64      memsize = 0;

  memsize += clay_g_queue_get_memsize (stroke->anchors, sizeof (ClayAnchor));

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

void
clay_stroke_set_id (ClayStroke *stroke,
                    gint        id)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));
  g_return_if_fail (stroke->id == 0 /* we don't want changing IDs... */);

  stroke->id = id;
}

gint
clay_stroke_get_id (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), -1);

  return stroke->id;
}


ClayAnchor *
clay_stroke_anchor_get (ClayStroke       *stroke,
                        const ClayCoords *coord)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->anchor_get (stroke, coord);
}


gdouble
clay_stroke_nearest_point_get (ClayStroke       *stroke,
                               const ClayCoords *coord,
                               const gdouble     precision,
                               ClayCoords       *ret_point,
                               ClayAnchor      **ret_segment_start,
                               ClayAnchor      **ret_segment_end,
                               gdouble          *ret_pos)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (CLAY_STROKE_GET_CLASS (stroke)->nearest_point_get)
    return CLAY_STROKE_GET_CLASS (stroke)->nearest_point_get (stroke,
                                                              coord,
                                                              precision,
                                                              ret_point,
                                                              ret_segment_start,
                                                              ret_segment_end,
                                                              ret_pos);
  return -1;
}

gdouble
clay_stroke_nearest_tangent_get (ClayStroke            *stroke,
                                 const ClayCoords      *coords1,
                                 const ClayCoords      *coords2,
                                 gdouble                precision,
                                 ClayCoords            *nearest,
                                 ClayAnchor           **ret_segment_start,
                                 ClayAnchor           **ret_segment_end,
                                 gdouble               *ret_pos)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coords1 != NULL, FALSE);
  g_return_val_if_fail (coords2 != NULL, FALSE);

  if (CLAY_STROKE_GET_CLASS (stroke)->nearest_tangent_get)
    return CLAY_STROKE_GET_CLASS (stroke)->nearest_tangent_get (stroke,
                                                                coords1,
                                                                coords2,
                                                                precision,
                                                                nearest,
                                                                ret_segment_start,
                                                                ret_segment_end,
                                                                ret_pos);
  return -1;
}

gdouble
clay_stroke_nearest_intersection_get (ClayStroke        *stroke,
                                      const ClayCoords  *coords1,
                                      const ClayCoords  *direction,
                                      gdouble            precision,
                                      ClayCoords        *nearest,
                                      ClayAnchor       **ret_segment_start,
                                      ClayAnchor       **ret_segment_end,
                                      gdouble           *ret_pos)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (coords1 != NULL, FALSE);
  g_return_val_if_fail (direction != NULL, FALSE);

  if (CLAY_STROKE_GET_CLASS (stroke)->nearest_intersection_get)
    return CLAY_STROKE_GET_CLASS (stroke)->nearest_intersection_get (stroke,
                                                                     coords1,
                                                                     direction,
                                                                     precision,
                                                                     nearest,
                                                                     ret_segment_start,
                                                                     ret_segment_end,
                                                                     ret_pos);
  return -1;
}

static ClayAnchor *
clay_stroke_real_anchor_get (ClayStroke       *stroke,
                             const ClayCoords *coord)
{
  gdouble     dx, dy;
  gdouble     mindist = -1;
  GList      *anchors;
  GList      *list;
  ClayAnchor *anchor = NULL;

  anchors = clay_stroke_get_draw_controls (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - CLAY_ANCHOR (list->data)->position.x;
      dy = coord->y - CLAY_ANCHOR (list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = CLAY_ANCHOR (list->data);
        }
    }

  g_list_free (anchors);

  anchors = clay_stroke_get_draw_anchors (stroke);

  for (list = anchors; list; list = g_list_next (list))
    {
      dx = coord->x - CLAY_ANCHOR (list->data)->position.x;
      dy = coord->y - CLAY_ANCHOR (list->data)->position.y;

      if (mindist < 0 || mindist > dx * dx + dy * dy)
        {
          mindist = dx * dx + dy * dy;
          anchor = CLAY_ANCHOR (list->data);
        }
    }

  g_list_free (anchors);

  return anchor;
}


ClayAnchor *
clay_stroke_anchor_get_next (ClayStroke       *stroke,
                             const ClayAnchor *prev)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->anchor_get_next (stroke, prev);
}

static ClayAnchor *
clay_stroke_real_anchor_get_next (ClayStroke       *stroke,
                                  const ClayAnchor *prev)
{
  GList *list;

  if (prev)
    {
      list = g_queue_find (stroke->anchors, prev);
      if (list)
        list = g_list_next (list);
    }
  else
    {
      list = stroke->anchors->head;
    }

  if (list)
    return CLAY_ANCHOR (list->data);

  return NULL;
}


void
clay_stroke_anchor_select (ClayStroke *stroke,
                           ClayAnchor *anchor,
                           gboolean    selected,
                           gboolean    exclusive)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->anchor_select (stroke, anchor,
                                                 selected, exclusive);
}

static void
clay_stroke_real_anchor_select (ClayStroke *stroke,
                                ClayAnchor *anchor,
                                gboolean    selected,
                                gboolean    exclusive)
{
  GList *list = stroke->anchors->head;

  if (exclusive)
    {
      while (list)
        {
          CLAY_ANCHOR (list->data)->selected = FALSE;
          list = g_list_next (list);
        }
    }

  list = g_queue_find (stroke->anchors, anchor);

  if (list)
    CLAY_ANCHOR (list->data)->selected = selected;
}


void
clay_stroke_anchor_move_relative (ClayStroke            *stroke,
                                  ClayAnchor            *anchor,
                                  const ClayCoords      *delta,
                                  ClayAnchorFeatureType  feature)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_queue_find (stroke->anchors, anchor));

  CLAY_STROKE_GET_CLASS (stroke)->anchor_move_relative (stroke, anchor,
                                                        delta, feature);
}

static void
clay_stroke_real_anchor_move_relative (ClayStroke            *stroke,
                                       ClayAnchor            *anchor,
                                       const ClayCoords      *delta,
                                       ClayAnchorFeatureType  feature)
{
  anchor->position.x += delta->x;
  anchor->position.y += delta->y;
}


void
clay_stroke_anchor_move_absolute (ClayStroke            *stroke,
                                  ClayAnchor            *anchor,
                                  const ClayCoords      *coord,
                                  ClayAnchorFeatureType  feature)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (g_queue_find (stroke->anchors, anchor));

  CLAY_STROKE_GET_CLASS (stroke)->anchor_move_absolute (stroke, anchor,
                                                        coord, feature);
}

static void
clay_stroke_real_anchor_move_absolute (ClayStroke            *stroke,
                                       ClayAnchor            *anchor,
                                       const ClayCoords      *coord,
                                       ClayAnchorFeatureType  feature)
{
  anchor->position.x = coord->x;
  anchor->position.y = coord->y;
}

gboolean
clay_stroke_point_is_movable (ClayStroke *stroke,
                              ClayAnchor *predec,
                              gdouble     position)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->point_is_movable (stroke, predec,
                                                           position);
}


static gboolean
clay_stroke_real_point_is_movable (ClayStroke *stroke,
                                   ClayAnchor *predec,
                                   gdouble     position)
{
  return FALSE;
}


void
clay_stroke_point_move_relative (ClayStroke            *stroke,
                                 ClayAnchor            *predec,
                                 gdouble                position,
                                 const ClayCoords      *deltacoord,
                                 ClayAnchorFeatureType  feature)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->point_move_relative (stroke, predec,
                                                       position, deltacoord,
                                                       feature);
}


static void
clay_stroke_real_point_move_relative (ClayStroke           *stroke,
                                      ClayAnchor           *predec,
                                      gdouble               position,
                                      const ClayCoords     *deltacoord,
                                      ClayAnchorFeatureType feature)
{
  g_printerr ("clay_stroke_point_move_relative: default implementation\n");
}


void
clay_stroke_point_move_absolute (ClayStroke            *stroke,
                                 ClayAnchor            *predec,
                                 gdouble                position,
                                 const ClayCoords      *coord,
                                 ClayAnchorFeatureType  feature)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->point_move_absolute (stroke, predec,
                                                       position, coord,
                                                       feature);
}

static void
clay_stroke_real_point_move_absolute (ClayStroke           *stroke,
                                      ClayAnchor           *predec,
                                      gdouble               position,
                                      const ClayCoords     *coord,
                                      ClayAnchorFeatureType feature)
{
  g_printerr ("clay_stroke_point_move_absolute: default implementation\n");
}


void
clay_stroke_close (ClayStroke *stroke)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));
  g_return_if_fail (g_queue_is_empty (stroke->anchors) == FALSE);

  CLAY_STROKE_GET_CLASS (stroke)->close (stroke);
}

static void
clay_stroke_real_close (ClayStroke *stroke)
{
  stroke->closed = TRUE;
  g_object_notify (G_OBJECT (stroke), "closed");
}


void
clay_stroke_anchor_convert (ClayStroke            *stroke,
                            ClayAnchor            *anchor,
                            ClayAnchorFeatureType  feature)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->anchor_convert (stroke, anchor, feature);
}

static void
clay_stroke_real_anchor_convert (ClayStroke            *stroke,
                                 ClayAnchor            *anchor,
                                 ClayAnchorFeatureType  feature)
{
  g_printerr ("clay_stroke_anchor_convert: default implementation\n");
}


void
clay_stroke_anchor_delete (ClayStroke *stroke,
                           ClayAnchor *anchor)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));
  g_return_if_fail (anchor && anchor->type == CLAY_ANCHOR_ANCHOR);

  CLAY_STROKE_GET_CLASS (stroke)->anchor_delete (stroke, anchor);
}

static void
clay_stroke_real_anchor_delete (ClayStroke *stroke,
                                ClayAnchor *anchor)
{
  g_printerr ("clay_stroke_anchor_delete: default implementation\n");
}

ClayStroke *
clay_stroke_open (ClayStroke *stroke,
                  ClayAnchor *end_anchor)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (end_anchor &&
                        end_anchor->type == CLAY_ANCHOR_ANCHOR, NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->open (stroke, end_anchor);
}

static ClayStroke *
clay_stroke_real_open (ClayStroke *stroke,
                       ClayAnchor *end_anchor)
{
  g_printerr ("clay_stroke_open: default implementation\n");
  return NULL;
}

gboolean
clay_stroke_anchor_is_insertable (ClayStroke *stroke,
                                  ClayAnchor *predec,
                                  gdouble     position)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->anchor_is_insertable (stroke,
                                                               predec,
                                                               position);
}

static gboolean
clay_stroke_real_anchor_is_insertable (ClayStroke *stroke,
                                       ClayAnchor *predec,
                                       gdouble     position)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return FALSE;
}

ClayAnchor *
clay_stroke_anchor_insert (ClayStroke *stroke,
                           ClayAnchor *predec,
                           gdouble     position)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (predec->type == CLAY_ANCHOR_ANCHOR, NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->anchor_insert (stroke,
                                                        predec, position);
}

static ClayAnchor *
clay_stroke_real_anchor_insert (ClayStroke *stroke,
                                ClayAnchor *predec,
                                gdouble     position)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return NULL;
}


gboolean
clay_stroke_is_extendable (ClayStroke *stroke,
                           ClayAnchor *neighbor)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->is_extendable (stroke, neighbor);
}

static gboolean
clay_stroke_real_is_extendable (ClayStroke *stroke,
                                ClayAnchor *neighbor)
{
  return FALSE;
}


ClayAnchor *
clay_stroke_extend (ClayStroke           *stroke,
                    const ClayCoords     *coords,
                    ClayAnchor           *neighbor,
                    ClayStrokeExtendMode  extend_mode)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);
  g_return_val_if_fail (!stroke->closed, NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->extend (stroke, coords,
                                                 neighbor, extend_mode);
}

static ClayAnchor *
clay_stroke_real_extend (ClayStroke           *stroke,
                         const ClayCoords     *coords,
                         ClayAnchor           *neighbor,
                         ClayStrokeExtendMode  extend_mode)
{
  g_printerr ("clay_stroke_extend: default implementation\n");
  return NULL;
}

gboolean
clay_stroke_connect_stroke (ClayStroke *stroke,
                            ClayAnchor *anchor,
                            ClayStroke *extension,
                            ClayAnchor *neighbor)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (CLAY_IS_STROKE (extension), FALSE);
  g_return_val_if_fail (stroke->closed == FALSE &&
                        extension->closed == FALSE, FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->connect_stroke (stroke, anchor,
                                                         extension, neighbor);
}

gboolean
clay_stroke_real_connect_stroke (ClayStroke *stroke,
                                 ClayAnchor *anchor,
                                 ClayStroke *extension,
                                 ClayAnchor *neighbor)
{
  g_printerr ("clay_stroke_connect_stroke: default implementation\n");
  return FALSE;
}

gboolean
clay_stroke_is_empty (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->is_empty (stroke);
}

static gboolean
clay_stroke_real_is_empty (ClayStroke *stroke)
{
  return g_queue_is_empty (stroke->anchors);
}


gboolean
clay_stroke_reverse (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->reverse (stroke);
}


static gboolean
clay_stroke_real_reverse (ClayStroke *stroke)
{
  g_queue_reverse (stroke->anchors);

  /* keep the first node the same for closed strokes */
  if (stroke->closed && stroke->anchors->length > 0)
    g_queue_push_head_link (stroke->anchors,
                            g_queue_pop_tail_link (stroke->anchors));

  return TRUE;
}


gboolean
clay_stroke_shift_start (ClayStroke *stroke,
                         ClayAnchor *new_start)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);
  g_return_val_if_fail (new_start != NULL, FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->shift_start (stroke, new_start);
}

static gboolean
clay_stroke_real_shift_start (ClayStroke *stroke,
                              ClayAnchor *new_start)
{
  GList *link;

  link = g_queue_find (stroke->anchors, new_start);
  if (!link)
    return FALSE;

  if (link == stroke->anchors->head)
    return TRUE;

  stroke->anchors->tail->next = stroke->anchors->head;
  stroke->anchors->head->prev = stroke->anchors->tail;
  stroke->anchors->tail = link->prev;
  stroke->anchors->head = link;
  stroke->anchors->tail->next = NULL;
  stroke->anchors->head->prev = NULL;

  return TRUE;
}


gdouble
clay_stroke_get_length (ClayStroke *stroke,
                        gdouble     precision)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), 0.0);

  return CLAY_STROKE_GET_CLASS (stroke)->get_length (stroke, precision);
}

static gdouble
clay_stroke_real_get_length (ClayStroke *stroke,
                             gdouble     precision)
{
  GArray     *points;
  gint        i;
  gdouble     length;
  ClayCoords  difference;

  if (g_queue_is_empty (stroke->anchors))
    return -1;

  points = clay_stroke_interpolate (stroke, precision, NULL);
  if (points == NULL)
    return -1;

  length = 0;

  for (i = 0; i < points->len - 1; i++ )
    {
       clay_coords_difference (&(g_array_index (points, ClayCoords, i)),
                               &(g_array_index (points, ClayCoords, i+1)),
                               &difference);
       length += clay_coords_length (&difference);
    }

  g_array_free(points, TRUE);

  return length;
}


gdouble
clay_stroke_get_distance (ClayStroke       *stroke,
                          const ClayCoords *coord)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), 0.0);

  return CLAY_STROKE_GET_CLASS (stroke)->get_distance (stroke, coord);
}

static gdouble
clay_stroke_real_get_distance (ClayStroke       *stroke,
                               const ClayCoords *coord)
{
  g_printerr ("clay_stroke_get_distance: default implementation\n");

  return 0.0;
}


GArray *
clay_stroke_interpolate (ClayStroke *stroke,
                         gdouble     precision,
                         gboolean   *ret_closed)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->interpolate (stroke, precision,
                                                      ret_closed);
}

static GArray *
clay_stroke_real_interpolate (ClayStroke *stroke,
                              gdouble     precision,
                              gboolean   *ret_closed)
{
  g_printerr ("clay_stroke_interpolate: default implementation\n");

  return NULL;
}

ClayStroke *
clay_stroke_duplicate (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->duplicate (stroke);
}

static ClayStroke *
clay_stroke_real_duplicate (ClayStroke *stroke)
{
  ClayStroke *new_stroke;
  GList      *list;

  new_stroke = g_object_new (G_TYPE_FROM_INSTANCE (stroke),
                             "name", clay_object_get_name (stroke),
                             NULL);

  g_queue_free_full (new_stroke->anchors, (GDestroyNotify) clay_anchor_free);
  new_stroke->anchors = g_queue_copy (stroke->anchors);

  for (list = new_stroke->anchors->head; list; list = g_list_next (list))
    {
      list->data = clay_anchor_copy (CLAY_ANCHOR (list->data));
    }

  new_stroke->closed = stroke->closed;
  /* we do *not* copy the ID! */

  return new_stroke;
}


ClayBezierDesc *
clay_stroke_make_bezier (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->make_bezier (stroke);
}

static ClayBezierDesc *
clay_stroke_real_make_bezier (ClayStroke *stroke)
{
  g_printerr ("clay_stroke_make_bezier: default implementation\n");

  return NULL;
}


void
clay_stroke_translate (ClayStroke *stroke,
                       gdouble     offset_x,
                       gdouble     offset_y)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->translate (stroke, offset_x, offset_y);
}

static void
clay_stroke_real_translate (ClayStroke *stroke,
                            gdouble     offset_x,
                            gdouble     offset_y)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      ClayAnchor *anchor = list->data;

      anchor->position.x += offset_x;
      anchor->position.y += offset_y;
    }
}


void
clay_stroke_scale (ClayStroke *stroke,
                   gdouble     scale_x,
                   gdouble     scale_y)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->scale (stroke, scale_x, scale_y);
}

static void
clay_stroke_real_scale (ClayStroke *stroke,
                        gdouble     scale_x,
                        gdouble     scale_y)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      ClayAnchor *anchor = list->data;

      anchor->position.x *= scale_x;
      anchor->position.y *= scale_y;
    }
}

void
clay_stroke_rotate (ClayStroke *stroke,
                    gdouble     center_x,
                    gdouble     center_y,
                    gdouble     angle)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->rotate (stroke, center_x, center_y, angle);
}

static void
clay_stroke_real_rotate (ClayStroke *stroke,
                         gdouble     center_x,
                         gdouble     center_y,
                         gdouble     angle)
{
  ClayMatrix3  matrix;

  angle = angle / 180.0 * G_PI;
  clay_matrix3_identity (&matrix);
  clay_transform_matrix_rotate_center (&matrix, center_x, center_y, angle);

  clay_stroke_transform (stroke, &matrix, NULL);
}

void
clay_stroke_flip (ClayStroke          *stroke,
                  ClayOrientationType  flip_type,
                  gdouble              axis)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->flip (stroke, flip_type, axis);
}

static void
clay_stroke_real_flip (ClayStroke          *stroke,
                       ClayOrientationType  flip_type,
                       gdouble              axis)
{
  ClayMatrix3  matrix;

  clay_matrix3_identity (&matrix);
  clay_transform_matrix_flip (&matrix, flip_type, axis);
  clay_stroke_transform (stroke, &matrix, NULL);
}

void
clay_stroke_flip_free (ClayStroke *stroke,
                       gdouble     x1,
                       gdouble     y1,
                       gdouble     x2,
                       gdouble     y2)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->flip_free (stroke, x1, y1, x2, y2);
}

static void
clay_stroke_real_flip_free (ClayStroke *stroke,
                            gdouble     x1,
                            gdouble     y1,
                            gdouble     x2,
                            gdouble     y2)
{
  /* x, y, width and height parameter in clay_transform_matrix_flip_free are unused */
  ClayMatrix3  matrix;

  clay_matrix3_identity (&matrix);
  clay_transform_matrix_flip_free (&matrix, x1, y1, x2, y2);

  clay_stroke_transform (stroke, &matrix, NULL);
}

/* transforms 'stroke' by 'matrix'.  due to clipping, the transformation may
 * result in multiple strokes.
 *
 * if 'ret_strokes' is not NULL, the transformed strokes are appended to the
 * queue, and 'stroke' is left in an unspecified state.  one of the resulting
 * strokes may alias 'stroke'.
 *
 * if 'ret_strokes' is NULL, the transformation is performed in-place.  if the
 * transformation results in multiple strokes (which, atm, can only happen for
 * non-affine transformation), the result is undefined.
 */
void
clay_stroke_transform (ClayStroke        *stroke,
                       const ClayMatrix3 *matrix,
                       GQueue            *ret_strokes)
{
  g_return_if_fail (CLAY_IS_STROKE (stroke));

  CLAY_STROKE_GET_CLASS (stroke)->transform (stroke, matrix, ret_strokes);
}

static void
clay_stroke_real_transform (ClayStroke        *stroke,
                            const ClayMatrix3 *matrix,
                            GQueue            *ret_strokes)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      ClayAnchor *anchor = list->data;

      clay_matrix3_transform_point (matrix,
                                    anchor->position.x,
                                    anchor->position.y,
                                    &anchor->position.x,
                                    &anchor->position.y);
    }

  if (ret_strokes)
    {
      stroke->id = 0;

      g_queue_push_tail (ret_strokes, g_object_ref (stroke));
    }
}


GList *
clay_stroke_get_draw_anchors (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->get_draw_anchors (stroke);
}

static GList *
clay_stroke_real_get_draw_anchors (ClayStroke *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      if (CLAY_ANCHOR (list->data)->type == CLAY_ANCHOR_ANCHOR)
        ret_list = g_list_prepend (ret_list, list->data);
    }

  return g_list_reverse (ret_list);
}


GList *
clay_stroke_get_draw_controls (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->get_draw_controls (stroke);
}

static GList *
clay_stroke_real_get_draw_controls (ClayStroke *stroke)
{
  GList *list;
  GList *ret_list = NULL;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      ClayAnchor *anchor = list->data;

      if (anchor->type == CLAY_ANCHOR_CONTROL)
        {
          ClayAnchor *next = list->next ? list->next->data : NULL;
          ClayAnchor *prev = list->prev ? list->prev->data : NULL;

          if (next && next->type == CLAY_ANCHOR_ANCHOR && next->selected)
            {
              /* Ok, this is a hack.
               * The idea is to give control points at the end of a
               * stroke a higher priority for the interactive tool.
               */
              if (prev)
                ret_list = g_list_prepend (ret_list, anchor);
              else
                ret_list = g_list_append (ret_list, anchor);
            }
          else if (prev && prev->type == CLAY_ANCHOR_ANCHOR && prev->selected)
            {
              /* same here... */
              if (next)
                ret_list = g_list_prepend (ret_list, anchor);
              else
                ret_list = g_list_append (ret_list, anchor);
            }
        }
    }

  return g_list_reverse (ret_list);
}


GArray *
clay_stroke_get_draw_lines (ClayStroke *stroke)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->get_draw_lines (stroke);
}

static GArray *
clay_stroke_real_get_draw_lines (ClayStroke *stroke)
{
  GList  *list;
  GArray *ret_lines = NULL;
  gint    count = 0;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      ClayAnchor *anchor = list->data;

      if (anchor->type == CLAY_ANCHOR_ANCHOR && anchor->selected)
        {
          if (list->next)
            {
              ClayAnchor *next = list->next->data;

              if (count == 0)
                ret_lines = g_array_new (FALSE, FALSE, sizeof (ClayCoords));

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, next->position);
              count += 1;
            }

          if (list->prev)
            {
              ClayAnchor *prev = list->prev->data;

              if (count == 0)
                ret_lines = g_array_new (FALSE, FALSE, sizeof (ClayCoords));

              ret_lines = g_array_append_val (ret_lines, anchor->position);
              ret_lines = g_array_append_val (ret_lines, prev->position);
              count += 1;
            }
        }
    }

  return ret_lines;
}

GArray *
clay_stroke_control_points_get (ClayStroke *stroke,
                                gboolean   *ret_closed)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), NULL);

  return CLAY_STROKE_GET_CLASS (stroke)->control_points_get (stroke,
                                                             ret_closed);
}

static GArray *
clay_stroke_real_control_points_get (ClayStroke *stroke,
                                     gboolean   *ret_closed)
{
  guint   num_anchors;
  GArray *ret_array;
  GList  *list;

  num_anchors = g_queue_get_length (stroke->anchors);
  ret_array = g_array_sized_new (FALSE, FALSE,
                                 sizeof (ClayAnchor), num_anchors);

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      g_array_append_vals (ret_array, list->data, 1);
    }

  if (ret_closed)
    *ret_closed = stroke->closed;

  return ret_array;
}

gboolean
clay_stroke_get_point_at_dist (ClayStroke *stroke,
                               gdouble     dist,
                               gdouble     precision,
                               ClayCoords *position,
                               gdouble    *slope)
{
  g_return_val_if_fail (CLAY_IS_STROKE (stroke), FALSE);

  return CLAY_STROKE_GET_CLASS (stroke)->get_point_at_dist (stroke,
                                                            dist,
                                                            precision,
                                                            position,
                                                            slope);
}


static gboolean
clay_stroke_real_get_point_at_dist (ClayStroke *stroke,
                                    gdouble     dist,
                                    gdouble     precision,
                                    ClayCoords *position,
                                    gdouble    *slope)
{
  GArray     *points;
  gint        i;
  gdouble     length;
  gdouble     segment_length;
  gboolean    ret = FALSE;
  ClayCoords  difference;

  points = clay_stroke_interpolate (stroke, precision, NULL);
  if (points == NULL)
    return ret;

  length = 0;
  for (i=0; i < points->len - 1; i++)
    {
      clay_coords_difference (&(g_array_index (points, ClayCoords , i)),
                              &(g_array_index (points, ClayCoords , i+1)),
                              &difference);
      segment_length = clay_coords_length (&difference);

      if (segment_length == 0 || length + segment_length < dist )
        {
          length += segment_length;
        }
      else
        {
          /* x = x1 + (x2 - x1 ) u  */
          /* x   = x1 (1-u) + u x2  */

          gdouble u = (dist - length) / segment_length;

          clay_coords_mix (1 - u, &(g_array_index (points, ClayCoords , i)),
                               u, &(g_array_index (points, ClayCoords , i+1)),
                           position);

          if (difference.x == 0)
            *slope = G_MAXDOUBLE;
          else
            *slope = difference.y / difference.x;

          ret = TRUE;
          break;
        }
    }

  g_array_free (points, TRUE);

  return ret;
}
