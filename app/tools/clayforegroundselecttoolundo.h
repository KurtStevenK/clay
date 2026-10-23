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

#if 0

#pragma once

#include "core/clayundo.h"


#define CLAY_TYPE_FOREGROUND_SELECT_TOOL_UNDO            (clay_foreground_select_tool_undo_get_type ())
#define CLAY_FOREGROUND_SELECT_TOOL_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FOREGROUND_SELECT_TOOL_UNDO, ClayForegroundSelectToolUndo))
#define CLAY_FOREGROUND_SELECT_TOOL_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FOREGROUND_SELECT_TOOL_UNDO, ClayForegroundSelectToolUndoClass))
#define CLAY_IS_FOREGROUND_SELECT_TOOL_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FOREGROUND_SELECT_TOOL_UNDO))
#define CLAY_IS_FOREGROUND_SELECT_TOOL_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FOREGROUND_SELECT_TOOL_UNDO))
#define CLAY_FOREGROUND_SELECT_TOOL_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FOREGROUND_SELECT_TOOL_UNDO, ClayForegroundSelectToolUndoClass))


typedef struct _ClayForegroundSelectToolUndo      ClayForegroundSelectToolUndo;
typedef struct _ClayForegroundSelectToolUndoClass ClayForegroundSelectToolUndoClass;

struct _ClayForegroundSelectToolUndo
{
  ClayUndo                  parent_instance;

  ClayForegroundSelectTool *foreground_select_tool;
};

struct _ClayForegroundSelectToolUndoClass
{
  ClayUndoClass  parent_class;
};


GType   clay_foreground_select_tool_undo_get_type (void);

#endif
