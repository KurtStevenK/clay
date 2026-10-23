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


#define CLAY_TYPE_COLOR_TOOL            (clay_color_tool_get_type ())
#define CLAY_COLOR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_TOOL, ClayColorTool))
#define CLAY_COLOR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_TOOL, ClayColorToolClass))
#define CLAY_IS_COLOR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_TOOL))
#define CLAY_IS_COLOR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_TOOL))
#define CLAY_COLOR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_TOOL, ClayColorToolClass))

#define CLAY_COLOR_TOOL_GET_OPTIONS(t)  (CLAY_COLOR_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayColorToolClass ClayColorToolClass;

struct _ClayColorTool
{
  ClayDrawTool         parent_instance;

  gboolean             enabled;
  ClayColorOptions    *options;
  gboolean             saved_snap_to;

  ClayColorPickTarget  pick_target;

  gboolean             can_pick;
  gint                 center_x;
  gint                 center_y;
  ClaySamplePoint     *sample_point;
};

struct _ClayColorToolClass
{
  ClayDrawToolClass  parent_class;

  /*  virtual functions  */
  gboolean (* can_pick) (ClayColorTool      *tool,
                         const ClayCoords   *coords,
                         ClayDisplay        *display);
  gboolean (* pick)     (ClayColorTool      *tool,
                         const ClayCoords   *coords,
                         ClayDisplay        *display,
                         const Babl        **sample_format,
                         gpointer            pixel,
                         GeglColor         **color);

  /*  signals  */
  void     (* picked)   (ClayColorTool      *tool,
                         const ClayCoords   *coords,
                         ClayDisplay        *display,
                         ClayColorPickState  pick_state,
                         const Babl         *sample_format,
                         gpointer            pixel,
                         GeglColor          *color);
};


GType      clay_color_tool_get_type   (void);

void       clay_color_tool_enable     (ClayColorTool    *color_tool,
                                       ClayColorOptions *options);
void       clay_color_tool_disable    (ClayColorTool    *color_tool);
gboolean   clay_color_tool_is_enabled (ClayColorTool    *color_tool);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayColorTool, g_object_unref);
