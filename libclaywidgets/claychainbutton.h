/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claychainbutton.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
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

/*
 * This implements a widget derived from GtkGrid that visualizes
 * it's state with two different pixmaps showing a closed and a
 * broken chain. It's intended to be used with the ClaySizeEntry
 * widget. The usage is quite similar to the one the GtkToggleButton
 * provides.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_CHAIN_BUTTON_H__
#define __CLAY_CHAIN_BUTTON_H__

G_BEGIN_DECLS


#define CLAY_TYPE_CHAIN_BUTTON (clay_chain_button_get_type ())
G_DECLARE_FINAL_TYPE (ClayChainButton, clay_chain_button, CLAY, CHAIN_BUTTON, GtkGrid)


GtkWidget   * clay_chain_button_new           (ClayChainPosition  position);

void          clay_chain_button_set_icon_size (ClayChainButton   *button,
                                               GtkIconSize        size);
GtkIconSize   clay_chain_button_get_icon_size (ClayChainButton   *button);

void          clay_chain_button_set_active    (ClayChainButton   *button,
                                               gboolean           active);
gboolean      clay_chain_button_get_active    (ClayChainButton   *button);

GtkWidget   * clay_chain_button_get_button    (ClayChainButton   *button);


G_END_DECLS

#endif /* __CLAY_CHAIN_BUTTON_H__ */
