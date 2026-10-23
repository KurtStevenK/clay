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

#include "paint/claypaintoptions.h"

#include "widgets/clayhelp-ids.h"

#include "claypaintbrushtool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static gboolean   clay_paintbrush_tool_is_alpha_only (ClayPaintTool *paint_tool,
                                                      ClayDrawable  *drawable);


G_DEFINE_TYPE (ClayPaintbrushTool, clay_paintbrush_tool, CLAY_TYPE_BRUSH_TOOL)


void
clay_paintbrush_tool_register (ClayToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (CLAY_TYPE_PAINTBRUSH_TOOL,
                CLAY_TYPE_PAINT_OPTIONS,
                clay_paint_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_EXPAND   |
                CLAY_CONTEXT_PROP_MASK_GRADIENT |
                CLAY_CONTEXT_PROP_MASK_PATTERN,
                "clay-paintbrush-tool",
                _("Paintbrush"),
                _("Paintbrush Tool: Paint smooth strokes using a brush"),
                N_("_Paintbrush"), "P",
                NULL, CLAY_HELP_TOOL_PAINTBRUSH,
                CLAY_ICON_TOOL_PAINTBRUSH,
                data);
}

static void
clay_paintbrush_tool_class_init (ClayPaintbrushToolClass *klass)
{
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  paint_tool_class->is_alpha_only = clay_paintbrush_tool_is_alpha_only;
}

static void
clay_paintbrush_tool_init (ClayPaintbrushTool *paintbrush)
{
  ClayTool *tool = CLAY_TOOL (paintbrush);

  clay_tool_control_set_tool_cursor (tool->control,
                                     CLAY_TOOL_CURSOR_PAINTBRUSH);

  clay_paint_tool_enable_color_picker (CLAY_PAINT_TOOL (paintbrush),
                                       CLAY_COLOR_PICK_TARGET_FOREGROUND);
}

static gboolean
clay_paintbrush_tool_is_alpha_only (ClayPaintTool *paint_tool,
                                    ClayDrawable  *drawable)
{
  ClayPaintOptions *paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  ClayContext      *context       = CLAY_CONTEXT (paint_options);
  ClayLayerMode     paint_mode    = clay_context_get_paint_mode (context);

  return clay_layer_mode_is_alpha_only (paint_mode);
}
