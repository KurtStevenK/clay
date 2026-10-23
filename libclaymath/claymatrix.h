/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claymatrix.h
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
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

#if !defined (__CLAY_MATH_H_INSIDE__) && !defined (CLAY_MATH_COMPILATION)
#error "Only <libclaymath/claymath.h> can be included directly."
#endif

#ifndef __CLAY_MATRIX_H__
#define __CLAY_MATRIX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*****************/
/*  ClayMatrix2  */
/*****************/

#define CLAY_TYPE_MATRIX2               (clay_matrix2_get_type ())
#define CLAY_VALUE_HOLDS_MATRIX2(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_MATRIX2))

GType         clay_matrix2_get_type        (void);


#define CLAY_TYPE_PARAM_MATRIX2            (clay_param_matrix2_get_type ())
#define CLAY_IS_PARAM_SPEC_MATRIX2(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_MATRIX2))

GType         clay_param_matrix2_get_type  (void);

GParamSpec *  clay_param_spec_matrix2      (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const ClayMatrix2  *default_value,
                                            GParamFlags         flags);


void          clay_matrix2_identity        (ClayMatrix2       *matrix);
void          clay_matrix2_mult            (const ClayMatrix2 *left,
                                            ClayMatrix2       *right);

gdouble       clay_matrix2_determinant     (const ClayMatrix2 *matrix);
void          clay_matrix2_invert          (ClayMatrix2       *matrix);

void          clay_matrix2_transform_point (const ClayMatrix2 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble           *newx,
                                            gdouble           *newy);


/*****************/
/*  ClayMatrix3  */
/*****************/

#define CLAY_TYPE_MATRIX3               (clay_matrix3_get_type ())
#define CLAY_VALUE_HOLDS_MATRIX3(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_MATRIX3))

GType         clay_matrix3_get_type        (void);


#define CLAY_TYPE_PARAM_MATRIX3            (clay_param_matrix3_get_type ())
#define CLAY_IS_PARAM_SPEC_MATRIX3(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_MATRIX3))

GType         clay_param_matrix3_get_type  (void);

GParamSpec *  clay_param_spec_matrix3      (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const ClayMatrix3  *default_value,
                                            GParamFlags         flags);


void          clay_matrix3_identity        (ClayMatrix3       *matrix);
void          clay_matrix3_mult            (const ClayMatrix3 *left,
                                            ClayMatrix3       *right);
void          clay_matrix3_translate       (ClayMatrix3       *matrix,
                                            gdouble            x,
                                            gdouble            y);
void          clay_matrix3_scale           (ClayMatrix3       *matrix,
                                            gdouble            x,
                                            gdouble            y);
void          clay_matrix3_rotate          (ClayMatrix3       *matrix,
                                            gdouble            theta);
void          clay_matrix3_xshear          (ClayMatrix3       *matrix,
                                            gdouble            amount);
void          clay_matrix3_yshear          (ClayMatrix3       *matrix,
                                            gdouble            amount);
void          clay_matrix3_affine          (ClayMatrix3       *matrix,
                                            gdouble            a,
                                            gdouble            b,
                                            gdouble            c,
                                            gdouble            d,
                                            gdouble            e,
                                            gdouble            f);

gdouble       clay_matrix3_determinant     (const ClayMatrix3 *matrix);
void          clay_matrix3_invert          (ClayMatrix3       *matrix);

gboolean      clay_matrix3_is_identity     (const ClayMatrix3 *matrix);
gboolean      clay_matrix3_is_diagonal     (const ClayMatrix3 *matrix);
gboolean      clay_matrix3_is_affine       (const ClayMatrix3 *matrix);
gboolean      clay_matrix3_is_simple       (const ClayMatrix3 *matrix);

gboolean      clay_matrix3_equal           (const ClayMatrix3 *matrix1,
                                            const ClayMatrix3 *matrix2);

void          clay_matrix3_transform_point (const ClayMatrix3 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble           *newx,
                                            gdouble           *newy);


/*****************/
/*  ClayMatrix4  */
/*****************/

void          clay_matrix4_identity        (ClayMatrix4       *matrix);
void          clay_matrix4_mult            (const ClayMatrix4 *left,
                                            ClayMatrix4       *right);

void          clay_matrix4_to_deg          (const ClayMatrix4 *matrix,
                                            gdouble           *a,
                                            gdouble           *b,
                                            gdouble           *c);

gdouble       clay_matrix4_transform_point (const ClayMatrix4 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble            z,
                                            gdouble           *newx,
                                            gdouble           *newy,
                                            gdouble           *newz);


G_END_DECLS

#endif /* __CLAY_MATRIX_H__ */
