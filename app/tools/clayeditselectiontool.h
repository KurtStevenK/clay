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


#define CLAY_TYPE_EDIT_SELECTION_TOOL            (clay_edit_selection_tool_get_type ())
#define CLAY_EDIT_SELECTION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_EDIT_SELECTION_TOOL, ClayEditSelectionTool))
#define CLAY_EDIT_SELECTION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_EDIT_SELECTION_TOOL, ClayEditSelectionToolClass))
#define CLAY_IS_EDIT_SELECTION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_EDIT_SELECTION_TOOL))
#define CLAY_IS_EDIT_SELECTION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_EDIT_SELECTION_TOOL))


GType      clay_edit_selection_tool_get_type  (void);

void       clay_edit_selection_tool_start     (ClayTool          *parent_tool,
                                               ClayDisplay       *display,
                                               const ClayCoords  *coords,
                                               ClayTranslateMode  edit_mode,
                                               gboolean           propagate_release);

gboolean   clay_edit_selection_tool_key_press (ClayTool          *tool,
                                               GdkEventKey       *kevent,
                                               ClayDisplay       *display);
gboolean   clay_edit_selection_tool_translate (ClayTool          *tool,
                                               GdkEventKey       *kevent,
                                               ClayTransformType  translate_type,
                                               ClayDisplay       *display,
                                               GtkWidget        **type_box);
