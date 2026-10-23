/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * claylimits.h
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_LIMITS_H__
#define __CLAY_LIMITS_H__

G_BEGIN_DECLS


/**
 * SECTION: claylimits
 * @title: claylimits
 * @short_description: Boundaries of some Clay data types and some
 *                     global constants.
 *
 * Boundaries of some Clay data types and some global constants.
 **/


/**
 * CLAY_MIN_IMAGE_SIZE:
 *
 * The minimum width and height of a Clay image in pixels.
 **/
#define CLAY_MIN_IMAGE_SIZE  1

/**
 * CLAY_MAX_IMAGE_SIZE:
 *
 * The maximum width and height of a Clay image in pixels. This is a
 * somewhat arbitrary value that can be used when an upper value for
 * pixel sizes is needed; for example to give a spin button an upper
 * limit.
 **/
#define CLAY_MAX_IMAGE_SIZE  524288    /*  2^19  */


/**
 * CLAY_MIN_RESOLUTION:
 *
 * The minimum resolution of a Clay image in pixels per inch. This is
 * a somewhat arbitrary value that can be used when a lower value for a
 * resolution is needed. Clay will not accept resolutions smaller than
 * this value.
 **/
#define CLAY_MIN_RESOLUTION  5e-3      /*  shouldn't display as 0.000  */

/**
 * CLAY_MAX_RESOLUTION:
 *
 * The maximum resolution of a Clay image in pixels per inch. This is
 * a somewhat arbitrary value that can be used to when an upper value
 * for a resolution is needed. Clay will not accept resolutions larger
 * than this value.
 **/
#define CLAY_MAX_RESOLUTION  1048576.0


/**
 * CLAY_MAX_MEMSIZE:
 *
 * A large but arbitrary value that can be used when an upper limit
 * for a memory size (in bytes) is needed. It is smaller than
 * %G_MAXDOUBLE since the #ClayMemsizeEntry doesn't handle larger
 * values.
 **/
#define CLAY_MAX_MEMSIZE     ((guint64) 1 << 42) /*  4 terabyte;
                                                  *  needs a 64bit variable
                                                  *  and must be < G_MAXDOUBLE
                                                  */


G_END_DECLS

#endif /* __CLAY_LIMITS_H__ */
