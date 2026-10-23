/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypropwidgets.h
 * Copyright (C) 2023 Jehan
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

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif


#ifndef __CLAY_PROP_WIDGETS_H__
#define __CLAY_PROP_WIDGETS_H__

G_BEGIN_DECLS


/*  ClayParamResource  */

GtkWidget     * clay_prop_brush_chooser_new     (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);
GtkWidget     * clay_prop_font_chooser_new      (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);
GtkWidget     * clay_prop_gradient_chooser_new  (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);
GtkWidget     * clay_prop_palette_chooser_new   (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);
GtkWidget     * clay_prop_pattern_chooser_new   (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);

GtkWidget     * clay_prop_image_chooser_new     (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);
GtkWidget     * clay_prop_item_chooser_new      (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);

CLAY_DEPRECATED_FOR(clay_prop_item_chooser_new)
GtkWidget     * clay_prop_drawable_chooser_new  (GObject              *config,
                                                 const gchar          *property_name,
                                                 const gchar          *chooser_title);


G_END_DECLS

#endif /* __CLAY_PROP_WIDGETS_H__ */
