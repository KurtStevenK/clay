/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claystroke.h
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

#pragma once

#include "core/clayobject.h"


#define CLAY_TYPE_STROKE            (clay_stroke_get_type ())
#define CLAY_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_STROKE, ClayStroke))
#define CLAY_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_STROKE, ClayStrokeClass))
#define CLAY_IS_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_STROKE))
#define CLAY_IS_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_STROKE))
#define CLAY_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_STROKE, ClayStrokeClass))


typedef struct _ClayStrokeClass ClayStrokeClass;

struct _ClayStroke
{
  ClayObject  parent_instance;
  gint        id;

  GQueue     *anchors;

  gboolean    closed;
};

struct _ClayStrokeClass
{
  ClayObjectClass  parent_class;

  void          (* changed)              (ClayStroke            *stroke);
  void          (* removed)              (ClayStroke            *stroke);

  ClayAnchor  * (* anchor_get)           (ClayStroke            *stroke,
                                          const ClayCoords      *coord);
  gdouble       (* nearest_point_get)    (ClayStroke            *stroke,
                                          const ClayCoords      *coord,
                                          gdouble                precision,
                                          ClayCoords            *ret_point,
                                          ClayAnchor           **ret_segment_start,
                                          ClayAnchor           **ret_segment_end,
                                          gdouble               *ret_pos);
  gdouble       (* nearest_tangent_get)  (ClayStroke            *stroke,
                                          const ClayCoords      *coord1,
                                          const ClayCoords      *coord2,
                                          gdouble                precision,
                                          ClayCoords            *nearest,
                                          ClayAnchor           **ret_segment_start,
                                          ClayAnchor           **ret_segment_end,
                                          gdouble               *ret_pos);
  gdouble       (* nearest_intersection_get)
                                         (ClayStroke            *stroke,
                                          const ClayCoords      *coord1,
                                          const ClayCoords      *direction,
                                          gdouble                precision,
                                          ClayCoords            *nearest,
                                          ClayAnchor           **ret_segment_start,
                                          ClayAnchor           **ret_segment_end,
                                          gdouble               *ret_pos);
  ClayAnchor  * (* anchor_get_next)      (ClayStroke            *stroke,
                                          const ClayAnchor      *prev);
  void          (* anchor_select)        (ClayStroke            *stroke,
                                          ClayAnchor            *anchor,
                                          gboolean               selected,
                                          gboolean               exclusive);
  void          (* anchor_move_relative) (ClayStroke            *stroke,
                                          ClayAnchor            *anchor,
                                          const ClayCoords      *deltacoord,
                                          ClayAnchorFeatureType  feature);
  void          (* anchor_move_absolute) (ClayStroke            *stroke,
                                          ClayAnchor            *anchor,
                                          const ClayCoords      *coord,
                                          ClayAnchorFeatureType  feature);
  void          (* anchor_convert)       (ClayStroke            *stroke,
                                          ClayAnchor            *anchor,
                                          ClayAnchorFeatureType  feature);
  void          (* anchor_delete)        (ClayStroke            *stroke,
                                          ClayAnchor            *anchor);

  gboolean      (* point_is_movable)     (ClayStroke            *stroke,
                                          ClayAnchor            *predec,
                                          gdouble                position);
  void          (* point_move_relative)  (ClayStroke            *stroke,
                                          ClayAnchor            *predec,
                                          gdouble                position,
                                          const ClayCoords      *deltacoord,
                                          ClayAnchorFeatureType  feature);
  void          (* point_move_absolute)  (ClayStroke            *stroke,
                                          ClayAnchor            *predec,
                                          gdouble                position,
                                          const ClayCoords      *coord,
                                          ClayAnchorFeatureType  feature);

  void          (* close)                (ClayStroke            *stroke);
  ClayStroke  * (* open)                 (ClayStroke            *stroke,
                                          ClayAnchor            *end_anchor);
  gboolean      (* anchor_is_insertable) (ClayStroke            *stroke,
                                          ClayAnchor            *predec,
                                          gdouble                position);
  ClayAnchor  * (* anchor_insert)        (ClayStroke            *stroke,
                                          ClayAnchor            *predec,
                                          gdouble                position);
  gboolean      (* is_extendable)        (ClayStroke            *stroke,
                                          ClayAnchor            *neighbor);
  ClayAnchor  * (* extend)               (ClayStroke            *stroke,
                                          const ClayCoords      *coords,
                                          ClayAnchor            *neighbor,
                                          ClayStrokeExtendMode   extend_mode);
  gboolean      (* connect_stroke)       (ClayStroke            *stroke,
                                          ClayAnchor            *anchor,
                                          ClayStroke            *extension,
                                          ClayAnchor            *neighbor);

