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

#include "claypolygonselecttool.h"


typedef enum
{
  MATTING_STATE_FREE_SELECT = 0,
  MATTING_STATE_PAINT_TRIMAP,
  MATTING_STATE_PREVIEW_MASK,
} MattingState;


#define CLAY_TYPE_FOREGROUND_SELECT_TOOL            (clay_foreground_select_tool_get_type ())
#define CLAY_FOREGROUND_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FOREGROUND_SELECT_TOOL, ClayForegroundSelectTool))
#define CLAY_FOREGROUND_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FOREGROUND_SELECT_TOOL, ClayForegroundSelectToolClass))
#define CLAY_IS_FOREGROUND_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FOREGROUND_SELECT_TOOL))
#define CLAY_IS_FOREGROUND_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FOREGROUND_SELECT_TOOL))
#define CLAY_FOREGROUND_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FOREGROUND_SELECT_TOOL, ClayForegroundSelectToolClass))

#define CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS(t)  (CLAY_FOREGROUND_SELECT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayForegroundSelectTool      ClayForegroundSelectTool;
typedef struct _ClayForegroundSelectToolClass ClayForegroundSelectToolClass;

struct _ClayForegroundSelectTool
{
  ClayPolygonSelectTool  parent_instance;

  MattingState           state;

  ClayCoords             last_coords;
  GArray                *stroke;
  GeglBuffer            *trimap;
  GeglBuffer            *mask;

  GList                 *undo_stack;
  GList                 *redo_stack;

  ClayToolGui           *gui;
  GtkWidget             *preview_toggle;

  ClayCanvasItem        *grayscale_preview;
};

struct _ClayForegroundSelectToolClass
{
  ClayPolygonSelectToolClass  parent_class;
};


void    clay_foreground_select_tool_register (ClayToolRegisterCallback  callback,
                                              gpointer                  data);

GType   clay_foreground_select_tool_get_type (void);
