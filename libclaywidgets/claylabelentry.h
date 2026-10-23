/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabelentry.h
 * Copyright (C) 2022 Jehan
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

#ifndef __CLAY_LABEL_ENTRY_H__
#define __CLAY_LABEL_ENTRY_H__

#include <libclaywidgets/claylabeled.h>

G_BEGIN_DECLS

#define CLAY_TYPE_LABEL_ENTRY (clay_label_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayLabelEntry, clay_label_entry, CLAY, LABEL_ENTRY, ClayLabeled)


GtkWidget   * clay_label_entry_new        (const gchar   *label);

void          clay_label_entry_set_value  (ClayLabelEntry *entry,
                                           const gchar    *value);
const gchar * clay_label_entry_get_value  (ClayLabelEntry *entry);

GtkWidget   * clay_label_entry_get_entry  (ClayLabelEntry *entry);

G_END_DECLS

#endif /* __CLAY_LABEL_ENTRY_H__ */
