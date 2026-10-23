/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabelcolor.h
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

#ifndef __CLAY_LABEL_COLOR_H__
#define __CLAY_LABEL_COLOR_H__

#include <libclaywidgets/claylabeled.h>

G_BEGIN_DECLS

#define CLAY_TYPE_LABEL_COLOR (clay_label_color_get_type ())
G_DECLARE_FINAL_TYPE (ClayLabelColor, clay_label_color, CLAY, LABEL_COLOR, ClayLabeled)


GtkWidget * clay_label_color_new              (const gchar       *label,
                                               GeglColor         *color,
                                               gboolean           editable);

/* TODO: it would be interesting for such a widget to have an API to
 * customize the label being also above or below, left or right. I could
 * imagine wanting to pretty-list several colors with specific layouts.
 */

void        clay_label_color_set_value        (ClayLabelColor     *color,
                                               GeglColor          *value);
GeglColor * clay_label_color_get_value        (ClayLabelColor     *color);

void        clay_label_color_set_editable     (ClayLabelColor     *color,
                                               gboolean            editable);
gboolean    clay_label_color_is_editable      (ClayLabelColor     *color);

GtkWidget * clay_label_color_get_color_widget (ClayLabelColor     *color);

G_END_DECLS

#endif /* __CLAY_LABEL_COLOR_H__ */
