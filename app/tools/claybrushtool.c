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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaymath/claymath.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"

#include "core/clay.h"
#include "core/claybezierdesc.h"
#include "core/claybrush.h"
#include "core/clayimage.h"
#include "core/claytoolinfo.h"

#include "paint/claybrushcore.h"
#include "paint/claypaintoptions.h"

#include "display/claycanvashandle.h"
#include "display/claycanvaspath.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "claybrushtool.h"
#include "claypainttool-paint.h"
#include "claytoolcontrol.h"


static void   clay_brush_tool_constructed     (GObject           *object);

static void   clay_brush_tool_oper_update     (ClayTool          *tool,
                                               const ClayCoords  *coords,
                                               GdkModifierType    state,
                                               gboolean           proximity,
                                               ClayDisplay       *display);
static void   clay_brush_tool_cursor_update   (ClayTool          *tool,
                                               const ClayCoords  *coords,
                                               GdkModifierType    state,
                                               ClayDisplay       *display);
static void   clay_brush_tool_options_notify  (ClayTool          *tool,
                                               ClayToolOptions   *options,
                                               const GParamSpec  *pspec);

static void   clay_brush_tool_paint_start     (ClayPaintTool     *paint_tool);
static void   clay_brush_tool_paint_end       (ClayPaintTool     *paint_tool);
static void   clay_brush_tool_paint_flush     (ClayPaintTool     *paint_tool);
static ClayCanvasItem *
              clay_brush_tool_get_outline     (ClayPaintTool     *paint_tool,
                                               ClayDisplay       *display,
                                               gdouble            x,
                                               gdouble            y);

static void   clay_brush_tool_brush_changed   (ClayContext       *context,
                                               ClayBrush         *brush,
                                               ClayBrushTool     *brush_tool);
static void   clay_brush_tool_set_brush       (ClayBrushCore     *brush_core,
                                               ClayBrush         *brush,
                                               ClayBrushTool     *brush_tool);

static const ClayBezierDesc *
                 clay_brush_tool_get_boundary (ClayBrushTool     *brush_tool,
                                               gint              *width,
                                               gint              *height);


G_DEFINE_TYPE (ClayBrushTool, clay_brush_tool, CLAY_TYPE_PAINT_TOOL)

#define parent_class clay_brush_tool_parent_class


static void
clay_brush_tool_class_init (ClayBrushToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  object_class->constructed     = clay_brush_tool_constructed;

  tool_class->oper_update       = clay_brush_tool_oper_update;
  tool_class->cursor_update     = clay_brush_tool_cursor_update;
  tool_class->options_notify    = clay_brush_tool_options_notify;

  paint_tool_class->paint_start = clay_brush_tool_paint_start;
  paint_tool_class->paint_end   = clay_brush_tool_paint_end;
  paint_tool_class->paint_flush = clay_brush_tool_paint_flush;
  paint_tool_class->get_outline = clay_brush_tool_get_outline;
}

static void
clay_brush_tool_init (ClayBrushTool *brush_tool)
{
  ClayTool *tool = CLAY_TOOL (brush_tool);

  clay_tool_control_set_action_pixel_size (tool->control,
                                           "tools-paintbrush-pixel-size-set");
  clay_tool_control_set_action_size       (tool->control,
                                           "tools-paintbrush-size-set");
  clay_tool_control_set_action_aspect     (tool->control,
                                           "tools-paintbrush-aspect-ratio-set");
  clay_tool_control_set_action_angle      (tool->control,
                                           "tools-paintbrush-angle-set");
  clay_tool_control_set_action_spacing    (tool->control,
                                           "tools-paintbrush-spacing-set");
  clay_tool_control_set_action_hardness   (tool->control,
                                           "tools-paintbrush-hardness-set");
  clay_tool_control_set_action_force      (tool->control,
                                           "tools-paintbrush-force-set");
  clay_tool_control_set_action_object_1   (tool->control,
                                           "context-brush-select-set");
}

static void
clay_brush_tool_constructed (GObject *object)
{
  ClayTool      *tool       = CLAY_TOOL (object);
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_BRUSH_CORE (paint_tool->core));

  g_signal_connect_object (clay_tool_get_options (tool), "brush-changed",
                           G_CALLBACK (clay_brush_tool_brush_changed),
                           paint_tool, 0);

  g_signal_connect_object (paint_tool->core, "set-brush",
                           G_CALLBACK (clay_brush_tool_set_brush),
                           paint_tool, 0);
}

