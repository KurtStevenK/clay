/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooloptionseditor.h
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

#include "clayeditor.h"


#define CLAY_TYPE_TOOL_OPTIONS_EDITOR            (clay_tool_options_editor_get_type ())
#define CLAY_TOOL_OPTIONS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_OPTIONS_EDITOR, ClayToolOptionsEditor))
#define CLAY_TOOL_OPTIONS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_OPTIONS_EDITOR, ClayToolOptionsEditorClass))
#define CLAY_IS_TOOL_OPTIONS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_OPTIONS_EDITOR))
#define CLAY_IS_TOOL_OPTIONS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_OPTIONS_EDITOR))
#define CLAY_TOOL_OPTIONS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_OPTIONS_EDITOR, ClayToolOptionsEditorClass))


typedef struct _ClayToolOptionsEditorPrivate  ClayToolOptionsEditorPrivate;
typedef struct _ClayToolOptionsEditorClass    ClayToolOptionsEditorClass;

struct _ClayToolOptionsEditor
{
  ClayEditor                    parent_instance;

  ClayToolOptionsEditorPrivate *p;
};

struct _ClayToolOptionsEditorClass
{
  ClayEditorClass  parent_class;
};


GType             clay_tool_options_editor_get_type         (void);
GtkWidget       * clay_tool_options_editor_new              (Clay                  *clay,
                                                             ClayMenuFactory       *menu_factory);
ClayToolOptions * clay_tool_options_editor_get_tool_options (ClayToolOptionsEditor *editor);
