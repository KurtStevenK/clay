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

#include "paint/clayairbrush.h"
#include "paint/clayairbrushoptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"

#include "clayairbrushtool.h"
#include "claypaintoptions-gui.h"
#include "claypainttool-paint.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void        clay_airbrush_tool_constructed    (GObject          *object);

static void        clay_airbrush_tool_airbrush_stamp (ClayAirbrush     *airbrush,
                                                      ClayAirbrushTool *airbrush_tool);

static void        clay_airbrush_tool_stamp          (ClayAirbrushTool *airbrush_tool,
                                                      gpointer          data);

static GtkWidget * clay_airbrush_options_gui         (ClayToolOptions  *tool_options);


G_DEFINE_TYPE (ClayAirbrushTool, clay_airbrush_tool, CLAY_TYPE_PAINTBRUSH_TOOL)

#define parent_class clay_airbrush_tool_parent_class


void
clay_airbrush_tool_register (ClayToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (CLAY_TYPE_AIRBRUSH_TOOL,
                CLAY_TYPE_AIRBRUSH_OPTIONS,
                clay_airbrush_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_EXPAND   |
                CLAY_CONTEXT_PROP_MASK_PATTERN  |
                CLAY_CONTEXT_PROP_MASK_GRADIENT,
                "clay-airbrush-tool",
                _("Airbrush"),
                _("Airbrush Tool: Paint using a brush, with variable pressure"),
                N_("_Airbrush"), "A",
                NULL, CLAY_HELP_TOOL_AIRBRUSH,
                CLAY_ICON_TOOL_AIRBRUSH,
                data);
}

static void
clay_airbrush_tool_class_init (ClayAirbrushToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = clay_airbrush_tool_constructed;
}

static void
clay_airbrush_tool_init (ClayAirbrushTool *airbrush)
{
  ClayTool *tool = CLAY_TOOL (airbrush);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_AIRBRUSH);
}

static void
clay_airbrush_tool_constructed (GObject *object)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (paint_tool->core, "stamp",
                           G_CALLBACK (clay_airbrush_tool_airbrush_stamp),
                           object, 0);
}

static void
clay_airbrush_tool_airbrush_stamp (ClayAirbrush     *airbrush,
                                   ClayAirbrushTool *airbrush_tool)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (airbrush_tool);

  clay_paint_tool_paint_push (
    paint_tool,
    (ClayPaintToolPaintFunc) clay_airbrush_tool_stamp,
    NULL);
}

static void
clay_airbrush_tool_stamp (ClayAirbrushTool *airbrush_tool,
                          gpointer          data)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (airbrush_tool);

  clay_airbrush_stamp (CLAY_AIRBRUSH (paint_tool->core));
}


/*  tool options stuff  */

static GtkWidget *
clay_airbrush_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_paint_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *scale;

  button = clay_prop_check_button_new (config, "motion-only", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  scale = clay_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = clay_prop_spin_scale_new (config, "flow",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
