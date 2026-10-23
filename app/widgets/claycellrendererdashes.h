/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycellrendererdashes.h
 * Copyright (C) 2005 Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_CELL_RENDERER_DASHES            (clay_cell_renderer_dashes_get_type ())
#define CLAY_CELL_RENDERER_DASHES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CELL_RENDERER_DASHES, ClayCellRendererDashes))
#define CLAY_CELL_RENDERER_DASHES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CELL_RENDERER_DASHES, ClayCellRendererDashesClass))
#define CLAY_IS_CELL_RENDERER_DASHES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CELL_RENDERER_DASHES))
#define CLAY_IS_CELL_RENDERER_DASHES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CELL_RENDERER_DASHES))
#define CLAY_CELL_RENDERER_DASHES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CELL_RENDERER_DASHES, ClayCellRendererDashesClass))


typedef struct _ClayCellRendererDashesClass ClayCellRendererDashesClass;

struct _ClayCellRendererDashes
{
  GtkCellRenderer   parent_instance;

  gboolean         *segments;
};

struct _ClayCellRendererDashesClass
{
  GtkCellRendererClass  parent_class;
};


GType             clay_cell_renderer_dashes_get_type (void);

GtkCellRenderer * clay_cell_renderer_dashes_new      (void);
