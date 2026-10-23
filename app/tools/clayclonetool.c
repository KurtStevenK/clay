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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "paint/clayclone.h"
#include "paint/claycloneoptions.h"

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"

#include "clayclonetool.h"
#include "claycloneoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static gboolean   clay_clone_tool_is_alpha_only (ClayPaintTool *paint_tool,
                                                 ClayDrawable  *drawable);


G_DEFINE_TYPE (ClayCloneTool, clay_clone_tool, CLAY_TYPE_SOURCE_TOOL)

#define parent_class clay_clone_tool_parent_class


void
clay_clone_tool_register (ClayToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (CLAY_TYPE_CLONE_TOOL,
                CLAY_TYPE_CLONE_OPTIONS,
                clay_clone_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_EXPAND   |
                CLAY_CONTEXT_PROP_MASK_PATTERN,
                "clay-clone-tool",
                _("Clone"),
                _("Clone Tool: Selectively copy from an image or pattern, using a brush"),
                N_("_Clone"), "C",
                NULL, CLAY_HELP_TOOL_CLONE,
                CLAY_ICON_TOOL_CLONE,
                data);
}

static void
clay_clone_tool_class_init (ClayCloneToolClass *klass)
{
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  paint_tool_class->is_alpha_only = clay_clone_tool_is_alpha_only;
}

static void
clay_clone_tool_init (ClayCloneTool *clone)
{
  ClayTool       *tool        = CLAY_TOOL (clone);
  ClayPaintTool  *paint_tool  = CLAY_PAINT_TOOL (tool);
  ClaySourceTool *source_tool = CLAY_SOURCE_TOOL (tool);

  clay_tool_control_set_tool_cursor     (tool->control,
                                         CLAY_TOOL_CURSOR_CLONE);
  clay_tool_control_set_action_object_2 (tool->control,
                                         "context-pattern-select-set");

  paint_tool->status      = _("Click to clone");
  paint_tool->status_ctrl = _("%s to set a new clone source");

  source_tool->status_paint           = _("Click to clone");
  source_tool->status_set_source      = _("Click to set a new clone source");
  source_tool->status_set_source_ctrl = _("%s to set a new clone source");
  source_tool->status_set_source_ctrl_click = _("%s-click to set a new clone source");
}

static gboolean
clay_clone_tool_is_alpha_only (ClayPaintTool *paint_tool,
                               ClayDrawable  *drawable)
{
  ClayPaintOptions *paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  ClayContext      *context       = CLAY_CONTEXT (paint_options);
  ClayLayerMode     paint_mode    = clay_context_get_paint_mode (context);

  return clay_layer_mode_is_alpha_only (paint_mode);
}
