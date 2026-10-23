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

#include "paint/clayinkoptions.h"

#include "widgets/clayhelp-ids.h"

#include "clayinkoptions-gui.h"
#include "clayinktool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static ClayCanvasItem * clay_ink_tool_get_outline    (ClayPaintTool    *paint_tool,
                                                      ClayDisplay      *display,
                                                      gdouble           x,
                                                      gdouble           y);
static gboolean         clay_ink_tool_is_alpha_only  (ClayPaintTool    *paint_tool,
                                                      ClayDrawable     *drawable);

static void             clay_ink_tool_options_notify (ClayTool         *tool,
                                                      ClayToolOptions  *options,
                                                      const GParamSpec *pspec);


G_DEFINE_TYPE (ClayInkTool, clay_ink_tool, CLAY_TYPE_PAINT_TOOL)

#define parent_class clay_ink_tool_parent_class


void
clay_ink_tool_register (ClayToolRegisterCallback  callback,
                        gpointer                  data)
{
  (* callback) (CLAY_TYPE_INK_TOOL,
                CLAY_TYPE_INK_OPTIONS,
                clay_ink_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                CLAY_CONTEXT_PROP_MASK_OPACITY    |
                CLAY_CONTEXT_PROP_MASK_PAINT_MODE |
                CLAY_CONTEXT_PROP_MASK_PATTERN    |
                CLAY_CONTEXT_PROP_MASK_EXPAND,
                "clay-ink-tool",
                _("Ink"),
                _("Ink Tool: Calligraphy-style painting"),
                N_("In_k"), "K",
                NULL, CLAY_HELP_TOOL_INK,
                CLAY_ICON_TOOL_INK,
                data);
}

static void
clay_ink_tool_class_init (ClayInkToolClass *klass)
{
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  tool_class->options_notify    = clay_ink_tool_options_notify;

  paint_tool_class->get_outline   = clay_ink_tool_get_outline;
  paint_tool_class->is_alpha_only = clay_ink_tool_is_alpha_only;
}

static void
clay_ink_tool_init (ClayInkTool *ink_tool)
{
  ClayTool *tool = CLAY_TOOL (ink_tool);

  clay_tool_control_set_tool_cursor   (tool->control, CLAY_TOOL_CURSOR_INK);
  clay_tool_control_set_action_pixel_size (tool->control,
                                           "tools-ink-blob-pixel-size-set");
  clay_tool_control_set_action_size       (tool->control,
                                           "tools-ink-blob-size-set");
  clay_tool_control_set_action_aspect     (tool->control,
                                           "tools-ink-blob-aspect-set");
  clay_tool_control_set_action_angle      (tool->control,
                                           "tools-ink-blob-angle-set");

  clay_paint_tool_enable_color_picker (CLAY_PAINT_TOOL (ink_tool),
                                       CLAY_COLOR_PICK_TARGET_FOREGROUND);
}

static void
clay_ink_tool_options_notify (ClayTool         *tool,
                              ClayToolOptions  *options,
                              const GParamSpec *pspec)
{
  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (g_strcmp0 (pspec->name, "size") == 0 &&
      CLAY_PAINT_TOOL (tool)->draw_brush)
    {
      /* This triggers a redraw of the tool pointer, especially useful
       * here when we change the pen size with on-canvas interaction.
       */
      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));
      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

static ClayCanvasItem *
clay_ink_tool_get_outline (ClayPaintTool *paint_tool,
                           ClayDisplay   *display,
                           gdouble        x,
                           gdouble        y)
{
  ClayInkOptions *options = CLAY_INK_TOOL_GET_OPTIONS (paint_tool);

  clay_paint_tool_set_draw_circle (paint_tool, TRUE,
                                   options->size);

  return NULL;
}

static gboolean
clay_ink_tool_is_alpha_only (ClayPaintTool *paint_tool,
                             ClayDrawable  *drawable)
{
  ClayPaintOptions *paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  ClayContext      *context       = CLAY_CONTEXT (paint_options);
  ClayLayerMode     paint_mode    = clay_context_get_paint_mode (context);

  return clay_layer_mode_is_alpha_only (paint_mode);
}
