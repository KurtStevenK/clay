/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycurve.h
 * Copyright (C) 2026 Alx Sa
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_CURVE_H__
#define __CLAY_CURVE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_CURVE (clay_curve_get_type ())
G_DECLARE_FINAL_TYPE (ClayCurve, clay_curve, CLAY, CURVE, GObject)


ClayCurve          * clay_curve_new               (void);

ClayCurveType        clay_curve_get_curve_type    (ClayCurve          *curve);
void                 clay_curve_set_curve_type    (ClayCurve          *curve,
                                                   ClayCurveType       curve_type);

gint                 clay_curve_get_n_points      (ClayCurve          *curve);
gint                 clay_curve_add_point         (ClayCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);
void                 clay_curve_get_point         (ClayCurve          *curve,
                                                   gint                point,
                                                   gdouble            *x,
                                                   gdouble            *y);
ClayCurvePointType   clay_curve_get_point_type    (ClayCurve          *curve,
                                                   gint                point);
void                 clay_curve_set_point_type    (ClayCurve          *curve,
                                                   gint                point,
                                                   ClayCurvePointType  type);
void                 clay_curve_delete_point      (ClayCurve          *curve,
                                                   gint                point);
void                 clay_curve_set_point         (ClayCurve          *curve,
                                                   gint                point,
                                                   gdouble             x,
                                                   gdouble             y);
void                 clay_curve_clear_points      (ClayCurve          *curve);

gint                 clay_curve_get_n_samples     (ClayCurve          *curve);
void                 clay_curve_set_n_samples     (ClayCurve          *curve,
                                                   gint                n_samples);
gdouble              clay_curve_get_sample        (ClayCurve          *curve,
                                                   gdouble             x);
void                 clay_curve_set_sample        (ClayCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);

gboolean             clay_curve_is_identity       (ClayCurve          *curve);


G_END_DECLS

#endif /* __CLAY_CURVE_H__ */
