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

#include "core/claydrawable.h"

#include "paint/clayeraseroptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "clayerasertool.h"
#include "claypaintoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void        clay_eraser_tool_modifier_key  (ClayTool         *tool,
                                                   GdkModifierType   key,
                                                   gboolean          press,
                                                   GdkModifierType   state,
                                                   ClayDisplay      *display);
static void        clay_eraser_tool_cursor_update (ClayTool         *tool,
                                                   const ClayCoords *coords,
                                                   GdkModifierType   state,
                                                   ClayDisplay      *display);

static gboolean    clay_eraser_tool_is_alpha_only (ClayPaintTool    *paint_tool,
                                                   ClayDrawable     *drawable);

static GtkWidget * clay_eraser_options_gui        (ClayToolOptions  *tool_options);


G_DEFINE_TYPE (ClayEraserTool, clay_eraser_tool, CLAY_TYPE_BRUSH_TOOL)

#define parent_class clay_eraser_tool_parent_class


void
clay_eraser_tool_register (ClayToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (CLAY_TYPE_ERASER_TOOL,
                CLAY_TYPE_ERASER_OPTIONS,
                clay_eraser_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK,
                "clay-eraser-tool",
                _("Eraser"),
                _("Eraser Tool: Erase to background or transparency using a brush"),
                N_("_Eraser"), "<shift>E",
                NULL, CLAY_HELP_TOOL_ERASER,
                CLAY_ICON_TOOL_ERASER,
                data);
}

static void
clay_eraser_tool_class_init (ClayEraserToolClass *klass)
{
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  tool_class->modifier_key        = clay_eraser_tool_modifier_key;
  tool_class->cursor_update       = clay_eraser_tool_cursor_update;

  paint_tool_class->is_alpha_only = clay_eraser_tool_is_alpha_only;
}

static void
clay_eraser_tool_init (ClayEraserTool *eraser)
{
  ClayTool      *tool       = CLAY_TOOL (eraser);
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (eraser);

  clay_tool_control_set_tool_cursor            (tool->control,
                                                CLAY_TOOL_CURSOR_ERASER);
  clay_tool_control_set_toggle_cursor_modifier (tool->control,
                                                CLAY_CURSOR_MODIFIER_MINUS);

  clay_paint_tool_enable_color_picker (paint_tool,
                                       CLAY_COLOR_PICK_TARGET_BACKGROUND);

  paint_tool->status      = _("Click to erase");
  paint_tool->status_line = _("Click to erase the line");
  paint_tool->status_ctrl = _("%s to pick a background color");
}

static void
clay_eraser_tool_modifier_key (ClayTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               ClayDisplay     *display)
{
  if (key == GDK_MOD1_MASK)
    {
      ClayEraserOptions *options = CLAY_ERASER_TOOL_GET_OPTIONS (tool);

      g_object_set (options,
                    "anti-erase", ! options->anti_erase,
                    NULL);
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state, display);
}

static void
clay_eraser_tool_cursor_update (ClayTool         *tool,
                                const ClayCoords *coords,
                                GdkModifierType   state,
                                ClayDisplay      *display)
{
  ClayEraserOptions *options = CLAY_ERASER_TOOL_GET_OPTIONS (tool);

  clay_tool_control_set_toggled (tool->control, options->anti_erase);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
clay_eraser_tool_is_alpha_only (ClayPaintTool *paint_tool,
                                ClayDrawable  *drawable)
{
  ClayEraserOptions *options = CLAY_ERASER_TOOL_GET_OPTIONS (paint_tool);

  if (! options->anti_erase)
    return clay_drawable_has_alpha (drawable);
  else
    return TRUE;
}


/*  tool options stuff  */

static GtkWidget *
clay_eraser_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_paint_options_gui (tool_options);
  GtkWidget *button;
  gchar     *str;

  /* the anti_erase toggle */
  str = g_strdup_printf (_("Anti erase  (%s)"),
                         clay_get_mod_string (GDK_MOD1_MASK));

  button = clay_prop_check_button_new (config, "anti-erase", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_free (str);

  return vbox;
}
