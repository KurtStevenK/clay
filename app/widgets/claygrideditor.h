/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGridEditor
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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


#define CLAY_TYPE_GRID_EDITOR            (clay_grid_editor_get_type ())
#define CLAY_GRID_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GRID_EDITOR, ClayGridEditor))
#define CLAY_GRID_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GRID_EDITOR, ClayGridEditorClass))
#define CLAY_IS_GRID_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GRID_EDITOR))
#define CLAY_IS_GRID_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GRID_EDITOR))
#define CLAY_GRID_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GRID_EDITOR, ClayGridEditorClass))


typedef struct _ClayGridEditorClass ClayGridEditorClass;

struct _ClayGridEditor
{
  GtkBox       parent_instance;

  ClayGrid    *grid;
  ClayContext *context;
  gdouble      xresolution;
  gdouble      yresolution;
};

struct _ClayGridEditorClass
{
  GtkBoxClass  parent_class;
};


GType       clay_grid_editor_get_type (void);

GtkWidget * clay_grid_editor_new      (ClayGrid    *grid,
                                       ClayContext *context,
                                       gdouble      xresolution,
                                       gdouble      yresolution);