static void
clay_brush_tool_oper_update (ClayTool         *tool,
                             const ClayCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             ClayDisplay      *display)
{
  ClayPaintOptions *paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (tool);
  ClayImage        *image         = clay_display_get_image (display);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                               proximity, display);

  if (! clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)) &&
      image && proximity)
    {
      ClayContext   *context    = CLAY_CONTEXT (paint_options);
      ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);
      ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paint_tool->core);

      clay_brush_core_set_brush (brush_core,
                                 clay_context_get_brush (context));

      clay_brush_core_set_dynamics (brush_core,
                                    clay_context_get_dynamics (context));

      if (CLAY_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
        {
          clay_brush_core_eval_transform_dynamics (brush_core,
                                                   image,
                                                   paint_options,
                                                   coords);
        }
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_brush_tool_cursor_update (ClayTool         *tool,
                               const ClayCoords *coords,
                               GdkModifierType   state,
                               ClayDisplay      *display)
{
  ClayBrushTool *brush_tool = CLAY_BRUSH_TOOL (tool);
  ClayBrushCore *brush_core = CLAY_BRUSH_CORE (CLAY_PAINT_TOOL (brush_tool)->core);

  if (! clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      if (! brush_core->main_brush || ! brush_core->dynamics)
        {
          clay_tool_set_cursor (tool, display,
                                clay_tool_control_get_cursor (tool->control),
                                clay_tool_control_get_tool_cursor (tool->control),
                                CLAY_CURSOR_MODIFIER_BAD);
          return;
        }
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool,  coords, state, display);
}

static void
clay_brush_tool_options_notify (ClayTool         *tool,
                                ClayToolOptions  *options,
                                const GParamSpec *pspec)
{
  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "brush-size")  ||
      ! strcmp (pspec->name, "brush-angle") ||
      ! strcmp (pspec->name, "brush-aspect-ratio"))
    {
      ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);
      ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paint_tool->core);

      g_signal_emit_by_name (brush_core, "set-brush",
                             brush_core->main_brush);
    }
}

static void
clay_brush_tool_paint_start (ClayPaintTool *paint_tool)
{
  ClayBrushTool        *brush_tool = CLAY_BRUSH_TOOL (paint_tool);
  ClayBrushCore        *brush_core = CLAY_BRUSH_CORE (paint_tool->core);
  const ClayBezierDesc *boundary;

  if (CLAY_PAINT_TOOL_CLASS (parent_class)->paint_start)
    CLAY_PAINT_TOOL_CLASS (parent_class)->paint_start (paint_tool);

  boundary = clay_brush_tool_get_boundary (brush_tool,
                                           &brush_tool->boundary_width,
                                           &brush_tool->boundary_height);

  if (boundary)
    brush_tool->boundary = clay_bezier_desc_copy (boundary);

  brush_tool->boundary_scale        = brush_core->scale;
  brush_tool->boundary_aspect_ratio = brush_core->aspect_ratio;
  brush_tool->boundary_angle        = brush_core->angle;
  brush_tool->boundary_reflect      = brush_core->reflect;
  brush_tool->boundary_hardness     = brush_core->hardness;
}

static void
clay_brush_tool_paint_end (ClayPaintTool *paint_tool)
{
  ClayBrushTool *brush_tool = CLAY_BRUSH_TOOL (paint_tool);

  g_clear_pointer (&brush_tool->boundary, clay_bezier_desc_free);

  if (CLAY_PAINT_TOOL_CLASS (parent_class)->paint_end)
    CLAY_PAINT_TOOL_CLASS (parent_class)->paint_end (paint_tool);
}

static void
clay_brush_tool_paint_flush (ClayPaintTool *paint_tool)
{
  ClayBrushTool        *brush_tool = CLAY_BRUSH_TOOL (paint_tool);
  ClayBrushCore        *brush_core = CLAY_BRUSH_CORE (paint_tool->core);
  const ClayBezierDesc *boundary;

  if (CLAY_PAINT_TOOL_CLASS (parent_class)->paint_flush)
    CLAY_PAINT_TOOL_CLASS (parent_class)->paint_flush (paint_tool);

  if (brush_tool->boundary_scale        != brush_core->scale        ||
      brush_tool->boundary_aspect_ratio != brush_core->aspect_ratio ||
      brush_tool->boundary_angle        != brush_core->angle        ||
      brush_tool->boundary_reflect      != brush_core->reflect      ||
      brush_tool->boundary_hardness     != brush_core->hardness)
    {
      g_clear_pointer (&brush_tool->boundary, clay_bezier_desc_free);

      boundary = clay_brush_tool_get_boundary (brush_tool,
                                               &brush_tool->boundary_width,
                                               &brush_tool->boundary_height);

      if (boundary)
        brush_tool->boundary = clay_bezier_desc_copy (boundary);

      brush_tool->boundary_scale        = brush_core->scale;
      brush_tool->boundary_aspect_ratio = brush_core->aspect_ratio;
      brush_tool->boundary_angle        = brush_core->angle;
      brush_tool->boundary_reflect      = brush_core->reflect;
      brush_tool->boundary_hardness     = brush_core->hardness;
    }
}

