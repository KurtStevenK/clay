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

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "paint/claymybrushoptions.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claycanvasarc.h"

#include "core/clay.h"

#include "widgets/clayhelp-ids.h"

#include "claymybrushoptions-gui.h"
#include "claymybrushtool.h"
#include "claytoolcontrol.h"
#include "core/claymybrush.h"

#include "clay-intl.h"

G_DEFINE_TYPE (ClayMybrushTool, clay_mybrush_tool, CLAY_TYPE_PAINT_TOOL)

#define parent_class clay_mybrush_tool_parent_class

static void   clay_mybrush_tool_cursor_update         (ClayTool         *tool,
                                                       const ClayCoords *coords,
                                                       GdkModifierType   state,
                                                       ClayDisplay      *display);
static void   clay_mybrush_tool_options_notify        (ClayTool         *tool,
                                                       ClayToolOptions  *options,
                                                       const GParamSpec *pspec);

static ClayCanvasItem * clay_mybrush_tool_get_outline (ClayPaintTool *paint_tool,
                                                       ClayDisplay   *display,
                                                       gdouble        x,
                                                       gdouble        y);


void
clay_mybrush_tool_register (ClayToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (CLAY_TYPE_MYBRUSH_TOOL,
                CLAY_TYPE_MYBRUSH_OPTIONS,
                clay_mybrush_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                CLAY_CONTEXT_PROP_MASK_OPACITY    |
                CLAY_CONTEXT_PROP_MASK_PAINT_MODE |
                CLAY_CONTEXT_PROP_MASK_MYBRUSH    |
                CLAY_CONTEXT_PROP_MASK_PATTERN    |
                CLAY_CONTEXT_PROP_MASK_EXPAND,
                "clay-mypaint-brush-tool",
                _("MyPaint Brush"),
                _("MyPaint Brush Tool: Use MyPaint brushes in Clay"),
                N_("M_yPaint Brush"), "Y",
                NULL, CLAY_HELP_TOOL_MYPAINT_BRUSH,
                CLAY_ICON_TOOL_MYPAINT_BRUSH,
                data);
}

static void
clay_mybrush_tool_class_init (ClayMybrushToolClass *klass)
{
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  tool_class->cursor_update     = clay_mybrush_tool_cursor_update;
  tool_class->options_notify    = clay_mybrush_tool_options_notify;

  paint_tool_class->get_outline = clay_mybrush_tool_get_outline;
}

static void
clay_mybrush_tool_init (ClayMybrushTool *mybrush_tool)
{
  ClayTool *tool = CLAY_TOOL (mybrush_tool);

  clay_tool_control_set_tool_cursor       (tool->control, CLAY_TOOL_CURSOR_INK);
  clay_tool_control_set_action_size       (tool->control,
                                           "tools-mypaint-brush-radius-set");
  clay_tool_control_set_action_pixel_size (tool->control,
                                           "tools-mypaint-brush-pixel-size-set");
  clay_tool_control_set_action_hardness   (tool->control,
                                           "tools-mypaint-brush-hardness-set");

  clay_paint_tool_enable_color_picker (CLAY_PAINT_TOOL (mybrush_tool),
                                       CLAY_COLOR_PICK_TARGET_FOREGROUND);
}

static void
clay_mybrush_tool_cursor_update (ClayTool         *tool,
                                 const ClayCoords *coords,
                                 GdkModifierType   state,
                                 ClayDisplay      *display)
{
  ClayMybrushOptions *options = CLAY_MYBRUSH_TOOL_GET_OPTIONS (tool);

  if (display && CLAY_IS_DISPLAY (display))
    {
      ClayDisplayShell *shell = clay_display_get_shell (display);

      options->view_zoom     = (clay_zoom_model_get_factor (shell->zoom) > 0.0f) ?
                                clay_zoom_model_get_factor (shell->zoom) : 1.0f;
      options->view_rotation = shell->rotate_angle;
    }
  else
    {
      options->view_zoom     = 1.0f;
      options->view_rotation = 0.0f;
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_mybrush_tool_options_notify (ClayTool         *tool,
                                  ClayToolOptions  *options,
                                  const GParamSpec *pspec)
{
  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "radius"))
    {
      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));
      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

static ClayCanvasItem *
clay_mybrush_tool_get_outline (ClayPaintTool *paint_tool,
                               ClayDisplay   *display,
                               gdouble        x,
                               gdouble        y)
{
  ClayMybrushOptions *options = CLAY_MYBRUSH_TOOL_GET_OPTIONS (paint_tool);
  ClayCanvasItem     *item    = NULL;
  ClayDisplayShell   *shell   = clay_display_get_shell (display);
  gdouble             radius;

  /* Radius size as used by libmypaint is logarithmic.
   * The drawn outline in the MyBrush tool is more of a general idea of
   * the brush size. With each brush having its own logic, actual drawn
   * dabs may be quite different, smaller but also bigger. And there are
   * some random elements in there too.
   * See also mypaint-brush.c code in libmypaint.
   */
  radius = exp (options->radius);
  radius = MAX (MAX (4 / shell->scale_x, 4 / shell->scale_y), radius);

  item = clay_mybrush_tool_create_cursor (paint_tool, display, x, y, radius);

  if (! item)
    {
      clay_paint_tool_set_draw_fallback (paint_tool,
                                         TRUE, radius);
    }

  return item;
}

ClayCanvasItem *
clay_mybrush_tool_create_cursor (ClayPaintTool *paint_tool,
                                 ClayDisplay   *display,
                                 gdouble        x,
                                 gdouble        y,
                                 gdouble        radius)
{

  ClayDisplayShell     *shell;

  g_return_val_if_fail (CLAY_IS_PAINT_TOOL (paint_tool), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  shell = clay_display_get_shell (display);

  /*  don't draw the boundary if it becomes too small  */
  if (SCALEX (shell, radius) > 4 &&
      SCALEY (shell, radius) > 4)
    {
      return clay_canvas_arc_new(shell, x, y, radius, radius, 0.0, 2 * G_PI, FALSE);
    }

  return NULL;
}
