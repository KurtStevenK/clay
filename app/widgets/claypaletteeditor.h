/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "claydataeditor.h"


#define CLAY_TYPE_PALETTE_EDITOR            (clay_palette_editor_get_type ())
#define CLAY_PALETTE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PALETTE_EDITOR, ClayPaletteEditor))
#define CLAY_PALETTE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PALETTE_EDITOR, ClayPaletteEditorClass))
#define CLAY_IS_PALETTE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PALETTE_EDITOR))
#define CLAY_IS_PALETTE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PALETTE_EDITOR))
#define CLAY_PALETTE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PALETTE_EDITOR, ClayPaletteEditorClass))


typedef struct _ClayPaletteEditorClass ClayPaletteEditorClass;

struct _ClayPaletteEditor
{
  ClayDataEditor    parent_instance;

  GtkWidget        *view;

  GtkWidget        *index_label;
  GtkWidget        *color_name;
  GtkAdjustment    *columns_adj;

  GtkWidget        *color_dialog;

  ClayPaletteEntry *color;

  gdouble           zoom_factor;  /* range from 0.1 to 4.0 */
  gint              col_width;
  gint              last_width;
  gint              columns;
};

struct _ClayPaletteEditorClass
{
  ClayDataEditorClass  parent_class;
};


GType       clay_palette_editor_get_type   (void);

GtkWidget * clay_palette_editor_new        (ClayContext        *context,
                                            ClayMenuFactory    *menu_factory);

void        clay_palette_editor_edit_color (ClayPaletteEditor  *editor);
void        clay_palette_editor_pick_color (ClayPaletteEditor  *editor,
                                            GeglColor          *color,
                                            ClayColorPickState  pick_state);
void        clay_palette_editor_zoom       (ClayPaletteEditor  *editor,
                                            ClayZoomType        zoom_type);

gint        clay_palette_editor_get_index  (ClayPaletteEditor *editor,
                                            GeglColor         *search);
gboolean    clay_palette_editor_set_index  (ClayPaletteEditor *editor,
                                            gint               index,
                                            GeglColor         *color);

gint        clay_palette_editor_max_index  (ClayPaletteEditor *editor);
