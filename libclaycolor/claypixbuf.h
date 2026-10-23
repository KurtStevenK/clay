/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claypixbuf.h
 * Copyright (C) 2012  Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_PIXBUF_H__
#define __CLAY_PIXBUF_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


const Babl * clay_pixbuf_get_format      (GdkPixbuf *pixbuf);
GeglBuffer * clay_pixbuf_create_buffer   (GdkPixbuf *pixbuf);

guint8     * clay_pixbuf_get_icc_profile (GdkPixbuf *pixbuf,
                                          gsize     *length);


G_END_DECLS

#endif  /* __CLAY_PIXBUF_H__ */
