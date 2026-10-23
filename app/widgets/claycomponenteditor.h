/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycomponenteditor.h
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

#include "clayimageeditor.h"


#define CLAY_TYPE_COMPONENT_EDITOR            (clay_component_editor_get_type ())
#define CLAY_COMPONENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COMPONENT_EDITOR, ClayComponentEditor))
#define CLAY_COMPONENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COMPONENT_EDITOR, ClayComponentEditorClass))
#define CLAY_IS_COMPONENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COMPONENT_EDITOR))
#define CLAY_IS_COMPONENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COMPONENT_EDITOR))
#define CLAY_COMPONENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COMPONENT_EDITOR, ClayComponentEditorClass))


typedef struct _ClayComponentEditorClass  ClayComponentEditorClass;

struct _ClayComponentEditor
{
  ClayImageEditor    parent_instance;

  gint               view_size;

  GtkTreeModel      *model;
  GtkTreeView       *view;
  GtkTreeSelection  *selection;

  GtkTreeViewColumn *eye_column;
  GtkCellRenderer   *eye_cell;
  GtkCellRenderer   *renderer_cell;

  ClayChannelType    clicked_component;
};

struct _ClayComponentEditorClass
{
  ClayImageEditorClass  parent_class;
};


GType       clay_component_editor_get_type      (void);

GtkWidget * clay_component_editor_new           (gint                 view_size,
                                                 ClayMenuFactory     *menu_factory);
void        clay_component_editor_set_view_size (ClayComponentEditor *editor,
                                                 gint                 view_size);
