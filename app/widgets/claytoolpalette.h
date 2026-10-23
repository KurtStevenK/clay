/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolpalette.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_TOOL_PALETTE            (clay_tool_palette_get_type ())
#define CLAY_TOOL_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_PALETTE, ClayToolPalette))
#define CLAY_TOOL_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_PALETTE, ClayToolPaletteClass))
#define CLAY_IS_TOOL_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_PALETTE))
#define CLAY_IS_TOOL_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_PALETTE))
#define CLAY_TOOL_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_PALETTE, ClayToolPaletteClass))


typedef struct _ClayToolPaletteClass ClayToolPaletteClass;

struct _ClayToolPalette
{
  GtkToolPalette  parent_instance;
};

struct _ClayToolPaletteClass
{
  GtkToolPaletteClass  parent_class;
};


GType       clay_tool_palette_get_type        (void);

GtkWidget * clay_tool_palette_new             (void);
void        clay_tool_palette_set_toolbox     (ClayToolPalette   *palette,
                                               ClayToolbox       *toolbox);
gboolean    clay_tool_palette_get_button_size (ClayToolPalette   *palette,
                                               gint              *width,
                                               gint              *height);
