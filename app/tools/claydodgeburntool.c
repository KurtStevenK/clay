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

#include "paint/claydodgeburnoptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claywidgets-utils.h"

#include "claydodgeburntool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void   clay_dodge_burn_tool_modifier_key  (ClayTool          *tool,
                                                  GdkModifierType    key,
                                                  gboolean           press,
                                                  GdkModifierType    state,
                                                  ClayDisplay       *display);
static void   clay_dodge_burn_tool_cursor_update (ClayTool          *tool,
                                                  const ClayCoords  *coords,
                                                  GdkModifierType    state,
                                                  ClayDisplay       *display);
static void   clay_dodge_burn_tool_oper_update   (ClayTool          *tool,
                                                  const ClayCoords  *coords,
                                                  GdkModifierType    state,
                                                  gboolean           proximity,
                                                  ClayDisplay       *display);
static void   clay_dodge_burn_tool_status_update (ClayTool          *tool,
                                                  ClayDodgeBurnType  type);

static GtkWidget * clay_dodge_burn_options_gui   (ClayToolOptions   *tool_options);


G_DEFINE_TYPE (ClayDodgeBurnTool, clay_dodge_burn_tool, CLAY_TYPE_BRUSH_TOOL)

#define parent_class clay_dodge_burn_tool_parent_class


void
clay_dodge_burn_tool_register (ClayToolRegisterCallback  callback,
                               gpointer                  data)
{
  (* callback) (CLAY_TYPE_DODGE_BURN_TOOL,
                CLAY_TYPE_DODGE_BURN_OPTIONS,
                clay_dodge_burn_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK,
                "clay-dodge-burn-tool",
                _("Dodge / Burn"),
                _("Dodge / Burn Tool: Selectively lighten or darken using a brush"),
                N_("Dod_ge / Burn"), "<shift>D",
                NULL, CLAY_HELP_TOOL_DODGE_BURN,
                CLAY_ICON_TOOL_DODGE,
                data);
}

static void
clay_dodge_burn_tool_class_init (ClayDodgeBurnToolClass *klass)
{
  ClayToolClass *tool_class = CLAY_TOOL_CLASS (klass);

  tool_class->modifier_key  = clay_dodge_burn_tool_modifier_key;
  tool_class->cursor_update = clay_dodge_burn_tool_cursor_update;
  tool_class->oper_update   = clay_dodge_burn_tool_oper_update;
}

static void
clay_dodge_burn_tool_init (ClayDodgeBurnTool *dodgeburn)
{
  ClayTool *tool = CLAY_TOOL (dodgeburn);

  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_DODGE);
  clay_tool_control_set_toggle_tool_cursor (tool->control,
                                            CLAY_TOOL_CURSOR_BURN);

  clay_dodge_burn_tool_status_update (tool, CLAY_DODGE_BURN_TYPE_BURN);
}

static void
clay_dodge_burn_tool_modifier_key (ClayTool        *tool,
                                   GdkModifierType  key,
                                   gboolean         press,
                                   GdkModifierType  state,
                                   ClayDisplay     *display)
{
  ClayDodgeBurnTool    *dodgeburn   = CLAY_DODGE_BURN_TOOL (tool);
  ClayDodgeBurnOptions *options     = CLAY_DODGE_BURN_TOOL_GET_OPTIONS (tool);
  GdkModifierType       line_mask   = CLAY_PAINT_TOOL_LINE_MASK;
  GdkModifierType       toggle_mask = clay_get_toggle_behavior_mask ();

  if ((key == toggle_mask   &&
      ! (state & line_mask) && /* leave stuff untouched in line draw mode */
       press != dodgeburn->toggled)

      ||

      (key == line_mask   && /* toggle back after keypresses CTRL(hold)->  */
       ! press            && /* SHIFT(hold)->CTRL(release)->SHIFT(release) */
       dodgeburn->toggled &&
       ! (state & toggle_mask)))
    {
      dodgeburn->toggled = press;

      switch (options->type)
        {
        case CLAY_DODGE_BURN_TYPE_DODGE:
          g_object_set (options, "type", CLAY_DODGE_BURN_TYPE_BURN, NULL);
          break;

        case CLAY_DODGE_BURN_TYPE_BURN:
          g_object_set (options, "type", CLAY_DODGE_BURN_TYPE_DODGE, NULL);
          break;

        default:
          break;
        }
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
clay_dodge_burn_tool_cursor_update (ClayTool         *tool,
                                    const ClayCoords *coords,
                                    GdkModifierType   state,
                                    ClayDisplay      *display)
{
  ClayDodgeBurnOptions *options = CLAY_DODGE_BURN_TOOL_GET_OPTIONS (tool);

  clay_tool_control_set_toggled (tool->control,
                                 options->type == CLAY_DODGE_BURN_TYPE_BURN);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
clay_dodge_burn_tool_oper_update (ClayTool         *tool,
                                  const ClayCoords *coords,
                                  GdkModifierType   state,
                                  gboolean          proximity,
                                  ClayDisplay      *display)
{
  ClayDodgeBurnOptions *options = CLAY_DODGE_BURN_TOOL_GET_OPTIONS (tool);

  clay_dodge_burn_tool_status_update (tool, options->type);

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
clay_dodge_burn_tool_status_update (ClayTool          *tool,
                                    ClayDodgeBurnType  type)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);

  switch (type)
    {
    case CLAY_DODGE_BURN_TYPE_DODGE:
      paint_tool->status      = _("Click to dodge");
      paint_tool->status_line = _("Click to dodge the line");
      paint_tool->status_ctrl = _("%s to burn");
      break;

    case CLAY_DODGE_BURN_TYPE_BURN:
      paint_tool->status      = _("Click to burn");
      paint_tool->status_line = _("Click to burn the line");
      paint_tool->status_ctrl = _("%s to dodge");
      break;

    default:
      break;
    }
}


/*  tool options stuff  */

static GtkWidget *
clay_dodge_burn_options_gui (ClayToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = clay_paint_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *scale;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = clay_get_toggle_behavior_mask ();

  /* the type (dodge or burn) */
  str = g_strdup_printf (_("Type  (%s)"),
                         clay_get_mod_string (toggle_mask));
  frame = clay_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  /*  mode (highlights, midtones, or shadows)  */
  frame = clay_prop_enum_radio_frame_new (config, "mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  /*  the exposure scale  */
  scale = clay_prop_spin_scale_new (config, "exposure",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
