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

#include "paint/clayconvolveoptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claywidgets-utils.h"

#include "clayconvolvetool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void   clay_convolve_tool_modifier_key  (ClayTool         *tool,
                                                GdkModifierType   key,
                                                gboolean          press,
                                                GdkModifierType   state,
                                                ClayDisplay      *display);
static void   clay_convolve_tool_cursor_update (ClayTool         *tool,
                                                const ClayCoords *coords,
                                                GdkModifierType   state,
                                                ClayDisplay      *display);
static void   clay_convolve_tool_oper_update   (ClayTool         *tool,
                                                const ClayCoords *coords,
                                                GdkModifierType   state,
                                                gboolean          proximity,
                                                ClayDisplay      *display);
static void   clay_convolve_tool_status_update (ClayTool         *tool,
                                                ClayConvolveType  type);

static GtkWidget * clay_convolve_options_gui   (ClayToolOptions  *options);


G_DEFINE_TYPE (ClayConvolveTool, clay_convolve_tool, CLAY_TYPE_BRUSH_TOOL)

#define parent_class clay_convolve_tool_parent_class


void
clay_convolve_tool_register (ClayToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (CLAY_TYPE_CONVOLVE_TOOL,
                CLAY_TYPE_CONVOLVE_OPTIONS,
                clay_convolve_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_PATTERN  |
                CLAY_CONTEXT_PROP_MASK_EXPAND,
                "clay-convolve-tool",
                _("Blur / Sharpen"),
                _("Blur / Sharpen Tool: Selective blurring or unblurring using a brush"),
                N_("Bl_ur / Sharpen"), "<shift>U",
                NULL, CLAY_HELP_TOOL_CONVOLVE,
                CLAY_ICON_TOOL_BLUR,
                data);
}

static void
clay_convolve_tool_class_init (ClayConvolveToolClass *klass)
{
  ClayToolClass *tool_class = CLAY_TOOL_CLASS (klass);

  tool_class->modifier_key  = clay_convolve_tool_modifier_key;
  tool_class->cursor_update = clay_convolve_tool_cursor_update;
  tool_class->oper_update   = clay_convolve_tool_oper_update;
}

static void
clay_convolve_tool_init (ClayConvolveTool *convolve)
{
  ClayTool *tool = CLAY_TOOL (convolve);

  clay_tool_control_set_tool_cursor            (tool->control,
                                                CLAY_TOOL_CURSOR_BLUR);
  clay_tool_control_set_toggle_cursor_modifier (tool->control,
                                                CLAY_CURSOR_MODIFIER_MINUS);

  clay_convolve_tool_status_update (tool, CLAY_CONVOLVE_BLUR);
}

static void
clay_convolve_tool_modifier_key (ClayTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state,
                                 ClayDisplay     *display)
{
  ClayConvolveTool    *convolve    = CLAY_CONVOLVE_TOOL (tool);
  ClayConvolveOptions *options     = CLAY_CONVOLVE_TOOL_GET_OPTIONS (tool);
  GdkModifierType      line_mask   = CLAY_PAINT_TOOL_LINE_MASK;
  GdkModifierType      toggle_mask = clay_get_toggle_behavior_mask ();

  if (((key == toggle_mask)  &&
       ! (state & line_mask) && /* leave stuff untouched in line draw mode */
       press != convolve->toggled)

      ||

      (key == line_mask  && /* toggle back after keypresses CTRL(hold)->  */
       ! press           && /* SHIFT(hold)->CTRL(release)->SHIFT(release) */
       convolve->toggled &&
       ! (state & toggle_mask)))
    {
      convolve->toggled = press;

      switch (options->type)
        {
        case CLAY_CONVOLVE_BLUR:
          g_object_set (options, "type", CLAY_CONVOLVE_SHARPEN, NULL);
          break;

        case CLAY_CONVOLVE_SHARPEN:
          g_object_set (options, "type", CLAY_CONVOLVE_BLUR, NULL);
          break;
        }
    }
}

static void
clay_convolve_tool_cursor_update (ClayTool         *tool,
                                  const ClayCoords *coords,
                                  GdkModifierType   state,
                                  ClayDisplay      *display)
{
  ClayConvolveOptions *options = CLAY_CONVOLVE_TOOL_GET_OPTIONS (tool);

  clay_tool_control_set_toggled (tool->control,
                                 options->type == CLAY_CONVOLVE_SHARPEN);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_convolve_tool_oper_update (ClayTool         *tool,
                                const ClayCoords *coords,
                                GdkModifierType   state,
                                gboolean          proximity,
                                ClayDisplay      *display)
{
  ClayConvolveOptions *options = CLAY_CONVOLVE_TOOL_GET_OPTIONS (tool);

  clay_convolve_tool_status_update (tool, options->type);

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
clay_convolve_tool_status_update (ClayTool         *tool,
                                  ClayConvolveType  type)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);

  switch (type)
    {
    case CLAY_CONVOLVE_BLUR:
      paint_tool->status      = _("Click to blur");
      paint_tool->status_line = _("Click to blur the line");
      paint_tool->status_ctrl = _("%s to sharpen");
      break;

    case CLAY_CONVOLVE_SHARPEN:
      paint_tool->status      = _("Click to sharpen");
      paint_tool->status_line = _("Click to sharpen the line");
      paint_tool->status_ctrl = _("%s to blur");
      break;

    default:
      break;
    }
}


/*  tool options stuff  */

static GtkWidget *
clay_convolve_options_gui (ClayToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = clay_paint_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *scale;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = clay_get_toggle_behavior_mask ();

  /*  the type radio box  */
  str = g_strdup_printf (_("Convolve Type  (%s)"),
                         clay_get_mod_string (toggle_mask));

  frame = clay_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  g_free (str);

  /*  the rate scale  */
  scale = clay_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
