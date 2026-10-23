/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayvector.h
 *
 * The clay_vector* functions were taken from:
 * GCK - The General Convenience Kit
 * Copyright (C) 1996 Tom Bech
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

#ifndef __CLAY_VECTOR_H__
#define __CLAY_VECTOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* Two dimensional vector functions */
/* ================================ */

ClayVector2 clay_vector2_new               (gdouble            x,
                                            gdouble            y);
void        clay_vector2_set               (ClayVector2       *vector,
                                            gdouble            x,
                                            gdouble            y);
gdouble     clay_vector2_length            (const ClayVector2 *vector);
gdouble     clay_vector2_length_val        (ClayVector2        vector);
void        clay_vector2_mul               (ClayVector2       *vector,
                                            gdouble            factor);
ClayVector2 clay_vector2_mul_val           (ClayVector2        vector,
                                            gdouble            factor);
void        clay_vector2_normalize         (ClayVector2       *vector);
ClayVector2 clay_vector2_normalize_val     (ClayVector2        vector);
void        clay_vector2_neg               (ClayVector2       *vector);
ClayVector2 clay_vector2_neg_val           (ClayVector2        vector);
void        clay_vector2_add               (ClayVector2       *result,
                                            const ClayVector2 *vector1,
                                            const ClayVector2 *vector2);
ClayVector2 clay_vector2_add_val           (ClayVector2        vector1,
                                            ClayVector2        vector2);
void        clay_vector2_sub               (ClayVector2       *result,
                                            const ClayVector2 *vector1,
                                            const ClayVector2 *vector2);
ClayVector2 clay_vector2_sub_val           (ClayVector2        vector1,
                                            ClayVector2        vector2);
gdouble     clay_vector2_inner_product     (const ClayVector2 *vector1,
                                            const ClayVector2 *vector2);
gdouble     clay_vector2_inner_product_val (ClayVector2        vector1,
                                            ClayVector2        vector2);
ClayVector2 clay_vector2_cross_product     (const ClayVector2 *vector1,
                                            const ClayVector2 *vector2);
ClayVector2 clay_vector2_cross_product_val (ClayVector2        vector1,
                                            ClayVector2        vector2);
void        clay_vector2_rotate            (ClayVector2       *vector,
                                            gdouble            alpha);
ClayVector2 clay_vector2_rotate_val        (ClayVector2        vector,
                                            gdouble            alpha);
ClayVector2 clay_vector2_normal            (ClayVector2       *vector);
ClayVector2 clay_vector2_normal_val        (ClayVector2        vector);

/* Three dimensional vector functions */
/* ================================== */

ClayVector3 clay_vector3_new               (gdouble            x,
                                            gdouble            y,
                                            gdouble            z);
void        clay_vector3_set               (ClayVector3       *vector,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble            z);
gdouble     clay_vector3_length            (const ClayVector3 *vector);
gdouble     clay_vector3_length_val        (ClayVector3        vector);
void        clay_vector3_mul               (ClayVector3       *vector,
                                            gdouble            factor);
ClayVector3 clay_vector3_mul_val           (ClayVector3        vector,
                                            gdouble            factor);
void        clay_vector3_normalize         (ClayVector3       *vector);
ClayVector3 clay_vector3_normalize_val     (ClayVector3        vector);
void        clay_vector3_neg               (ClayVector3       *vector);
ClayVector3 clay_vector3_neg_val           (ClayVector3        vector);
void        clay_vector3_add               (ClayVector3       *result,
                                            const ClayVector3 *vector1,
                                            const ClayVector3 *vector2);
ClayVector3 clay_vector3_add_val           (ClayVector3        vector1,
                                            ClayVector3        vector2);
void        clay_vector3_sub               (ClayVector3       *result,
                                            const ClayVector3 *vector1,
                                            const ClayVector3 *vector2);
ClayVector3 clay_vector3_sub_val           (ClayVector3        vector1,
                                            ClayVector3        vector2);
gdouble     clay_vector3_inner_product     (const ClayVector3 *vector1,
                                            const ClayVector3 *vector2);
gdouble     clay_vector3_inner_product_val (ClayVector3        vector1,
                                            ClayVector3        vector2);
ClayVector3 clay_vector3_cross_product     (const ClayVector3 *vector1,
                                            const ClayVector3 *vector2);
ClayVector3 clay_vector3_cross_product_val (ClayVector3        vector1,
                                            ClayVector3        vector2);
void        clay_vector3_rotate            (ClayVector3       *vector,
                                            gdouble            alpha,
                                            gdouble            beta,
                                            gdouble            gamma);
ClayVector3 clay_vector3_rotate_val        (ClayVector3        vector,
                                            gdouble            alpha,
                                            gdouble            beta,
                                            gdouble            gamma);

/* 2d <-> 3d Vector projection functions */
/* ===================================== */

void        clay_vector_2d_to_3d           (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gint               x,
                                            gint               y,
                                            const ClayVector3 *vp,
                                            ClayVector3       *p);

ClayVector3 clay_vector_2d_to_3d_val       (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gint               x,
                                            gint               y,
                                            ClayVector3        vp,
                                            ClayVector3        p);

void        clay_vector_3d_to_2d           (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gdouble           *x,
                                            gdouble           *y,
                                            const ClayVector3 *vp,
                                            const ClayVector3 *p);


G_END_DECLS

#endif  /* __CLAY_VECTOR_H__ */
