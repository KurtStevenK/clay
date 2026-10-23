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

#include "tools-types.h"

#include "core/claychannel.h"
#include "core/clayimage.h"
#include "core/claypickable.h"

#include "paint/claysourcecore.h"
#include "paint/claysourceoptions.h"

#include "widgets/claywidgets-utils.h"

#include "display/claycanvashandle.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-items.h"

#include "claysourcetool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static gboolean      clay_source_tool_has_display   (ClayTool            *tool,
                                                     ClayDisplay         *display);
static ClayDisplay * clay_source_tool_has_image     (ClayTool            *tool,
                                                     ClayImage           *image);
static void          clay_source_tool_control       (ClayTool            *tool,
                                                     ClayToolAction       action,
                                                     ClayDisplay         *display);
static void          clay_source_tool_button_press  (ClayTool            *tool,
                                                     const ClayCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     ClayButtonPressType  press_type,
                                                     ClayDisplay         *display);
static void          clay_source_tool_motion        (ClayTool            *tool,
                                                     const ClayCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     ClayDisplay         *display);
static void          clay_source_tool_cursor_update (ClayTool            *tool,
                                                     const ClayCoords    *coords,
                                                     GdkModifierType      state,
                                                     ClayDisplay         *display);
static void          clay_source_tool_modifier_key  (ClayTool            *tool,
                                                     GdkModifierType      key,
                                                     gboolean             press,
                                                     GdkModifierType      state,
                                                     ClayDisplay         *display);
static void          clay_source_tool_oper_update   (ClayTool            *tool,
                                                     const ClayCoords    *coords,
                                                     GdkModifierType      state,
                                                     gboolean             proximity,
                                                     ClayDisplay         *display);

static void          clay_source_tool_draw          (ClayDrawTool        *draw_tool);

static void          clay_source_tool_paint_prepare (ClayPaintTool       *paint_tool,
                                                     ClayDisplay         *display);

static void          clay_source_tool_set_src_display (ClaySourceTool      *source_tool,
                                                       ClayDisplay         *display);


G_DEFINE_TYPE (ClaySourceTool, clay_source_tool, CLAY_TYPE_BRUSH_TOOL)

#define parent_class clay_source_tool_parent_class


static void
clay_source_tool_class_init (ClaySourceToolClass *klass)
{
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass  *draw_tool_class  = CLAY_DRAW_TOOL_CLASS (klass);
  ClayPaintToolClass *paint_tool_class = CLAY_PAINT_TOOL_CLASS (klass);

  tool_class->has_display         = clay_source_tool_has_display;
  tool_class->has_image           = clay_source_tool_has_image;
  tool_class->control             = clay_source_tool_control;
  tool_class->button_press        = clay_source_tool_button_press;
  tool_class->motion              = clay_source_tool_motion;
  tool_class->modifier_key        = clay_source_tool_modifier_key;
  tool_class->oper_update         = clay_source_tool_oper_update;
  tool_class->cursor_update       = clay_source_tool_cursor_update;

  draw_tool_class->draw           = clay_source_tool_draw;

  paint_tool_class->paint_prepare = clay_source_tool_paint_prepare;
}

static void
clay_source_tool_init (ClaySourceTool *source)
{
  source->show_source_outline = TRUE;

  clay_paint_tool_enable_multi_paint (CLAY_PAINT_TOOL (source));
}

