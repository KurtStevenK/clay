/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypath.h
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

#include "core/clayitem.h"


#define CLAY_TYPE_PATH            (clay_path_get_type ())
#define CLAY_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATH, ClayPath))
#define CLAY_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PATH, ClayPathClass))
#define CLAY_IS_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATH))
#define CLAY_IS_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PATH))
#define CLAY_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PATH, ClayPathClass))


typedef struct _ClayPathClass  ClayPathClass;

struct _ClayPath
{
  ClayItem        parent_instance;

  GQueue         *strokes;        /* Queue of ClayStrokes         */
  GHashTable     *stroke_to_list; /* Map from ClayStroke to strokes listnode */
  gint            last_stroke_id;

  gint            freeze_count;
  gdouble         precision;

  ClayBezierDesc *bezier_desc;    /* Cached bezier representation */

  gboolean        bounds_valid;   /* Cached bounding box          */
  gboolean        bounds_empty;
  gdouble         bounds_x1;
  gdouble         bounds_y1;
  gdouble         bounds_x2;
  gdouble         bounds_y2;
};

struct _ClayPathClass
{
  ClayItemClass  parent_class;

  /*  signals  */
  void          (* freeze)            (ClayPath       *path);
  void          (* thaw)              (ClayPath       *path);

  /*  virtual functions  */
  void          (* stroke_add)        (ClayPath          *path,
                                       ClayStroke        *stroke);
  void          (* stroke_remove)     (ClayPath          *path,
                                       ClayStroke        *stroke);
  ClayStroke  * (* stroke_get)        (ClayPath          *path,
                                       const ClayCoords  *coord);
  ClayStroke  * (* stroke_get_next)   (ClayPath          *path,
                                       ClayStroke        *prev);
  gdouble       (* stroke_get_length) (ClayPath          *path,
                                       ClayStroke        *stroke);
  ClayAnchor  * (* anchor_get)        (ClayPath          *path,
                                       const ClayCoords  *coord,
                                       ClayStroke       **ret_stroke);
  void          (* anchor_delete)     (ClayPath          *path,
                                       ClayAnchor        *anchor);
  gdouble       (* get_length)        (ClayPath          *path,
                                       const ClayAnchor  *start);
  gdouble       (* get_distance)      (ClayPath          *path,
                                       const ClayCoords  *coord);
  gint          (* interpolate)       (ClayPath          *path,
                                       ClayStroke        *stroke,
                                       gdouble            precision,
                                       gint               max_points,
                                       ClayCoords        *ret_coords);
  ClayBezierDesc * (* make_bezier)    (ClayPath          *path);
};


/*  path utility functions  */

GType           clay_path_get_type              (void);

ClayPath      * clay_path_new                   (ClayImage   *image,
                                                 const gchar *name);

ClayPath      * clay_path_get_parent            (ClayPath    *path);

void            clay_path_freeze                (ClayPath    *path);
void            clay_path_thaw                  (ClayPath    *path);

void            clay_path_copy_strokes          (ClayPath    *src_path,
                                                 ClayPath    *dest_path);
void            clay_path_add_strokes           (ClayPath    *src_path,
                                                 ClayPath    *dest_path);


/* accessing / modifying the anchors */

ClayAnchor    * clay_path_anchor_get            (ClayPath          *path,
                                                 const ClayCoords  *coord,
                                                 ClayStroke       **ret_stroke);

/* prev == NULL: "first" anchor */
ClayAnchor    * clay_path_anchor_get_next       (ClayPath           *path,
                                                 const ClayAnchor   *prev);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void          clay_path_anchor_move_relative    (ClayPath           *path,
                                                 ClayAnchor         *anchor,
                                                 const ClayCoords   *deltacoord,
                                                 gint                type);
void          clay_path_anchor_move_absolute    (ClayPath           *path,
                                                 ClayAnchor         *anchor,
                                                 const ClayCoords   *coord,
                                                 gint                type);

void          clay_path_anchor_delete           (ClayPath           *path,
                                                 ClayAnchor         *anchor);

void          clay_path_anchor_select           (ClayPath           *path,
                                                 ClayStroke         *target_stroke,
                                                 ClayAnchor         *anchor,
                                                 gboolean            selected,
                                                 gboolean            exclusive);


/* ClayStroke is a connected component of a ClayPath object */

void            clay_path_stroke_add            (ClayPath           *path,
                                                 ClayStroke         *stroke);
void            clay_path_stroke_remove         (ClayPath           *path,
                                                 ClayStroke         *stroke);
gint            clay_path_get_n_strokes         (ClayPath           *path);
ClayStroke    * clay_path_stroke_get            (ClayPath           *path,
                                                 const ClayCoords   *coord);
ClayStroke    * clay_path_stroke_get_by_id      (ClayPath           *path,
                                                 gint                id);

/* prev == NULL: "first" stroke */
ClayStroke    * clay_path_stroke_get_next       (ClayPath           *path,
                                                 ClayStroke         *prev);
gdouble         clay_path_stroke_get_length     (ClayPath           *path,
                                                 ClayStroke         *stroke);

/* accessing the shape of the curve */

gdouble         clay_path_get_length            (ClayPath           *path,
                                                 const ClayAnchor   *start);
gdouble         clay_path_get_distance          (ClayPath           *path,
                                                 const ClayCoords   *coord);

/* returns the number of valid coordinates */

gint            clay_path_interpolate           (ClayPath           *path,
                                                 ClayStroke         *stroke,
                                                 gdouble             precision,
                                                 gint                max_points,
                                                 ClayCoords         *ret_coords);

gboolean        clay_path_attached_to_vector_layer
                                                (ClayPath           *path,
                                                 ClayImage          *image);
/* usually overloaded */

/* returns a bezier representation */
const ClayBezierDesc * clay_path_get_bezier     (ClayPath           *path);
