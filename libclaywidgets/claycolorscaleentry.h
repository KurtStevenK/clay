/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorscaleentry.h
 * Copyright (C) 2020 Jehan
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

#ifndef __CLAY_COLOR_SCALE_ENTRY_H__
#define __CLAY_COLOR_SCALE_ENTRY_H__

#include <libclaywidgets/clayscaleentry.h>

G_BEGIN_DECLS

#define CLAY_TYPE_COLOR_SCALE_ENTRY (clay_color_scale_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorScaleEntry, clay_color_scale_entry, CLAY, COLOR_SCALE_ENTRY, ClayScaleEntry)

GtkWidget     * clay_color_scale_entry_new        (const gchar *text,
                                                   gdouble      value,
                                                   gdouble      lower,
                                                   gdouble      upper,
                                                   guint        digits);


G_END_DECLS

#endif /* __CLAY_COLOR_SCALE_ENTRY_H__ */
