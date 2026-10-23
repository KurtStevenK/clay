/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybezierstroke.h
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

#include "claystroke.h"


#define CLAY_TYPE_BEZIER_STROKE            (clay_bezier_stroke_get_type ())
#define CLAY_BEZIER_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BEZIER_STROKE, ClayBezierStroke))
#define CLAY_BEZIER_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BEZIER_STROKE, ClayBezierStrokeClass))
#define CLAY_IS_BEZIER_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BEZIER_STROKE))
#define CLAY_IS_BEZIER_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BEZIER_STROKE))
#define CLAY_BEZIER_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BEZIER_STROKE, ClayBezierStrokeClass))


typedef struct _ClayBezierStrokeClass ClayBezierStrokeClass;

struct _ClayBezierStroke
{
  ClayStroke  parent_instance;
};

struct _ClayBezierStrokeClass
{
  ClayStrokeClass  parent_class;
};


GType        clay_bezier_stroke_get_type        (void);

ClayStroke * clay_bezier_stroke_new             (void);
ClayStroke * clay_bezier_stroke_new_from_coords (const ClayCoords *coords,
                                                 gint              n_coords,
                                                 gboolean          closed);

ClayStroke * clay_bezier_stroke_new_moveto      (const ClayCoords *start);
void         clay_bezier_stroke_lineto          (ClayStroke       *bez_stroke,
                                                 const ClayCoords *end);
void         clay_bezier_stroke_conicto         (ClayStroke       *bez_stroke,
                                                 const ClayCoords *control,
                                                 const ClayCoords *end);
void         clay_bezier_stroke_cubicto         (ClayStroke       *bez_stroke,
                                                 const ClayCoords *control1,
                                                 const ClayCoords *control2,
                                                 const ClayCoords *end);
void         clay_bezier_stroke_arcto           (ClayStroke       *bez_stroke,
                                                 gdouble           radius_x,
                                                 gdouble           radius_y,
                                                 gdouble           angle_rad,
                                                 gboolean          large_arc,
                                                 gboolean          sweep,
                                                 const ClayCoords *end);
ClayStroke * clay_bezier_stroke_new_ellipse     (const ClayCoords *center,
                                                 gdouble           radius_x,
                                                 gdouble           radius_y,
                                                 gdouble           angle);


ClayAnchor * clay_bezier_stroke_extend      (ClayStroke           *stroke,
                                             const ClayCoords     *coords,
                                             ClayAnchor           *neighbor,
                                             ClayStrokeExtendMode  extend_mode);
