/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayenumstore.h
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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_ENUM_STORE_H__
#define __CLAY_ENUM_STORE_H__

#include <libclaywidgets/clayintstore.h>


G_BEGIN_DECLS

#define CLAY_TYPE_ENUM_STORE (clay_enum_store_get_type ())
G_DECLARE_FINAL_TYPE (ClayEnumStore, clay_enum_store, CLAY, ENUM_STORE, ClayIntStore)


GtkListStore * clay_enum_store_new                    (GType    enum_type);
GtkListStore * clay_enum_store_new_with_range         (GType    enum_type,
                                                       gint     minimum,
                                                       gint     maximum);
GtkListStore * clay_enum_store_new_with_values        (GType    enum_type,
                                                       gint     n_values,
                                                       ...);
GtkListStore * clay_enum_store_new_with_values_valist (GType    enum_type,
                                                       gint     n_values,
                                                       va_list  args);

void           clay_enum_store_set_icon_prefix  (ClayEnumStore *store,
                                                 const gchar   *icon_prefix);


G_END_DECLS

#endif  /* __CLAY_ENUM_STORE_H__ */
