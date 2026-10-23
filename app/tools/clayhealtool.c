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

#include "paint/claysourceoptions.h"

#include "widgets/clayhelp-ids.h"

#include "clayhealtool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static GtkWidget * clay_heal_options_gui (ClayToolOptions *tool_options);


G_DEFINE_TYPE (ClayHealTool, clay_heal_tool, CLAY_TYPE_SOURCE_TOOL)


void
clay_heal_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_HEAL_TOOL,
                CLAY_TYPE_SOURCE_OPTIONS,
                clay_heal_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_PATTERN  |
                CLAY_CONTEXT_PROP_MASK_EXPAND,
                "clay-heal-tool",
                _("Heal"),
                _("Healing Tool: Heal image irregularities"),
                N_("_Heal"),
                "H",
                NULL,
                CLAY_HELP_TOOL_HEAL,
                CLAY_ICON_TOOL_HEAL,
                data);
}

static void
clay_heal_tool_class_init (ClayHealToolClass *klass)
{
}

static void
clay_heal_tool_init (ClayHealTool *heal)
{
  ClayTool       *tool        = CLAY_TOOL (heal);
  ClayPaintTool  *paint_tool  = CLAY_PAINT_TOOL (tool);
  ClaySourceTool *source_tool = CLAY_SOURCE_TOOL (tool);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_HEAL);

  paint_tool->status      = _("Click to heal");
  paint_tool->status_ctrl = _("%s to set a new heal source");

  source_tool->status_paint           = _("Click to heal");
  source_tool->status_set_source      = _("Click to set a new heal source");
  source_tool->status_set_source_ctrl = _("%s to set a new heal source");
  source_tool->status_set_source_ctrl_click = _("%s-click to set a new heal source");
}


/*  tool options stuff  */

static GtkWidget *
clay_heal_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *combo;

  /* the sample merged checkbox */
  button = clay_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* the alignment combo */
  combo = clay_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo), _("Alignment"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  return vbox;
}
