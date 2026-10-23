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

#include "config/clayguiconfig.h"

#include "core/clayimage.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasrectangle.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-scale.h"

#include "claymagnifyoptions.h"
#include "claymagnifytool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void   clay_magnify_tool_button_press   (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayButtonPressType    press_type,
                                                ClayDisplay           *display);
static void   clay_magnify_tool_button_release (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayButtonReleaseType  release_type,
                                                ClayDisplay           *display);
static void   clay_magnify_tool_motion         (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayDisplay           *display);
static void   clay_magnify_tool_modifier_key   (ClayTool              *tool,
                                                GdkModifierType        key,
                                                gboolean               press,
                                                GdkModifierType        state,
                                                ClayDisplay           *display);
static void   clay_magnify_tool_cursor_update  (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                ClayDisplay           *display);

static void   clay_magnify_tool_draw           (ClayDrawTool          *draw_tool);

static void   clay_magnify_tool_update_items   (ClayMagnifyTool       *magnify);


G_DEFINE_TYPE (ClayMagnifyTool, clay_magnify_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_magnify_tool_parent_class


void
clay_magnify_tool_register (ClayToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (CLAY_TYPE_MAGNIFY_TOOL,
                CLAY_TYPE_MAGNIFY_OPTIONS,
                clay_magnify_options_gui,
                0,
                "clay-zoom-tool",
                _("Zoom"),
                _("Zoom Tool: Adjust the zoom level"),
                N_("_Zoom"), "Z",
                NULL, CLAY_HELP_TOOL_ZOOM,
                CLAY_ICON_TOOL_ZOOM,
                data);
}

static void
clay_magnify_tool_class_init (ClayMagnifyToolClass *klass)
{
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = clay_magnify_tool_button_press;
  tool_class->button_release = clay_magnify_tool_button_release;
  tool_class->motion         = clay_magnify_tool_motion;
  tool_class->modifier_key   = clay_magnify_tool_modifier_key;
  tool_class->cursor_update  = clay_magnify_tool_cursor_update;
  tool_class->is_destructive = FALSE;

  draw_tool_class->draw      = clay_magnify_tool_draw;
}

static void
clay_magnify_tool_init (ClayMagnifyTool *magnify_tool)
{
  ClayTool *tool = CLAY_TOOL (magnify_tool);

  clay_tool_control_set_scroll_lock            (tool->control, TRUE);
  clay_tool_control_set_handle_empty_image     (tool->control, TRUE);
  clay_tool_control_set_wants_click            (tool->control, TRUE);
  clay_tool_control_set_snap_to                (tool->control, FALSE);

  clay_tool_control_set_tool_cursor            (tool->control,
                                                CLAY_TOOL_CURSOR_ZOOM);
  clay_tool_control_set_cursor_modifier        (tool->control,
                                                CLAY_CURSOR_MODIFIER_PLUS);
  clay_tool_control_set_toggle_cursor_modifier (tool->control,
                                                CLAY_CURSOR_MODIFIER_MINUS);

  magnify_tool->x = 0;
  magnify_tool->y = 0;
  magnify_tool->w = 0;
  magnify_tool->h = 0;
}

static void
clay_magnify_tool_button_press (ClayTool            *tool,
                                const ClayCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                ClayButtonPressType  press_type,
                                ClayDisplay         *display)
{
  ClayMagnifyTool *magnify = CLAY_MAGNIFY_TOOL (tool);

  magnify->x = coords->x;
  magnify->y = coords->y;
  magnify->w = 0;
  magnify->h = 0;

  clay_tool_control_activate (tool->control);
  tool->display = display;

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}

static void
clay_magnify_tool_button_release (ClayTool              *tool,
                                  const ClayCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  ClayButtonReleaseType  release_type,
                                  ClayDisplay           *display)
{
  ClayMagnifyTool    *magnify = CLAY_MAGNIFY_TOOL (tool);
  ClayMagnifyOptions *options = CLAY_MAGNIFY_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell   *shell   = clay_display_get_shell (tool->display);

  clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  clay_tool_control_halt (tool->control);

  switch (release_type)
    {
    case CLAY_BUTTON_RELEASE_CLICK:
    case CLAY_BUTTON_RELEASE_NO_MOTION:
      clay_display_shell_scale (shell,
                                options->zoom_type,
                                0.0,
                                CLAY_ZOOM_FOCUS_POINTER);
      break;

    case CLAY_BUTTON_RELEASE_NORMAL:
      {
        gdouble  x, y;
        gdouble  width, height;
        gboolean resize_window;

        x      = (magnify->w < 0) ?  magnify->x + magnify->w : magnify->x;
        y      = (magnify->h < 0) ?  magnify->y + magnify->h : magnify->y;
        width  = (magnify->w < 0) ? -magnify->w : magnify->w;
        height = (magnify->h < 0) ? -magnify->h : magnify->h;

        /* Resize windows only in multi-window mode */
        resize_window = (options->auto_resize &&
                         ! CLAY_GUI_CONFIG (display->config)->single_window_mode);

        clay_display_shell_scale_to_rectangle (shell,
                                               options->zoom_type,
                                               x, y, width, height,
                                               resize_window);
      }
      break;

    default:
      break;
    }
}

static void
clay_magnify_tool_motion (ClayTool         *tool,
                          const ClayCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          ClayDisplay      *display)
{
  ClayMagnifyTool *magnify = CLAY_MAGNIFY_TOOL (tool);

  magnify->w = coords->x - magnify->x;
  magnify->h = coords->y - magnify->y;

  clay_magnify_tool_update_items (magnify);
}

static void
clay_magnify_tool_modifier_key (ClayTool        *tool,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state,
                                ClayDisplay     *display)
{
  ClayMagnifyOptions *options = CLAY_MAGNIFY_TOOL_GET_OPTIONS (tool);

  if (key == clay_get_toggle_behavior_mask ())
    {
      switch (options->zoom_type)
        {
        case CLAY_ZOOM_IN:
          g_object_set (options, "zoom-type", CLAY_ZOOM_OUT, NULL);
          break;

        case CLAY_ZOOM_OUT:
          g_object_set (options, "zoom-type", CLAY_ZOOM_IN, NULL);
          break;

        default:
          break;
        }
    }
}

static void
clay_magnify_tool_cursor_update (ClayTool         *tool,
                                 const ClayCoords *coords,
                                 GdkModifierType   state,
                                 ClayDisplay      *display)
{
  ClayMagnifyOptions *options = CLAY_MAGNIFY_TOOL_GET_OPTIONS (tool);

  clay_tool_control_set_toggled (tool->control,
                                 options->zoom_type == CLAY_ZOOM_OUT);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_magnify_tool_draw (ClayDrawTool *draw_tool)
{
  ClayMagnifyTool *magnify = CLAY_MAGNIFY_TOOL (draw_tool);

  magnify->rectangle =
    clay_draw_tool_add_rectangle (draw_tool, FALSE,
                                  magnify->x,
                                  magnify->y,
                                  magnify->w,
                                  magnify->h);
}

static void
clay_magnify_tool_update_items (ClayMagnifyTool *magnify)
{
  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (magnify)))
    {
      clay_canvas_rectangle_set (magnify->rectangle,
                                 magnify->x,
                                 magnify->y,
                                 magnify->w,
                                 magnify->h);
    }
}
