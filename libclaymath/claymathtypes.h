/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claymathtypes.h
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

#ifndef __CLAY_MATH_TYPES_H__
#define __CLAY_MATH_TYPES_H__


#include <libclaybase/claybasetypes.h>


G_BEGIN_DECLS

typedef struct _ClayMatrix2 ClayMatrix2;
typedef struct _ClayMatrix3 ClayMatrix3;
typedef struct _ClayMatrix4 ClayMatrix4;

/**
 * ClayMatrix2
 * @coeff: the coefficients
 *
 * A two by two matrix.
 **/
struct _ClayMatrix2
{
  gdouble coeff[2][2];
};

/**
 * ClayMatrix3
 * @coeff: the coefficients
 *
 * A three by three matrix.
 **/
struct _ClayMatrix3
{
  gdouble coeff[3][3];
};

/**
 * ClayMatrix4
 * @coeff: the coefficients
 *
 * A four by four matrix.
 **/
struct _ClayMatrix4
{
  gdouble coeff[4][4];
};


typedef struct _ClayVector2 ClayVector2;
typedef struct _ClayVector3 ClayVector3;
typedef struct _ClayVector4 ClayVector4;

/**
 * ClayVector2:
 * @x: the x axis
 * @y: the y axis
 *
 * A two dimensional vector.
 **/
struct _ClayVector2
{
  gdouble x, y;
};

/**
 * CLAY_TYPE_VECTOR2:
 *
 * Boxed type representing a two-dimensional vector.
 */
#define CLAY_TYPE_VECTOR2 (clay_vector2_get_type ())

GType clay_vector2_get_type (void);


/**
 * ClayVector3:
 * @x: the x axis
 * @y: the y axis
 * @z: the z axis
 *
 * A three dimensional vector.
 **/
struct _ClayVector3
{
  gdouble x, y, z;
};

/**
 * CLAY_TYPE_VECTOR3:
 *
 * Boxed type representing a three-dimensional vector.
 */
#define CLAY_TYPE_VECTOR3 (clay_vector3_get_type ())

GType clay_vector3_get_type (void);


/**
 * ClayVector4:
 * @x: the x axis
 * @y: the y axis
 * @z: the z axis
 * @w: the w axis
 *
 * A four dimensional vector.
 **/
struct _ClayVector4
{
  gdouble x, y, z, w;
};


G_END_DECLS

#endif /* __CLAY_MATH_TYPES_H__ */
