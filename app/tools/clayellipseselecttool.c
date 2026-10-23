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

#include "core/claychannel-select.h"
#include "core/clayimage.h"

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"

#include "clayellipseselecttool.h"
#include "clayrectangleselectoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void   clay_ellipse_select_tool_select (ClayRectangleSelectTool *rect_tool,
                                               ClayChannelOps           operation,
                                               gint                     x,
                                               gint                     y,
                                               gint                     w,
                                               gint                     h);


G_DEFINE_TYPE (ClayEllipseSelectTool, clay_ellipse_select_tool,
               CLAY_TYPE_RECTANGLE_SELECT_TOOL)

#define parent_class clay_ellipse_select_tool_parent_class


void
clay_ellipse_select_tool_register (ClayToolRegisterCallback  callback,
                                   gpointer                  data)
{
  (* callback) (CLAY_TYPE_ELLIPSE_SELECT_TOOL,
                CLAY_TYPE_RECTANGLE_SELECT_OPTIONS,
                clay_rectangle_select_options_gui,
                0,
                "clay-ellipse-select-tool",
                _("Ellipse Select"),
                _("Ellipse Select Tool: Select an elliptical region"),
                N_("_Ellipse Select"), "E",
                NULL, CLAY_HELP_TOOL_ELLIPSE_SELECT,
                CLAY_ICON_TOOL_ELLIPSE_SELECT,
                data);
}

static void
clay_ellipse_select_tool_class_init (ClayEllipseSelectToolClass *klass)
{
  ClayRectangleSelectToolClass *rect_tool_class;

  rect_tool_class = CLAY_RECTANGLE_SELECT_TOOL_CLASS (klass);

  rect_tool_class->select       = clay_ellipse_select_tool_select;
  rect_tool_class->draw_ellipse = TRUE;
}

static void
clay_ellipse_select_tool_init (ClayEllipseSelectTool *ellipse_select)
{
  ClayTool *tool = CLAY_TOOL (ellipse_select);

  clay_tool_control_set_tool_cursor (tool->control,
                                     CLAY_TOOL_CURSOR_ELLIPSE_SELECT);
}

static void
clay_ellipse_select_tool_select (ClayRectangleSelectTool *rect_tool,
                                 ClayChannelOps           operation,
                                 gint                     x,
                                 gint                     y,
                                 gint                     w,
                                 gint                     h)
{
  ClayTool             *tool    = CLAY_TOOL (rect_tool);
  ClaySelectionOptions *options = CLAY_SELECTION_TOOL_GET_OPTIONS (rect_tool);
  ClayImage            *image   = clay_display_get_image (tool->display);

  clay_channel_select_ellipse (clay_image_get_mask (image),
                               x, y, w, h,
                               operation,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);
}
