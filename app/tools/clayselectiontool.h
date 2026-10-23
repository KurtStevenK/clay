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

#include "claydrawtool.h"


#define CLAY_TYPE_SELECTION_TOOL            (clay_selection_tool_get_type ())
#define CLAY_SELECTION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SELECTION_TOOL, ClaySelectionTool))
#define CLAY_SELECTION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SELECTION_TOOL, ClaySelectionToolClass))
#define CLAY_IS_SELECTION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SELECTION_TOOL))
#define CLAY_IS_SELECTION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SELECTION_TOOL))
#define CLAY_SELECTION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SELECTION_TOOL, ClaySelectionToolClass))

#define CLAY_SELECTION_TOOL_GET_OPTIONS(t)  (CLAY_SELECTION_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClaySelectionTool      ClaySelectionTool;
typedef struct _ClaySelectionToolClass ClaySelectionToolClass;

struct _ClaySelectionTool
{
  ClayDrawTool    parent_instance;

  SelectFunction  function;         /*  selection function        */
  ClayChannelOps  saved_operation;  /*  saved tool options state  */
  GdkModifierType held_keys;        /*  only keys that have been pressed are
                                        used to determine the modified
                                        operation type - see issue #1589 */

  gint            change_count;
  gboolean        saved_show_selection;
  ClayUndo       *undo;
  ClayUndo       *redo;
  gint            idle_id;

  gboolean        allow_move;
};

struct _ClaySelectionToolClass
{
  ClayDrawToolClass  parent_class;

  /*  virtual functions  */
  gboolean (* have_selection) (ClaySelectionTool *sel_tool,
                               ClayDisplay       *display);
};


GType      clay_selection_tool_get_type     (void);

/*  protected function  */
gboolean   clay_selection_tool_start_edit   (ClaySelectionTool *sel_tool,
                                             ClayDisplay       *display,
                                             const ClayCoords  *coords);

void       clay_selection_tool_start_change (ClaySelectionTool *sel_tool,
                                             gboolean           create,
                                             ClayChannelOps     operation);
void       clay_selection_tool_end_change   (ClaySelectionTool *sel_tool,
                                             gboolean           cancel);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClaySelectionTool, g_object_unref);
