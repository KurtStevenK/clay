/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claydrawablechooser.c
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

#ifndef __CLAY_DRAWABLE_CHOOSER_H__
#define __CLAY_DRAWABLE_CHOOSER_H__

G_BEGIN_DECLS

#define CLAY_TYPE_DRAWABLE_CHOOSER (clay_drawable_chooser_get_type ())
G_DECLARE_FINAL_TYPE (ClayDrawableChooser, clay_drawable_chooser, CLAY, DRAWABLE_CHOOSER, GtkBox)


CLAY_DEPRECATED_FOR(clay_item_chooser_new)
GtkWidget    * clay_drawable_chooser_new          (const gchar          *title,
                                                   const gchar          *label,
                                                   GType                 drawable_type,
                                                   ClayDrawable         *drawable);

CLAY_DEPRECATED_FOR(clay_item_chooser_get_item)
ClayDrawable * clay_drawable_chooser_get_drawable (ClayDrawableChooser  *chooser);

CLAY_DEPRECATED_FOR(clay_item_chooser_set_item)
void           clay_drawable_chooser_set_drawable (ClayDrawableChooser  *chooser,
                                                   ClayDrawable         *drawable);

CLAY_DEPRECATED_FOR(clay_item_chooser_get_label)
GtkWidget    * clay_drawable_chooser_get_label    (ClayDrawableChooser  *widget);


G_END_DECLS

#endif /* __CLAY_DRAWABLE_CHOOSER_H__ */
