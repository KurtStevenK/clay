/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfuzzyselecttool.c
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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/clayimage-new.h"
#include "core/clayitem.h"
#include "core/claypickable.h"
#include "core/claypickable-contiguous-region.h"

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"

#include "clayfuzzyselecttool.h"
#include "clayregionselectoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static GeglBuffer * clay_fuzzy_select_tool_get_mask (ClayRegionSelectTool *region_select,
                                                     ClayDisplay          *display);


G_DEFINE_TYPE (ClayFuzzySelectTool, clay_fuzzy_select_tool,
               CLAY_TYPE_REGION_SELECT_TOOL)

#define parent_class clay_fuzzy_select_tool_parent_class


void
clay_fuzzy_select_tool_register (ClayToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (CLAY_TYPE_FUZZY_SELECT_TOOL,
                CLAY_TYPE_REGION_SELECT_OPTIONS,
                clay_region_select_options_gui,
                0,
                "clay-fuzzy-select-tool",
                _("Fuzzy Select"),
                _("Fuzzy Select Tool: Select a contiguous region on the basis of color"),
                N_("Fu_zzy Select"), "U",
                NULL, CLAY_HELP_TOOL_FUZZY_SELECT,
                CLAY_ICON_TOOL_FUZZY_SELECT,
                data);
}

static void
clay_fuzzy_select_tool_class_init (ClayFuzzySelectToolClass *klass)
{
  ClayRegionSelectToolClass *region_class;

  region_class = CLAY_REGION_SELECT_TOOL_CLASS (klass);

  region_class->undo_desc = C_("command", "Fuzzy Select");
  region_class->get_mask  = clay_fuzzy_select_tool_get_mask;
}

static void
clay_fuzzy_select_tool_init (ClayFuzzySelectTool *fuzzy_select)
{
  ClayTool *tool = CLAY_TOOL (fuzzy_select);

  clay_tool_control_set_tool_cursor (tool->control,
                                     CLAY_TOOL_CURSOR_FUZZY_SELECT);
}

static GeglBuffer *
clay_fuzzy_select_tool_get_mask (ClayRegionSelectTool *region_select,
                                 ClayDisplay          *display)
{
  ClayTool                *tool         = CLAY_TOOL (region_select);
  ClaySelectionOptions    *sel_options  = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  ClayRegionSelectOptions *options      = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  ClayImage               *image        = clay_display_get_image (display);
  ClayImage               *select_image = NULL;
  GList                   *drawables    = clay_image_get_selected_drawables (image);
  ClayPickable            *pickable;
  GeglBuffer              *mask;
  gint                     x, y;

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

  mask = clay_pickable_contiguous_region_by_seed (pickable,
                                                  sel_options->antialias,
                                                  options->threshold / 255.0,
                                                  options->select_transparent,
                                                  options->select_criterion,
                                                  options->diagonal_neighbors,
                                                  x, y);

  if (select_image)
    g_object_unref (select_image);

  return mask;
}