static gboolean
clay_source_tool_has_display (ClayTool    *tool,
                              ClayDisplay *display)
{
  ClaySourceTool *source_tool = CLAY_SOURCE_TOOL (tool);

  return (display == source_tool->src_display ||
          CLAY_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static ClayDisplay *
clay_source_tool_has_image (ClayTool  *tool,
                            ClayImage *image)
{
  ClaySourceTool *source_tool = CLAY_SOURCE_TOOL (tool);
  ClayDisplay    *display;

  display = CLAY_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && source_tool->src_display)
    {
      if (image && clay_display_get_image (source_tool->src_display) == image)
        display = source_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = source_tool->src_display;
    }

  return display;
}

static void
clay_source_tool_control (ClayTool       *tool,
                          ClayToolAction  action,
                          ClayDisplay    *display)
{
  ClaySourceTool    *source_tool = CLAY_SOURCE_TOOL (tool);
  ClaySourceOptions *options     = CLAY_SOURCE_TOOL_GET_OPTIONS (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_source_tool_set_src_display (source_tool, NULL);
      g_object_set (options,
                    "src-drawables", NULL,
                    "src-x",         0,
                    "src-y",         0,
                    NULL);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_source_tool_button_press (ClayTool            *tool,
                               const ClayCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               ClayButtonPressType  press_type,
                               ClayDisplay         *display)
{
  ClayPaintTool     *paint_tool  = CLAY_PAINT_TOOL (tool);
  ClaySourceTool    *source_tool = CLAY_SOURCE_TOOL (tool);
  ClaySourceCore    *source      = CLAY_SOURCE_CORE (paint_tool->core);
  ClaySourceOptions *options     = CLAY_SOURCE_TOOL_GET_OPTIONS (tool);
  GdkModifierType    extend_mask = clay_get_extend_selection_mask ();
  GdkModifierType    toggle_mask = clay_get_toggle_behavior_mask ();

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  if ((state & (toggle_mask | extend_mask)) == toggle_mask)
    {
      source->set_source = TRUE;

      clay_source_tool_set_src_display (source_tool, display);
    }
  else
    {
      source->set_source = FALSE;
    }

  CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  g_object_get (options,
                "src-x", &source_tool->src_x,
                "src-y", &source_tool->src_y,
                NULL);

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_source_tool_motion (ClayTool         *tool,
                         const ClayCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         ClayDisplay      *display)
{
  ClaySourceTool    *source_tool = CLAY_SOURCE_TOOL (tool);
  ClaySourceOptions *options     = CLAY_SOURCE_TOOL_GET_OPTIONS (tool);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  g_object_get (options,
                "src-x", &source_tool->src_x,
                "src-y", &source_tool->src_y,
                NULL);

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_source_tool_modifier_key (ClayTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               ClayDisplay     *display)
{
  ClaySourceTool    *source_tool = CLAY_SOURCE_TOOL (tool);
  ClayPaintTool     *paint_tool  = CLAY_PAINT_TOOL (tool);
  ClaySourceOptions *options     = CLAY_SOURCE_TOOL_GET_OPTIONS (tool);

  if (clay_source_core_use_source (CLAY_SOURCE_CORE (paint_tool->core),
                                   options) &&
      key == clay_get_toggle_behavior_mask ())
    {
      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

      if (press)
        {
          paint_tool->status = source_tool->status_set_source;

          source_tool->show_source_outline = FALSE;

          source_tool->saved_precision =
            clay_tool_control_get_precision (tool->control);
          clay_tool_control_set_precision (tool->control,
                                           CLAY_CURSOR_PRECISION_PIXEL_CENTER);
        }
      else
        {
          paint_tool->status = source_tool->status_paint;

          source_tool->show_source_outline = TRUE;

          clay_tool_control_set_precision (tool->control,
                                           source_tool->saved_precision);
        }

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
clay_source_tool_cursor_update (ClayTool         *tool,
                                const ClayCoords *coords,
                                GdkModifierType   state,
                                ClayDisplay      *display)
{
  ClayPaintTool      *paint_tool = CLAY_PAINT_TOOL (tool);
  ClaySourceOptions  *options    = CLAY_SOURCE_TOOL_GET_OPTIONS (tool);
  ClayCursorType      cursor     = CLAY_CURSOR_MOUSE;
  ClayCursorModifier  modifier   = CLAY_CURSOR_MODIFIER_NONE;

  if (clay_source_core_use_source (CLAY_SOURCE_CORE (paint_tool->core),
                                   options))
    {
      GdkModifierType extend_mask = clay_get_extend_selection_mask ();
      GdkModifierType toggle_mask = clay_get_toggle_behavior_mask ();

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          cursor = CLAY_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! options->src_drawables)
        {
          modifier = CLAY_CURSOR_MODIFIER_BAD;
        }
    }

  clay_tool_control_set_cursor          (tool->control, cursor);
  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_source_tool_oper_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              ClayDisplay      *display)
{
  ClayPaintTool     *paint_tool  = CLAY_PAINT_TOOL (tool);
  ClaySourceTool    *source_tool = CLAY_SOURCE_TOOL (tool);
  ClaySourceOptions *options     = CLAY_SOURCE_TOOL_GET_OPTIONS (tool);
  ClaySourceCore    *source;

  source = CLAY_SOURCE_CORE (CLAY_PAINT_TOOL (tool)->core);

  if (proximity)
    {
      if (clay_source_core_use_source (source, options))
        paint_tool->status_ctrl = source_tool->status_set_source_ctrl;
      else
        paint_tool->status_ctrl = NULL;
    }

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (clay_source_core_use_source (source, options))
    {
      if (options->src_drawables == NULL)
        {
          GdkModifierType toggle_mask = clay_get_toggle_behavior_mask ();

          if (state & toggle_mask)
            {
              clay_tool_replace_status (tool, display, "%s",
                                        source_tool->status_set_source);
            }
          else
            {
              clay_tool_replace_status (tool, display,
                                        source_tool->status_set_source_ctrl_click,
                                        clay_get_mod_string (toggle_mask));
            }
        }
      else
        {
          clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

          g_object_get (options,
                        "src-x", &source_tool->src_x,
                        "src-y", &source_tool->src_y,
                        NULL);

          if (! source->first_stroke)
            {
              switch (options->align_mode)
                {
                case CLAY_SOURCE_ALIGN_YES:
                  source_tool->src_x = floor (coords->x) + source->offset_x;
                  source_tool->src_y = floor (coords->y) + source->offset_y;
                  break;

                case CLAY_SOURCE_ALIGN_REGISTERED:
                  source_tool->src_x = floor (coords->x);
                  source_tool->src_y = floor (coords->y);
                  break;

                default:
                  break;
                }
            }

          clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
        }
    }
}

static void
clay_source_tool_draw (ClayDrawTool *draw_tool)
{
  ClaySourceTool    *source_tool = CLAY_SOURCE_TOOL (draw_tool);
  ClaySourceOptions *options     = CLAY_SOURCE_TOOL_GET_OPTIONS (draw_tool);
  ClaySourceCore    *source;

  source = CLAY_SOURCE_CORE (CLAY_PAINT_TOOL (draw_tool)->core);

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (clay_source_core_use_source (source, options) &&
      options->src_drawables && source_tool->src_display)
    {
      ClayDisplayShell *src_shell;
      gdouble           src_x;
      gdouble           src_y;

      src_shell = clay_display_get_shell (source_tool->src_display);

      src_x = (gdouble) source_tool->src_x + 0.5;
      src_y = (gdouble) source_tool->src_y + 0.5;

      if (source_tool->src_outline)
        {
          clay_display_shell_remove_tool_item (src_shell,
                                               source_tool->src_outline);
          source_tool->src_outline = NULL;
        }

      if (source_tool->show_source_outline)
        {
          source_tool->src_outline =
            clay_brush_tool_create_outline (CLAY_BRUSH_TOOL (source_tool),
                                            source_tool->src_display,
                                            src_x, src_y);

          if (source_tool->src_outline)
            {
              clay_display_shell_add_tool_item (src_shell,
                                                source_tool->src_outline);
              g_object_unref (source_tool->src_outline);
            }
        }

      if (source_tool->src_outline)
        {
          if (source_tool->src_handle)
            {
              clay_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_handle);
              source_tool->src_handle = NULL;
            }
        }
      else
        {
          if (! source_tool->src_handle)
            {
              source_tool->src_handle =
                clay_canvas_handle_new (src_shell,
                                        CLAY_HANDLE_CROSS,
                                        CLAY_HANDLE_ANCHOR_CENTER,
                                        src_x, src_y,
                                        CLAY_TOOL_HANDLE_SIZE_CROSS,
                                        CLAY_TOOL_HANDLE_SIZE_CROSS);
              clay_display_shell_add_tool_item (src_shell,
                                                source_tool->src_handle);
              g_object_unref (source_tool->src_handle);
            }
          else
            {
              clay_canvas_handle_set_position (source_tool->src_handle,
                                               src_x, src_y);
            }
        }
    }
}

static void
clay_source_tool_paint_prepare (ClayPaintTool *paint_tool,
                                ClayDisplay   *display)
{
  ClaySourceTool *source_tool = CLAY_SOURCE_TOOL (paint_tool);

  if (CLAY_PAINT_TOOL_CLASS (parent_class)->paint_prepare)
    CLAY_PAINT_TOOL_CLASS (parent_class)->paint_prepare (paint_tool, display);

  if (source_tool->src_display)
    {
      ClayDisplayShell *src_shell;

      src_shell = clay_display_get_shell (source_tool->src_display);

      clay_paint_core_set_show_all (paint_tool->core, src_shell->show_all);
    }
}

static void
clay_source_tool_set_src_display (ClaySourceTool *source_tool,
                                  ClayDisplay    *display)
{
  if (source_tool->src_display != display)
    {
      if (source_tool->src_display)
        {
          ClayDisplayShell *src_shell;

          src_shell = clay_display_get_shell (source_tool->src_display);

          if (source_tool->src_handle)
            {
              clay_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_handle);
              source_tool->src_handle = NULL;
            }

          if (source_tool->src_outline)
            {
              clay_display_shell_remove_tool_item (src_shell,
                                                   source_tool->src_outline);
              source_tool->src_outline = NULL;
            }
        }

      source_tool->src_display = display;
    }
}
