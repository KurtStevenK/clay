/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybycolorselecttool.c
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

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/clayimage-new.h"
#include "core/clayitem.h"
#include "core/claychannel.h"
#include "core/claypickable.h"
#include "core/claypickable-contiguous-region.h"

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"

#include "claybycolorselecttool.h"
#include "clayregionselectoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static GeglBuffer * clay_by_color_select_tool_get_mask (ClayRegionSelectTool *region_select,
                                                        ClayDisplay          *display);


G_DEFINE_TYPE (ClayByColorSelectTool, clay_by_color_select_tool,
               CLAY_TYPE_REGION_SELECT_TOOL)

#define parent_class clay_by_color_select_tool_parent_class


void
clay_by_color_select_tool_register (ClayToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (CLAY_TYPE_BY_COLOR_SELECT_TOOL,
                CLAY_TYPE_REGION_SELECT_OPTIONS,
                clay_region_select_options_gui,
                0,
                "clay-by-color-select-tool",
                _("Select by Color"),
                _("Select by Color Tool: Select regions with similar colors"),
                N_("_By Color Select"), "<shift>O",
                NULL, CLAY_HELP_TOOL_BY_COLOR_SELECT,
                CLAY_ICON_TOOL_BY_COLOR_SELECT,
                data);
}

static void
clay_by_color_select_tool_class_init (ClayByColorSelectToolClass *klass)
{
  ClayRegionSelectToolClass *region_class;

  region_class = CLAY_REGION_SELECT_TOOL_CLASS (klass);

  region_class->undo_desc = C_("command", "Select by Color");
  region_class->get_mask  = clay_by_color_select_tool_get_mask;
}

static void
clay_by_color_select_tool_init (ClayByColorSelectTool *by_color_select)
{
  ClayTool *tool = CLAY_TOOL (by_color_select);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_HAND);
}

static GeglBuffer *
clay_by_color_select_tool_get_mask (ClayRegionSelectTool *region_select,
                                    ClayDisplay          *display)
{
  ClayTool                *tool         = CLAY_TOOL (region_select);
  ClaySelectionOptions    *sel_options  = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  ClayRegionSelectOptions *options      = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  ClayImage               *image        = clay_display_get_image (display);
  ClayImage               *select_image = NULL;
  GList                   *drawables    = clay_image_get_selected_drawables (image);
  ClayPickable            *pickable;
  GeglBuffer              *mask         = NULL;
  GeglColor               *color;
  gint                     x, y;
  GeglRectangle            roi          = { 0, 0, 0, 0 };
  ClayChannel             *selection    = clay_image_get_mask (image);

  x = region_select->x;
  y = region_select->y;

  if (! options->sample_merged)
    {
      if (g_list_length (drawables) == 1)
        {
          gint off_x, off_y;

          clay_item_get_offset (drawables->data, &off_x, &off_y);

          x -= off_x;
          y -= off_y;

          pickable = CLAY_PICKABLE (drawables->data);
        }
      else
        {
          select_image = clay_image_new_from_drawables (image->clay, drawables, FALSE, FALSE);
          clay_container_remove (image->clay->images, CLAY_OBJECT (select_image));

          pickable = CLAY_PICKABLE (select_image);
          clay_pickable_flush (pickable);
        }
    }
  else
    {
      pickable = CLAY_PICKABLE (image);
    }

  g_list_free (drawables);
  clay_pickable_flush (pickable);

  if (sel_options->operation == CLAY_CHANNEL_OP_INTERSECT &&
      ! clay_channel_is_empty (selection))
    clay_item_bounds (CLAY_ITEM (selection), &roi.x , &roi.y, &roi.width,
                      &roi.height);

  if ((color = clay_pickable_get_color_at (pickable, x, y)) != NULL)
    {
      mask = clay_pickable_contiguous_region_by_color (pickable,
                                                       sel_options->antialias,
                                                       options->threshold / 255.0,
                                                       options->select_transparent,
                                                       options->select_criterion,
                                                       color,
                                                       &roi);
      g_object_unref (color);
    }

  if (select_image)
    g_object_unref (select_image);

  return mask;
}
