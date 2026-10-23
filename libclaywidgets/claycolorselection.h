/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorselection.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_COLOR_SELECTION_H__
#define __CLAY_COLOR_SELECTION_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define CLAY_TYPE_COLOR_SELECTION (clay_color_selection_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorSelection, clay_color_selection, CLAY, COLOR_SELECTION, GtkBox)


GtkWidget * clay_color_selection_new            (void);

void        clay_color_selection_set_show_alpha (ClayColorSelection *selection,
                                                 gboolean           show_alpha);
gboolean    clay_color_selection_get_show_alpha (ClayColorSelection *selection);

void        clay_color_selection_set_color      (ClayColorSelection *selection,
                                                 GeglColor          *color);
GeglColor * clay_color_selection_get_color      (ClayColorSelection *selection);

void        clay_color_selection_set_old_color  (ClayColorSelection *selection,
                                                 GeglColor          *color);
GeglColor * clay_color_selection_get_old_color  (ClayColorSelection *selection);

void        clay_color_selection_reset          (ClayColorSelection *selection);

void        clay_color_selection_color_changed  (ClayColorSelection *selection);

void        clay_color_selection_set_format     (ClayColorSelection *selection,
                                                 const Babl          *format);
void        clay_color_selection_set_simulation (ClayColorSelection *selection,
                                                 ClayColorProfile   *profile,
                                                 ClayColorRenderingIntent intent,
                                                 gboolean            bpc);

void        clay_color_selection_set_config     (ClayColorSelection *selection,
                                                 ClayColorConfig    *config);

GtkWidget * clay_color_selection_get_notebook   (ClayColorSelection *selection);
GtkWidget * clay_color_selection_get_right_vbox (ClayColorSelection *selection);


G_END_DECLS

#endif /* __CLAY_COLOR_SELECTION_H__ */
