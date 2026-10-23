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

#pragma once

#include "claydrawtool.h"


#define CLAY_TYPE_GUIDE_TOOL            (clay_guide_tool_get_type ())
#define CLAY_GUIDE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GUIDE_TOOL, ClayGuideTool))
#define CLAY_GUIDE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GUIDE_TOOL, ClayGuideToolClass))
#define CLAY_IS_GUIDE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GUIDE_TOOL))
#define CLAY_IS_GUIDE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GUIDE_TOOL))
#define CLAY_GUIDE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GUIDE_TOOL, ClayGuideToolClass))


typedef struct _ClayGuideToolGuide ClayGuideToolGuide;
typedef struct _ClayGuideTool      ClayGuideTool;
typedef struct _ClayGuideToolClass ClayGuideToolClass;

struct _ClayGuideToolGuide
{
  ClayGuide           *guide;

  gint                 old_position;
  gint                 position;
  ClayOrientationType  orientation;
  gboolean             custom;
};

struct _ClayGuideTool
{
  ClayDrawTool        parent_instance;

  ClayGuideToolGuide *guides;
  gint                n_guides;
};

struct _ClayGuideToolClass
{
  ClayDrawToolClass  parent_class;
};


GType   clay_guide_tool_get_type        (void);

void    clay_guide_tool_start_new       (ClayTool            *parent_tool,
                                         ClayDisplay         *display,
                                         ClayOrientationType  orientation);
void    clay_guide_tool_start_edit      (ClayTool            *parent_tool,
                                         ClayDisplay         *display,
                                         ClayGuide           *guide);
void    clay_guide_tool_start_edit_many (ClayTool            *parent_tool,
                                         ClayDisplay         *display,
                                         GList               *guides);
