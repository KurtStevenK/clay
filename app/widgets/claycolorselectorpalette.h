/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorselectorpalette.h
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define CLAY_TYPE_COLOR_SELECTOR_PALETTE            (clay_color_selector_palette_get_type ())
#define CLAY_COLOR_SELECTOR_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_SELECTOR_PALETTE, ClayColorSelectorPalette))
#define CLAY_IS_COLOR_SELECTOR_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_SELECTOR_PALETTE))
#define CLAY_COLOR_SELECTOR_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_SELECTOR_PALETTE, ClayColorSelectorPaletteClass))
#define CLAY_IS_COLOR_SELECTOR_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_SELECTOR_PALETTE))
#define CLAY_COLOR_SELECTOR_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_SELECTOR_PALETTE, ClayColorSelectorPaletteClass))


typedef struct _ClayColorSelectorPalette      ClayColorSelectorPalette;
typedef struct _ClayColorSelectorPaletteClass ClayColorSelectorPaletteClass;

struct _ClayColorSelectorPalette
{
  ClayColorSelector  parent_instance;

  ClayContext       *context;
  GtkWidget         *view;
  GtkWidget         *name_label;
};

struct _ClayColorSelectorPaletteClass
{
  ClayColorSelectorClass  parent_class;
};


GType   clay_color_selector_palette_get_type (void);
