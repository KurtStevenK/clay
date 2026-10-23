/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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


#define CLAY_TYPE_TOOL_PRESET_EDITOR            (clay_tool_preset_editor_get_type ())
#define CLAY_TOOL_PRESET_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_PRESET_EDITOR, ClayToolPresetEditor))
#define CLAY_TOOL_PRESET_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_PRESET_EDITOR, ClayToolPresetEditorClass))
#define CLAY_IS_TOOL_PRESET_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_PRESET_EDITOR))
#define CLAY_IS_TOOL_PRESET_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_PRESET_EDITOR))
#define CLAY_TOOL_PRESET_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_PRESET_EDITOR, ClayToolPresetEditorClass))


typedef struct _ClayToolPresetEditorPrivate ClayToolPresetEditorPrivate;
typedef struct _ClayToolPresetEditorClass   ClayToolPresetEditorClass;

struct _ClayToolPresetEditor
{
  ClayDataEditor               parent_instance;

  ClayToolPresetEditorPrivate *priv;
};

struct _ClayToolPresetEditorClass
{
  ClayDataEditorClass  parent_class;
};


GType       clay_tool_preset_editor_get_type (void);

GtkWidget * clay_tool_preset_editor_new      (ClayContext     *context,
                                              ClayMenuFactory *menu_factory);
