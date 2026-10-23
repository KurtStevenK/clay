/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycellrendererbutton.h
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CELL_RENDERER_BUTTON            (clay_cell_renderer_button_get_type ())
#define CLAY_CELL_RENDERER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CELL_RENDERER_BUTTON, ClayCellRendererButton))
#define CLAY_CELL_RENDERER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CELL_RENDERER_BUTTON, ClayCellRendererButtonClass))
#define CLAY_IS_CELL_RENDERER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CELL_RENDERER_BUTTON))
#define CLAY_IS_CELL_RENDERER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CELL_RENDERER_BUTTON))
#define CLAY_CELL_RENDERER_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CELL_RENDERER_BUTTON, ClayCellRendererButtonClass))


typedef struct _ClayCellRendererButtonClass ClayCellRendererButtonClass;

struct _ClayCellRendererButton
{
  GtkCellRendererPixbuf  parent_instance;
};

struct _ClayCellRendererButtonClass
{
  GtkCellRendererPixbufClass  parent_class;

  void (* clicked) (ClayCellRendererButton *cell,
                    const gchar            *path,
                    GdkModifierType         state);
};


GType             clay_cell_renderer_button_get_type (void);

GtkCellRenderer * clay_cell_renderer_button_new      (void);

void              clay_cell_renderer_button_clicked  (ClayCellRendererButton *cell,
                                                      const gchar            *path,
                                                      GdkModifierType         state);
