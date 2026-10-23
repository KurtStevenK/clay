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

#include "core/clay.h"
#include "core/claysamplepoint.h"
#include "core/clayimage.h"
#include "core/clayimage-sample-points.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-selection.h"
#include "display/claydisplayshell-transform.h"

#include "claysamplepointtool.h"
#include "claytoolcontrol.h"
#include "tool_manager.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_sample_point_tool_button_release (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     ClayButtonReleaseType  release_type,
                                                     ClayDisplay           *display);
static void   clay_sample_point_tool_motion         (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     ClayDisplay           *display);

static void   clay_sample_point_tool_draw           (ClayDrawTool          *draw_tool);

static void   clay_sample_point_tool_start          (ClayTool              *parent_tool,
                                                     ClayDisplay           *display,
                                                     ClaySamplePoint       *sample_point);


G_DEFINE_TYPE (ClaySamplePointTool, clay_sample_point_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_sample_point_tool_parent_class


static void
clay_sample_point_tool_class_init (ClaySamplePointToolClass *klass)
{
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  tool_class->button_release = clay_sample_point_tool_button_release;
  tool_class->motion         = clay_sample_point_tool_motion;

  draw_tool_class->draw      = clay_sample_point_tool_draw;
}

static void
clay_sample_point_tool_init (ClaySamplePointTool *sp_tool)
{
  ClayTool *tool = CLAY_TOOL (sp_tool);

  clay_tool_control_set_snap_to            (tool->control, FALSE);
  clay_tool_control_set_handle_empty_image (tool->control, TRUE);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_MOVE);
  clay_tool_control_set_scroll_lock        (tool->control, TRUE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_PIXEL_CENTER);

  sp_tool->sample_point   = NULL;
  sp_tool->sample_point_x = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;
  sp_tool->sample_point_y = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;
}

static void
clay_sample_point_tool_button_release (ClayTool              *tool,
                                       const ClayCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       ClayButtonReleaseType  release_type,
                                       ClayDisplay           *display)
{
  ClaySamplePointTool *sp_tool = CLAY_SAMPLE_POINT_TOOL (tool);
  ClayDisplayShell    *shell   = clay_display_get_shell (display);
  ClayImage           *image   = clay_display_get_image (display);

  clay_tool_pop_status (tool, display);

  clay_tool_control_halt (tool->control);

  clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  if (release_type != CLAY_BUTTON_RELEASE_CANCEL)
    {
      gint width  = clay_image_get_width  (image);
      gint height = clay_image_get_height (image);

      if (sp_tool->sample_point_x == CLAY_SAMPLE_POINT_POSITION_UNDEFINED ||
          sp_tool->sample_point_x <  0                                    ||
          sp_tool->sample_point_x >= width                                ||
          sp_tool->sample_point_y == CLAY_SAMPLE_POINT_POSITION_UNDEFINED ||
          sp_tool->sample_point_y <  0                                    ||
          sp_tool->sample_point_y >= height)
        {
          if (sp_tool->sample_point)
            {
              clay_image_remove_sample_point (image,
                                              sp_tool->sample_point, TRUE);
              sp_tool->sample_point = NULL;
            }
        }
      else
        {
          if (sp_tool->sample_point)
            {
              clay_image_move_sample_point (image,
                                            sp_tool->sample_point,
                                            sp_tool->sample_point_x,
                                            sp_tool->sample_point_y,
                                            TRUE);
            }
          else
            {
              sp_tool->sample_point =
                clay_image_add_sample_point_at_pos (image,
                                                    sp_tool->sample_point_x,
                                                    sp_tool->sample_point_y,
                                                    TRUE);
            }
        }

      clay_image_flush (image);
    }

  clay_display_shell_selection_resume (shell);

  sp_tool->sample_point_x = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;
  sp_tool->sample_point_y = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;

  tool_manager_pop_tool (display->clay);
  g_object_unref (sp_tool);

  {
    ClayTool *active_tool = tool_manager_get_active (display->clay);

    if (CLAY_IS_DRAW_TOOL (active_tool))
      clay_draw_tool_pause (CLAY_DRAW_TOOL (active_tool));

    tool_manager_oper_update_active (display->clay, coords, state,
                                     TRUE, display);
    tool_manager_cursor_update_active (display->clay, coords, state,
                                       display);

    if (CLAY_IS_DRAW_TOOL (active_tool))
      clay_draw_tool_resume (CLAY_DRAW_TOOL (active_tool));
  }
}

static void
clay_sample_point_tool_motion (ClayTool         *tool,
                               const ClayCoords *coords,
                               guint32           time,
                               GdkModifierType   state,
                               ClayDisplay      *display)