static ClayCanvasItem *
clay_brush_tool_get_outline (ClayPaintTool *paint_tool,
                             ClayDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  ClayBrushTool  *brush_tool = CLAY_BRUSH_TOOL (paint_tool);
  ClayCanvasItem *item;

  item = clay_brush_tool_create_outline (brush_tool, display, x, y);

  if (! item)
    {
      ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paint_tool->core);

      if (brush_core->main_brush && brush_core->dynamics)
        {
          /*  if an outline was expected, but got scaled away by
           *  transform/dynamics, draw a circle in the "normal" size.
           */
          ClayPaintOptions *options;

          options = CLAY_PAINT_TOOL_GET_OPTIONS (brush_tool);

          clay_paint_tool_set_draw_fallback (paint_tool,
                                             TRUE, options->brush_size);
        }
    }

  return item;
}

ClayCanvasItem *
clay_brush_tool_create_outline (ClayBrushTool *brush_tool,
                                ClayDisplay   *display,
                                gdouble        x,
                                gdouble        y)
{
  ClayTool             *tool;
  ClayDisplayShell     *shell;
  const ClayBezierDesc *boundary = NULL;
  gint                  width    = 0;
  gint                  height   = 0;

  g_return_val_if_fail (CLAY_IS_BRUSH_TOOL (brush_tool), NULL);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  if (clay_paint_tool_paint_is_active (CLAY_PAINT_TOOL (brush_tool)))
    {
      boundary = brush_tool->boundary;
      width    = brush_tool->boundary_width;
      height   = brush_tool->boundary_height;
    }
  else
    {
      boundary = clay_brush_tool_get_boundary (brush_tool, &width, &height);
    }

  if (! boundary)
    return NULL;

  tool  = CLAY_TOOL (brush_tool);
  shell = clay_display_get_shell (display);

  /*  don't draw the boundary if it becomes too small  */
  if (SCALEX (shell, width)  > 4 &&
      SCALEY (shell, height) > 4)
    {
      x -= width  / 2.0;
      y -= height / 2.0;

      if (clay_tool_control_get_precision (tool->control) ==
          CLAY_CURSOR_PRECISION_PIXEL_CENTER)
        {
#define EPSILON 0.000001
          /*  Add EPSILON before rounding since e.g.
           *  (5.0 - 0.5) may end up at (4.499999999....)
           *  due to floating point fnords
           */
          x = RINT (x + EPSILON);
          y = RINT (y + EPSILON);
#undef EPSILON
        }

      return clay_canvas_path_new (shell, boundary, x, y, FALSE,
                                   CLAY_PATH_STYLE_OUTLINE);
    }

  return NULL;
}

static void
clay_brush_tool_brush_changed (ClayContext   *context,
                               ClayBrush     *brush,
                               ClayBrushTool *brush_tool)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (brush_tool);
  ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paint_tool->core);

  clay_brush_core_set_brush (brush_core, brush);

}

static void
clay_brush_tool_set_brush (ClayBrushCore *brush_core,
                           ClayBrush     *brush,
                           ClayBrushTool *brush_tool)
{
  clay_draw_tool_pause (CLAY_DRAW_TOOL (brush_tool));

  if (CLAY_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
    {
      ClayPaintCore *paint_core = CLAY_PAINT_CORE (brush_core);

      clay_brush_core_eval_transform_dynamics (brush_core,
                                               NULL,
                                               CLAY_PAINT_TOOL_GET_OPTIONS (brush_tool),
                                               &paint_core->cur_coords);
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (brush_tool));
}

static const ClayBezierDesc *
clay_brush_tool_get_boundary (ClayBrushTool *brush_tool,
                              gint          *width,
                              gint          *height)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (brush_tool);
  ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paint_tool->core);

  if (paint_tool->draw_brush &&
      brush_core->main_brush &&
      brush_core->dynamics   &&
      brush_core->scale > 0.0)
    {
      return clay_brush_transform_boundary (brush_core->main_brush,
                                            brush_core->scale,
                                            brush_core->aspect_ratio,
                                            brush_core->angle,
                                            brush_core->reflect,
                                            brush_core->hardness,
                                            width,
                                            height);
    }

  return NULL;
}
