/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypixbuf.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#ifndef __LIBCLAY_CLAY_PIXBUF_H__
#define __LIBCLAY_CLAY_PIXBUF_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


G_GNUC_INTERNAL GdkPixbuf * _clay_pixbuf_from_data (guchar                 *data,
                                                    gint                    width,
                                                    gint                    height,
                                                    gint                    bpp,
                                                    ClayPixbufTransparency  alpha);


G_END_DECLS

#endif /* __LIBCLAY_CLAY_PIXBUF_H__ */
