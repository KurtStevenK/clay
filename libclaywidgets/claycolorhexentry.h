/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorhexentry.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_HEX_ENTRY_H__
#define __CLAY_COLOR_HEX_ENTRY_H__

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_HEX_ENTRY (clay_color_hex_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorHexEntry, clay_color_hex_entry, CLAY, COLOR_HEX_ENTRY, GtkEntry)


GtkWidget * clay_color_hex_entry_new       (void);

void        clay_color_hex_entry_set_color (ClayColorHexEntry *entry,
                                            GeglColor         *color);
GeglColor * clay_color_hex_entry_get_color (ClayColorHexEntry *entry);


G_END_DECLS

#endif /* __CLAY_COLOR_HEX_ENTRY_H__ */
