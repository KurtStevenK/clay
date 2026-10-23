/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypaletteview.h
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

#include "clayview.h"


#define CLAY_TYPE_PALETTE_VIEW            (clay_palette_view_get_type ())
#define CLAY_PALETTE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PALETTE_VIEW, ClayPaletteView))
#define CLAY_PALETTE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PALETTE_VIEW, ClayPaletteViewClass))
#define CLAY_IS_PALETTE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_PALETTE_VIEW))
#define CLAY_IS_PALETTE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PALETTE_VIEW))
#define CLAY_PALETTE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PALETTE_VIEW, ClayPaletteViewClass))


typedef struct _ClayPaletteViewClass  ClayPaletteViewClass;

struct _ClayPaletteView
{
  ClayView          parent_instance;

  ClayPaletteEntry *selected;
  ClayPaletteEntry *dnd_entry;
};

struct _ClayPaletteViewClass
{
  ClayViewClass  parent_class;

  void (* entry_clicked)   (ClayPaletteView  *view,
                            ClayPaletteEntry *entry,
                            GdkModifierType   state);
  void (* entry_selected)  (ClayPaletteView  *view,
                            ClayPaletteEntry *entry);
  void (* entry_activated) (ClayPaletteView  *view,
                            ClayPaletteEntry *entry);
  void (* entry_context)   (ClayPaletteView  *view,
                            ClayPaletteEntry *entry);
  void (* color_dropped)   (ClayPaletteView  *view,
                            ClayPaletteEntry *entry,
                            GeglColor        *color);
};


GType   clay_palette_view_get_type     (void);

void    clay_palette_view_select_entry (ClayPaletteView  *view,
                                        ClayPaletteEntry *entry);

ClayPaletteEntry * clay_palette_view_get_selected_entry (ClayPaletteView *view);

void               clay_palette_view_get_entry_rect     (ClayPaletteView  *view,
                                                         ClayPaletteEntry *entry,
                                                         GdkRectangle     *rect);
