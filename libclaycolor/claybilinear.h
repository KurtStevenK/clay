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

#ifndef __CLAY_BILINEAR_H__
#define __CLAY_BILINEAR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  bilinear interpolation functions taken from LibGCK  */


gdouble   clay_bilinear          (gdouble    x,
                                  gdouble    y,
                                  gdouble   *values);
guchar    clay_bilinear_8        (gdouble    x,
                                  gdouble    y,
                                  guchar    *values);
guint16   clay_bilinear_16       (gdouble    x,
                                  gdouble    y,
                                  guint16   *values);
guint32   clay_bilinear_32       (gdouble    x,
                                  gdouble    y,
                                  guint32   *values);
void      clay_bilinear_rgb      (gdouble    x,
                                  gdouble    y,
                                  gdouble   *values,
                                  gboolean   has_alpha,
                                  gdouble   *retvalues);


G_END_DECLS

#endif  /* __CLAY_BILINEAR_H__ */
