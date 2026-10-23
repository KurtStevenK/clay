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

#if !defined (__CLAY_COLOR_H_INSIDE__) && !defined (CLAY_COLOR_COMPILATION)
#error "Only <libclaycolor/claycolor.h> can be included directly."
#endif

#ifndef __CLAY_ADAPTIVE_SUPERSAMPLE_H__
#define __CLAY_ADAPTIVE_SUPERSAMPLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayRenderFunc:
 * @x:
 * @y:
 * @color: (out caller-allocates): The rendered pixel as RGB
 * @data: (closure):
 */
typedef void (* ClayRenderFunc)   (gdouble   x,
                                   gdouble   y,
                                   gdouble  *color,
                                   gpointer  data);
/**
 * ClayPutPixelFunc:
 * @x:
 * @y:
 * @color:
 * @data: (closure):
 */
typedef void (* ClayPutPixelFunc) (gint      x,
                                   gint      y,
                                   gdouble  *color,
                                   gpointer  data);
/**
 * ClayProgressFunc:
 * @min:
 * @max:
 * @current:
 * @data: (closure):
 */
typedef void (* ClayProgressFunc) (gint      min,
                                   gint      max,
                                   gint      current,
                                   gpointer  data);


gulong   clay_adaptive_supersample_area (gint              x1,
                                         gint              y1,
                                         gint              x2,
                                         gint              y2,
                                         gint              max_depth,
                                         gdouble           threshold,
                                         ClayRenderFunc    render_func,
                                         gpointer          render_data,
                                         ClayPutPixelFunc  put_pixel_func,
                                         gpointer          put_pixel_data,
                                         ClayProgressFunc  progress_func,
                                         gpointer          progress_data);


G_END_DECLS

#endif  /* __CLAY_ADAPTIVE_SUPERSAMPLE_H__ */