  gboolean      (* is_empty)             (ClayStroke            *stroke);
  gboolean      (* reverse)              (ClayStroke            *stroke);
  gboolean      (* shift_start)          (ClayStroke            *stroke,
                                          ClayAnchor            *new_start);

  gdouble       (* get_length)           (ClayStroke            *stroke,
                                          gdouble                precision);
  gdouble       (* get_distance)         (ClayStroke            *stroke,
                                          const ClayCoords      *coord);
  gboolean      (* get_point_at_dist)    (ClayStroke            *stroke,
                                          gdouble                dist,
                                          gdouble                precision,
                                          ClayCoords            *position,
                                          gdouble               *slope);

  GArray      * (* interpolate)          (ClayStroke            *stroke,
                                          gdouble                precision,
                                          gboolean              *ret_closed);

  ClayStroke  * (* duplicate)            (ClayStroke            *stroke);

  ClayBezierDesc * (* make_bezier)       (ClayStroke            *stroke);

  void          (* translate)            (ClayStroke            *stroke,
                                          gdouble                offset_x,
                                          gdouble                offset_y);
  void          (* scale)                (ClayStroke            *stroke,
                                          gdouble                scale_x,
                                          gdouble                scale_y);
  void          (* rotate)               (ClayStroke            *stroke,
                                          gdouble                center_x,
                                          gdouble                center_y,
                                          gdouble                angle);
  void          (* flip)                 (ClayStroke             *stroke,
                                          ClayOrientationType    flip_type,
                                          gdouble                axis);
  void          (* flip_free)            (ClayStroke            *stroke,
                                          gdouble                x1,
                                          gdouble                y1,
                                          gdouble                x2,
                                          gdouble                y2);
  void          (* transform)            (ClayStroke            *stroke,
                                          const ClayMatrix3     *matrix,
                                          GQueue                *ret_strokes);

  GList       * (* get_draw_anchors)     (ClayStroke            *stroke);
  GList       * (* get_draw_controls)    (ClayStroke            *stroke);
  GArray      * (* get_draw_lines)       (ClayStroke            *stroke);
  GArray      * (* control_points_get)   (ClayStroke            *stroke,
                                          gboolean              *ret_closed);
};


GType        clay_stroke_get_type             (void);

void         clay_stroke_set_id               (ClayStroke            *stroke,
                                               gint                   id);
gint         clay_stroke_get_id               (ClayStroke            *stroke);


/* accessing / modifying the anchors */

GArray     * clay_stroke_control_points_get   (ClayStroke            *stroke,
                                               gboolean              *closed);

ClayAnchor * clay_stroke_anchor_get           (ClayStroke            *stroke,
                                               const ClayCoords      *coord);

gdouble      clay_stroke_nearest_point_get    (ClayStroke            *stroke,
                                               const ClayCoords      *coord,
                                               gdouble                precision,
                                               ClayCoords            *ret_point,
                                               ClayAnchor           **ret_segment_start,
                                               ClayAnchor           **ret_segment_end,
                                               gdouble               *ret_pos);
gdouble     clay_stroke_nearest_tangent_get   (ClayStroke            *stroke,
                                               const ClayCoords      *coords1,
                                               const ClayCoords      *coords2,
                                               gdouble                precision,
                                               ClayCoords            *nearest,
                                               ClayAnchor           **ret_segment_start,
                                               ClayAnchor           **ret_segment_end,
                                               gdouble               *ret_pos);
gdouble  clay_stroke_nearest_intersection_get (ClayStroke            *stroke,
                                               const ClayCoords      *coords1,
                                               const ClayCoords      *direction,
                                               gdouble                precision,
                                               ClayCoords            *nearest,
                                               ClayAnchor           **ret_segment_start,
                                               ClayAnchor           **ret_segment_end,
                                               gdouble               *ret_pos);


/* prev == NULL: "first" anchor */
ClayAnchor * clay_stroke_anchor_get_next      (ClayStroke            *stroke,
                                               const ClayAnchor      *prev);

