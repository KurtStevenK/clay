/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycairo-utils.h
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_CAIRO_UTILS_H__
#define __CLAY_CAIRO_UTILS_H__


gboolean          clay_cairo_set_focus_line_pattern     (cairo_t       *cr,
                                                         GtkWidget     *widget);

cairo_surface_t * clay_cairo_surface_create_from_pixbuf (GdkPixbuf     *pixbuf);

void              clay_cairo_set_source_color           (cairo_t         *cr,
                                                         GeglColor       *color,
                                                         ClayColorConfig *config,
                                                         gboolean         softproof,
                                                         GtkWidget       *widget);


#endif /* __CLAY_CAIRO_UTILS_H__ */
