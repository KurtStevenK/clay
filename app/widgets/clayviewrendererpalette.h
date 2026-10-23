/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererpalette.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include "clayviewrenderer.h"


#define CLAY_TYPE_VIEW_RENDERER_PALETTE            (clay_view_renderer_palette_get_type ())
#define CLAY_VIEW_RENDERER_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEW_RENDERER_PALETTE, ClayViewRendererPalette))
#define CLAY_VIEW_RENDERER_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEW_RENDERER_PALETTE, ClayViewRendererPaletteClass))
#define CLAY_IS_VIEW_RENDERER_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_VIEW_RENDERER_PALETTE))
#define CLAY_IS_VIEW_RENDERER_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEW_RENDERER_PALETTE))
#define CLAY_VIEW_RENDERER_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEW_RENDERER_PALETTE, ClayViewRendererPaletteClass))


typedef struct _ClayViewRendererPaletteClass  ClayViewRendererPaletteClass;

struct _ClayViewRendererPalette
{
  ClayViewRenderer  parent_instance;

  gint              cell_size;
  gboolean          draw_grid;

  gint              cell_width;
  gint              cell_height;
  gint              columns;
  gint              rows;
};

struct _ClayViewRendererPaletteClass
{
  ClayViewRendererClass  parent_class;
};


GType   clay_view_renderer_palette_get_type    (void);

void    clay_view_renderer_palette_set_cell_size (ClayViewRendererPalette *renderer,
                                                  gint                     cell_size);
void    clay_view_renderer_palette_set_draw_grid (ClayViewRendererPalette *renderer,
                                                  gboolean                 draw_grid);
