/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycellrendererviewable.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CELL_RENDERER_VIEWABLE            (clay_cell_renderer_viewable_get_type ())
#define CLAY_CELL_RENDERER_VIEWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CELL_RENDERER_VIEWABLE, ClayCellRendererViewable))
#define CLAY_CELL_RENDERER_VIEWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CELL_RENDERER_VIEWABLE, ClayCellRendererViewableClass))
#define CLAY_IS_CELL_RENDERER_VIEWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CELL_RENDERER_VIEWABLE))
#define CLAY_IS_CELL_RENDERER_VIEWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CELL_RENDERER_VIEWABLE))
#define CLAY_CELL_RENDERER_VIEWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CELL_RENDERER_VIEWABLE, ClayCellRendererViewableClass))


typedef struct _ClayCellRendererViewableClass ClayCellRendererViewableClass;

struct _ClayCellRendererViewable
{
  GtkCellRenderer   parent_instance;

  ClayViewRenderer *renderer;
};

struct _ClayCellRendererViewableClass
{
  GtkCellRendererClass  parent_class;

  gboolean (* pre_clicked) (ClayCellRendererViewable *cell,
                            const gchar              *path,
                            GdkModifierType           state);
  void     (* clicked)     (ClayCellRendererViewable *cell,
                            const gchar              *path,
                            GdkModifierType           state);
};


GType             clay_cell_renderer_viewable_get_type    (void);
GtkCellRenderer * clay_cell_renderer_viewable_new         (void);
gboolean          clay_cell_renderer_viewable_pre_clicked (ClayCellRendererViewable *cell,
                                                           const gchar              *path,
                                                           GdkModifierType           state);
void              clay_cell_renderer_viewable_clicked     (ClayCellRendererViewable *cell,
                                                           const gchar              *path,
                                                           GdkModifierType           state);
