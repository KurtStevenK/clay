/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_CHECKS_H__
#define __CLAY_CHECKS_H__

G_BEGIN_DECLS


/**
 * CLAY_CHECK_SIZE:
 *
 * The default checkerboard size in pixels. This is configurable in
 * the core but Clay plug-ins can't access the user preference and
 * should use this constant instead.
 **/
#define CLAY_CHECK_SIZE     8

/**
 * CLAY_CHECK_SIZE_SM:
 *
 * The default small checkerboard size in pixels.
 **/
#define CLAY_CHECK_SIZE_SM  4


/**
 * CLAY_CHECK_DARK:
 *
 * The dark gray value for the default checkerboard pattern.
 **/
#define CLAY_CHECK_DARK   0.4

/**
 * CLAY_CHECK_LIGHT:
 *
 * The dark light value for the default checkerboard pattern.
 **/
#define CLAY_CHECK_LIGHT  0.6

/**
 * CLAY_CHECKS_LIGHT_COLOR_DARK:
 *
 * The dark color for the light checkerboard type.
 **/
#define CLAY_CHECKS_LIGHT_COLOR_DARK ((gdouble[]) { 0.8, 0.8, 0.8, 1.0 })

/**
 * CLAY_CHECKS_LIGHT_COLOR_LIGHT:
 *
 * The light color for the light checkerboard type.
 **/
#define CLAY_CHECKS_LIGHT_COLOR_LIGHT ((gdouble[]) { 1.0, 1.0, 1.0, 1.0 })

/**
 * CLAY_CHECKS_GRAY_COLOR_DARK:
 *
 * The dark color for the gray checkerboard type.
 **/
#define CLAY_CHECKS_GRAY_COLOR_DARK ((gdouble[]) { 0.4, 0.4, 0.4, 1.0 })

/**
 * CLAY_CHECKS_GRAY_COLOR_LIGHT:
 *
 * The light color for the gray checkerboard type.
 **/
#define CLAY_CHECKS_GRAY_COLOR_LIGHT ((gdouble[]) { 0.6, 0.6, 0.6, 1.0 })

/**
 * CLAY_CHECKS_DARK_COLOR_DARK:
 *
 * The dark color for the dark checkerboard type.
 **/
#define CLAY_CHECKS_DARK_COLOR_DARK ((gdouble[]) { 0.0, 0.0, 0.0, 1.0 })

/**
 * CLAY_CHECKS_DARK_COLOR_LIGHT:
 *
 * The light color for the dark checkerboard type.
 **/
#define CLAY_CHECKS_DARK_COLOR_LIGHT ((gdouble[]) { 0.2, 0.2, 0.2, 1.0 })

/**
 * CLAY_CHECKS_WHITE_COLOR:
 *
 * The light/dark color for the white checkerboard type.
 **/
#define CLAY_CHECKS_WHITE_COLOR ((gdouble[]) { 1.0, 1.0, 1.0, 1.0 })

/**
 * CLAY_CHECKS_GRAY_COLOR:
 *
 * The light/dark color for the gray checkerboard type.
 **/
#define CLAY_CHECKS_GRAY_COLOR ((gdouble[]) { 0.5, 0.5, 0.5, 1.0 })

/**
 * CLAY_CHECKS_BLACK_COLOR:
 *
 * The light/dark color for the black checkerboard type.
 **/
#define CLAY_CHECKS_BLACK_COLOR ((gdouble[]) { 0.0, 0.0, 0.0, 1.0 })

/**
 * CLAY_CHECKS_CUSTOM_COLOR1:
 *
 * The default color 1 for the custom checkerboard type.
 **/
#define CLAY_CHECKS_CUSTOM_COLOR1 CLAY_CHECKS_GRAY_COLOR_LIGHT

/**
 * CLAY_CHECKS_CUSTOM_COLOR2:
 *
 * The default color 2 for the custom checkerboard type.
 **/
#define CLAY_CHECKS_CUSTOM_COLOR2 CLAY_CHECKS_GRAY_COLOR_DARK


void  clay_checks_get_colors (ClayCheckType  type,
                              GeglColor     **color1,
                              GeglColor     **color2);


G_END_DECLS

#endif  /* __CLAY_CHECKS_H__ */
