/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooleditor.h
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
 *                         Stephen Griffiths <scgmk5@gmail.com>
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

#include "claycontainertreeview.h"


#define CLAY_TYPE_TOOL_EDITOR            (clay_tool_editor_get_type ())
#define CLAY_TOOL_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_EDITOR, ClayToolEditor))
#define CLAY_TOOL_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_EDITOR, ClayToolEditorClass))
#define CLAY_IS_TOOL_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_EDITOR))
#define CLAY_IS_TOOL_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_EDITOR))
#define CLAY_TOOL_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_EDITOR, ClayToolEditorClass))


typedef struct _ClayToolEditorPrivate ClayToolEditorPrivate;
typedef struct _ClayToolEditorClass   ClayToolEditorClass;

struct _ClayToolEditor
{
  ClayContainerTreeView  parent_instance;

  ClayToolEditorPrivate *priv;
};

struct _ClayToolEditorClass
{
  ClayContainerTreeViewClass  parent_class;
};


GType       clay_tool_editor_get_type       (void);

GtkWidget * clay_tool_editor_new            (ClayContainer  *container,
                                             ClayContext    *context,
                                             gint            view_size,
                                             gint            view_border_width);

void        clay_tool_editor_revert_changes (ClayToolEditor *tool_editor);
