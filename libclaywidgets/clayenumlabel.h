/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayenumlabel.h
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_ENUM__LABEL_H__
#define __CLAY_ENUM__LABEL_H__

G_BEGIN_DECLS


#define CLAY_TYPE_ENUM_LABEL (clay_enum_label_get_type ())
G_DECLARE_FINAL_TYPE (ClayEnumLabel, clay_enum_label, CLAY, ENUM_LABEL, GtkScale)


GtkWidget * clay_enum_label_new              (GType          enum_type,
                                              gint           value);
void        clay_enum_label_set_value        (ClayEnumLabel *label,
                                              gint           value);


G_END_DECLS

#endif  /* __CLAY_ENUM_LABEL_H__ */
