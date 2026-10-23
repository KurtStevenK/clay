/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayoffsetarea.h
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_OFFSET_AREA_H__
#define __CLAY_OFFSET_AREA_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */

#define CLAY_TYPE_OFFSET_AREA (clay_offset_area_get_type ())
G_DECLARE_FINAL_TYPE (ClayOffsetArea, clay_offset_area, CLAY, OFFSET_AREA, GtkDrawingArea)


GtkWidget * clay_offset_area_new         (gint            orig_width,
                                          gint            orig_height);
void        clay_offset_area_set_pixbuf  (ClayOffsetArea *offset_area,
                                          GdkPixbuf      *pixbuf);

void        clay_offset_area_set_size    (ClayOffsetArea *offset_area,
                                          gint            width,
                                          gint            height);
void        clay_offset_area_set_offsets (ClayOffsetArea *offset_area,
                                          gint            offset_x,
                                          gint            offset_y);


G_END_DECLS

#endif /* __CLAY_OFFSET_AREA_H__ */
