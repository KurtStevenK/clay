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

#include "paint/claysmudgeoptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"

#include "claysmudgetool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static GtkWidget * clay_smudge_options_gui (ClayToolOptions *tool_options);


G_DEFINE_TYPE (ClaySmudgeTool, clay_smudge_tool, CLAY_TYPE_BRUSH_TOOL)


void
clay_smudge_tool_register (ClayToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (CLAY_TYPE_SMUDGE_TOOL,
                CLAY_TYPE_SMUDGE_OPTIONS,
                clay_smudge_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_EXPAND   |
                CLAY_CONTEXT_PROP_MASK_PATTERN  |
                CLAY_CONTEXT_PROP_MASK_GRADIENT,
                "clay-smudge-tool",
                _("Smudge"),
                _("Smudge Tool: Smudge selectively using a brush"),
                N_("_Smudge"), "S",
                NULL, CLAY_HELP_TOOL_SMUDGE,
                CLAY_ICON_TOOL_SMUDGE,
                data);
}

static void
clay_smudge_tool_class_init (ClaySmudgeToolClass *klass)
{
}

static void
clay_smudge_tool_init (ClaySmudgeTool *smudge)
{
  ClayTool      *tool       = CLAY_TOOL (smudge);
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (smudge);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_SMUDGE);

  clay_paint_tool_enable_color_picker (CLAY_PAINT_TOOL (smudge),
                                       CLAY_COLOR_PICK_TARGET_FOREGROUND);

  paint_tool->status      = _("Click to smudge");
  paint_tool->status_line = _("Click to smudge the line");
  paint_tool->status_ctrl = NULL;
}


/*  tool options stuff  */

static GtkWidget *
clay_smudge_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_paint_options_gui (tool_options);
  GtkWidget *scale;
  GtkWidget *button;

  button = clay_prop_check_button_new (config, "no-erasing", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = clay_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  the rate scale  */
  scale = clay_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = clay_prop_spin_scale_new (config, "flow",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
