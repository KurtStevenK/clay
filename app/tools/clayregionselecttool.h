/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayregionselecttool.h
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

#pragma once

#include "clayselectiontool.h"


#define CLAY_TYPE_REGION_SELECT_TOOL            (clay_region_select_tool_get_type ())
#define CLAY_REGION_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_REGION_SELECT_TOOL, ClayRegionSelectTool))
#define CLAY_REGION_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_REGION_SELECT_TOOL, ClayRegionSelectToolClass))
#define CLAY_IS_REGION_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_REGION_SELECT_TOOL))
#define CLAY_IS_REGION_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_REGION_SELECT_TOOL))
#define CLAY_REGION_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_REGION_SELECT_TOOL, ClayRegionSelectToolClass))

#define CLAY_REGION_SELECT_TOOL_GET_OPTIONS(t)  (CLAY_REGION_SELECT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayRegionSelectTool      ClayRegionSelectTool;
typedef struct _ClayRegionSelectToolClass ClayRegionSelectToolClass;

struct _ClayRegionSelectTool
{
  ClaySelectionTool  parent_instance;

  gint               x, y;
  gdouble            saved_threshold;

  GeglBuffer        *region_mask;
  ClayBoundSeg      *segs;
  gint               n_segs;
};

struct _ClayRegionSelectToolClass
{
  ClaySelectionToolClass parent_class;

  const gchar * undo_desc;

  GeglBuffer * (* get_mask) (ClayRegionSelectTool *region_tool,
                             ClayDisplay          *display);
};


GType   clay_region_select_tool_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayRegionSelectTool, g_object_unref);