{
  ClaySamplePointTool *sp_tool      = CLAY_SAMPLE_POINT_TOOL (tool);
  ClayDisplayShell    *shell        = clay_display_get_shell (display);
  gboolean             delete_point = FALSE;
  gint                 tx, ty;

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  clay_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  if (tx < 0 || tx >= shell->disp_width ||
      ty < 0 || ty >= shell->disp_height)
    {
      sp_tool->sample_point_x = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;
      sp_tool->sample_point_y = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;

      delete_point = TRUE;
    }
  else
    {
      ClayImage *image  = clay_display_get_image (display);
      gint       width  = clay_image_get_width  (image);
      gint       height = clay_image_get_height (image);

      sp_tool->sample_point_x = floor (coords->x);
      sp_tool->sample_point_y = floor (coords->y);

      if (sp_tool->sample_point_x <  0      ||
          sp_tool->sample_point_x >= height ||
          sp_tool->sample_point_y <  0      ||
          sp_tool->sample_point_y >= width)
        {
          delete_point = TRUE;
        }
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));

  clay_tool_pop_status (tool, display);

  if (delete_point)
    {
      clay_tool_push_status (tool, display,
                             sp_tool->sample_point ?
                             _("Remove Sample Point") :
                             _("Cancel Sample Point"));
    }
  else if (sp_tool->sample_point)
    {
      clay_tool_push_status_coords (tool, display,
                                    clay_tool_control_get_precision (tool->control),
                                    _("Move Sample Point: "),
                                    sp_tool->sample_point_x -
                                    sp_tool->sample_point_old_x,
                                    ", ",
                                    sp_tool->sample_point_y -
                                    sp_tool->sample_point_old_y,
                                    NULL);
    }
  else
    {
      clay_tool_push_status_coords (tool, display,
                                    clay_tool_control_get_precision (tool->control),
                                    _("Add Sample Point: "),
                                    sp_tool->sample_point_x,
                                    ", ",
                                    sp_tool->sample_point_y,
                                    NULL);
    }
}

static void
clay_sample_point_tool_draw (ClayDrawTool *draw_tool)
{
  ClaySamplePointTool *sp_tool = CLAY_SAMPLE_POINT_TOOL (draw_tool);

  if (sp_tool->sample_point_x != CLAY_SAMPLE_POINT_POSITION_UNDEFINED &&
      sp_tool->sample_point_y != CLAY_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      clay_draw_tool_add_crosshair (draw_tool,
                                    sp_tool->sample_point_x,
                                    sp_tool->sample_point_y);
    }
}

static void
clay_sample_point_tool_start (ClayTool        *parent_tool,
                              ClayDisplay     *display,
                              ClaySamplePoint *sample_point)
{
  ClaySamplePointTool *sp_tool;
  ClayTool            *tool;

  sp_tool = g_object_new (CLAY_TYPE_SAMPLE_POINT_TOOL,
                          "tool-info", parent_tool->tool_info,
                          NULL);

  tool = CLAY_TOOL (sp_tool);

  clay_display_shell_selection_pause (clay_display_get_shell (display));

  if (sample_point)
    {
      sp_tool->sample_point = sample_point;

      clay_sample_point_get_position (sample_point,
                                      &sp_tool->sample_point_old_x,
                                      &sp_tool->sample_point_old_y);

      sp_tool->sample_point_x = sp_tool->sample_point_old_x;
      sp_tool->sample_point_y = sp_tool->sample_point_old_y;
    }
  else
    {
      sp_tool->sample_point       = NULL;
      sp_tool->sample_point_old_x = 0;
      sp_tool->sample_point_old_y = 0;
      sp_tool->sample_point_x     = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;
      sp_tool->sample_point_y     = CLAY_SAMPLE_POINT_POSITION_UNDEFINED;
    }

  clay_tool_set_cursor (tool, display,
                        CLAY_CURSOR_MOUSE,
                        CLAY_TOOL_CURSOR_COLOR_PICKER,
                        CLAY_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->clay, tool);

  tool->display = display;
  clay_tool_control_activate (tool->control);

  clay_draw_tool_start (CLAY_DRAW_TOOL (sp_tool), display);

  if (sp_tool->sample_point)
    {
      clay_tool_push_status_coords (tool, display,
                                    clay_tool_control_get_precision (tool->control),
                                    _("Move Sample Point: "),
                                    sp_tool->sample_point_x -
                                    sp_tool->sample_point_old_x,
                                    ", ",
                                    sp_tool->sample_point_y -
                                    sp_tool->sample_point_old_y,
                                    NULL);
    }
  else
    {
      clay_tool_push_status_coords (tool, display,
                                    clay_tool_control_get_precision (tool->control),
                                    _("Add Sample Point: "),
                                    sp_tool->sample_point_x,
                                    ", ",
                                    sp_tool->sample_point_y,
                                    NULL);
    }
}


/*  public functions  */

void
clay_sample_point_tool_start_new (ClayTool    *parent_tool,
                                  ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_TOOL (parent_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  clay_sample_point_tool_start (parent_tool, display, NULL);
}

void
clay_sample_point_tool_start_edit (ClayTool        *parent_tool,
                                   ClayDisplay     *display,
                                   ClaySamplePoint *sample_point)
{
  g_return_if_fail (CLAY_IS_TOOL (parent_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  clay_sample_point_tool_start (parent_tool, display, sample_point);
}