void         clay_stroke_anchor_select        (ClayStroke            *stroke,
                                               ClayAnchor            *anchor,
                                               gboolean               selected,
                                               gboolean               exclusive);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void         clay_stroke_anchor_move_relative (ClayStroke            *stroke,
                                               ClayAnchor            *anchor,
                                               const ClayCoords      *delta,
                                               ClayAnchorFeatureType  feature);
void         clay_stroke_anchor_move_absolute (ClayStroke            *stroke,
                                               ClayAnchor            *anchor,
                                               const ClayCoords      *coord,
                                               ClayAnchorFeatureType  feature);

gboolean     clay_stroke_point_is_movable     (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position);
void         clay_stroke_point_move_relative  (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position,
                                               const ClayCoords      *deltacoord,
                                               ClayAnchorFeatureType  feature);
void         clay_stroke_point_move_absolute  (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position,
                                               const ClayCoords      *coord,
                                               ClayAnchorFeatureType  feature);

void         clay_stroke_close                (ClayStroke            *stroke);

void         clay_stroke_anchor_convert       (ClayStroke            *stroke,
                                               ClayAnchor            *anchor,
                                               ClayAnchorFeatureType  feature);

void         clay_stroke_anchor_delete        (ClayStroke            *stroke,
                                               ClayAnchor            *anchor);

ClayStroke * clay_stroke_open                 (ClayStroke            *stroke,
                                               ClayAnchor            *end_anchor);
gboolean     clay_stroke_anchor_is_insertable (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position);
ClayAnchor * clay_stroke_anchor_insert        (ClayStroke            *stroke,
                                               ClayAnchor            *predec,
                                               gdouble                position);

gboolean     clay_stroke_is_extendable        (ClayStroke            *stroke,
                                               ClayAnchor            *neighbor);

ClayAnchor * clay_stroke_extend               (ClayStroke            *stroke,
                                               const ClayCoords      *coords,
                                               ClayAnchor            *neighbor,
                                               ClayStrokeExtendMode   extend_mode);

gboolean     clay_stroke_connect_stroke       (ClayStroke            *stroke,
                                               ClayAnchor            *anchor,
                                               ClayStroke            *extension,
                                               ClayAnchor            *neighbor);

gboolean     clay_stroke_is_empty             (ClayStroke            *stroke);

gboolean     clay_stroke_reverse              (ClayStroke            *stroke);
gboolean     clay_stroke_shift_start          (ClayStroke            *stroke,
                                               ClayAnchor            *new_start);

/* accessing the shape of the curve */

gdouble      clay_stroke_get_length           (ClayStroke            *stroke,
                                               gdouble                precision);
gdouble      clay_stroke_get_distance         (ClayStroke            *stroke,
                                               const ClayCoords      *coord);

gboolean     clay_stroke_get_point_at_dist    (ClayStroke            *stroke,
                                               gdouble                dist,
                                               gdouble                precision,
                                               ClayCoords            *position,
                                               gdouble               *slope);

/* returns an array of valid coordinates */
GArray     * clay_stroke_interpolate          (ClayStroke            *stroke,
                                               const gdouble          precision,
                                               gboolean              *closed);

ClayStroke * clay_stroke_duplicate            (ClayStroke            *stroke);

/* creates a bezier approximation. */
ClayBezierDesc * clay_stroke_make_bezier      (ClayStroke            *stroke);

void         clay_stroke_translate            (ClayStroke            *stroke,
                                               gdouble                offset_x,
                                               gdouble                offset_y);
void         clay_stroke_scale                (ClayStroke            *stroke,
                                               gdouble                scale_x,
                                               gdouble                scale_y);
void         clay_stroke_rotate               (ClayStroke            *stroke,
                                               gdouble                center_x,
                                               gdouble                center_y,
                                               gdouble                angle);
void         clay_stroke_flip                 (ClayStroke             *stroke,
                                               ClayOrientationType    flip_type,
                                               gdouble                axis);
void         clay_stroke_flip_free            (ClayStroke            *stroke,
                                               gdouble                x1,
                                               gdouble                y1,
                                               gdouble                x2,
                                               gdouble                y2);
void         clay_stroke_transform            (ClayStroke            *stroke,
                                               const ClayMatrix3     *matrix,
                                               GQueue                *ret_strokes);

GList      * clay_stroke_get_draw_anchors     (ClayStroke            *stroke);
GList      * clay_stroke_get_draw_controls    (ClayStroke            *stroke);
GArray     * clay_stroke_get_draw_lines       (ClayStroke            *stroke);
