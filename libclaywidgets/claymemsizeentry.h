/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claymemsizeentry.h
 * Copyright (C) 2000-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_MEMSIZE_ENTRY_H__
#define __CLAY_MEMSIZE_ENTRY_H__

G_BEGIN_DECLS


#define CLAY_TYPE_MEMSIZE_ENTRY (clay_memsize_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayMemsizeEntry, clay_memsize_entry, CLAY, MEMSIZE_ENTRY, GtkBox)


GtkWidget * clay_memsize_entry_new            (guint64           value,
                                               guint64           lower,
                                               guint64           upper);
void        clay_memsize_entry_set_value      (ClayMemsizeEntry *entry,
                                               guint64           value);
guint64     clay_memsize_entry_get_value      (ClayMemsizeEntry *entry);

GtkWidget * clay_memsize_entry_get_spinbutton (ClayMemsizeEntry *entry);


G_END_DECLS

#endif /* __CLAY_MEMSIZE_ENTRY_H__ */
