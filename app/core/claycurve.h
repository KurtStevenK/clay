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

#pragma once

#include "claydata.h"


#define CLAY_TYPE_CURVE            (clay_curve_get_type ())
#define CLAY_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CURVE, ClayCurve))
#define CLAY_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CURVE, ClayCurveClass))
#define CLAY_IS_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CURVE))
#define CLAY_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CURVE))
#define CLAY_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CURVE, ClayCurveClass))


typedef struct _ClayCurvePoint ClayCurvePoint;
typedef struct _ClayCurveClass ClayCurveClass;

struct _ClayCurvePoint
{
  gdouble            x;
  gdouble            y;

  ClayCurvePointType type;
};

struct _ClayCurve
{
  ClayData        parent_instance;

  ClayCurveType   curve_type;

  gint            n_points;
  ClayCurvePoint *points;

  gint            n_samples;
  gdouble        *samples;

  gboolean        identity;  /* whether the curve is an identity mapping */
};

struct _ClayCurveClass
{
  ClayDataClass  parent_class;
};


GType                clay_curve_get_type          (void);

ClayData           * clay_curve_new               (const gchar        *name);
ClayData           * clay_curve_get_standard      (void);

void                 clay_curve_reset             (ClayCurve          *curve,
                                                   gboolean            reset_type);

void                 clay_curve_set_curve_type    (ClayCurve          *curve,
                                                   ClayCurveType       curve_type);
ClayCurveType        clay_curve_get_curve_type    (ClayCurve          *curve);

gint                 clay_curve_get_n_points      (ClayCurve          *curve);

void                 clay_curve_set_n_samples     (ClayCurve          *curve,
                                                   gint                n_samples);
gint                 clay_curve_get_n_samples     (ClayCurve          *curve);

gint                 clay_curve_get_point_at      (ClayCurve          *curve,
                                                   gdouble             x);
gint                 clay_curve_get_closest_point (ClayCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y,
                                                   gdouble             max_distance);

gint                 clay_curve_add_point         (ClayCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);
void                 clay_curve_delete_point      (ClayCurve          *curve,
                                                   gint                point);
void                 clay_curve_set_point         (ClayCurve          *curve,
                                                   gint                point,
                                                   gdouble             x,
                                                   gdouble             y);
void                 clay_curve_move_point        (ClayCurve          *curve,
                                                   gint                point,
                                                   gdouble             y);
void                 clay_curve_get_point         (ClayCurve          *curve,
                                                   gint                point,
                                                   gdouble            *x,
                                                   gdouble            *y);
void                 clay_curve_set_point_type    (ClayCurve          *curve,
                                                   gint                point,
                                                   ClayCurvePointType  type);
ClayCurvePointType   clay_curve_get_point_type    (ClayCurve          *curve,
                                                   gint                point);
void                 clay_curve_clear_points      (ClayCurve          *curve);

gdouble              clay_curve_get_sample        (ClayCurve          *curve,
                                                   gdouble             x);
void                 clay_curve_set_sample        (ClayCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);

gboolean             clay_curve_is_identity       (ClayCurve          *curve);

void                 clay_curve_get_uchar         (ClayCurve          *curve,
                                                   gint                n_samples,
                                                   guchar             *samples);
