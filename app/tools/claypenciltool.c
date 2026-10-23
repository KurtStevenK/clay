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

#include "paint/claypenciloptions.h"

#include "widgets/clayhelp-ids.h"

#include "claypenciltool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


G_DEFINE_TYPE (ClayPencilTool, clay_pencil_tool, CLAY_TYPE_PAINTBRUSH_TOOL)


void
clay_pencil_tool_register (ClayToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (CLAY_TYPE_PENCIL_TOOL,
                CLAY_TYPE_PENCIL_OPTIONS,
                clay_paint_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_EXPAND   |
                CLAY_CONTEXT_PROP_MASK_PATTERN  |
                CLAY_CONTEXT_PROP_MASK_GRADIENT,
                "clay-pencil-tool",
                _("Pencil"),
                _("Pencil Tool: Hard edge painting using a brush"),
                N_("Pe_ncil"), "N",
                NULL, CLAY_HELP_TOOL_PENCIL,
                CLAY_ICON_TOOL_PENCIL,
                data);
}

static void
clay_pencil_tool_class_init (ClayPencilToolClass *klass)
{
}

static void
clay_pencil_tool_init (ClayPencilTool *pencil)
{
  ClayTool *tool = CLAY_TOOL (pencil);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_PENCIL);
}
