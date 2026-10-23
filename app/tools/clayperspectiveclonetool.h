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

#include "claybrushtool.h"
#include "claytransformtool.h"  /* for TransInfo */


#define CLAY_TYPE_PERSPECTIVE_CLONE_TOOL            (clay_perspective_clone_tool_get_type ())
#define CLAY_PERSPECTIVE_CLONE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PERSPECTIVE_CLONE_TOOL, ClayPerspectiveCloneTool))
#define CLAY_PERSPECTIVE_CLONE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PERSPECTIVE_CLONE_TOOL, ClayPerspectiveCloneToolClass))
#define CLAY_IS_PERSPECTIVE_CLONE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PERSPECTIVE_CLONE_TOOL))
#define CLAY_IS_PERSPECTIVE_CLONE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PERSPECTIVE_CLONE_TOOL))
#define CLAY_PERSPECTIVE_CLONE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PERSPECTIVE_CLONE_TOOL, ClayPerspectiveCloneToolClass))

#define CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS(t)  (CLAY_PERSPECTIVE_CLONE_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayPerspectiveCloneTool      ClayPerspectiveCloneTool;
typedef struct _ClayPerspectiveCloneToolClass ClayPerspectiveCloneToolClass;

struct _ClayPerspectiveCloneTool
{
  ClayBrushTool        parent_instance;

  ClayDisplay         *src_display;
  gint                 src_x;
  gint                 src_y;

  ClayMatrix3          transform;      /*  transformation matrix            */
  TransInfo            trans_info;     /*  transformation info              */
  TransInfo            old_trans_info; /*  for cancelling a drag operation  */

  gint                 x1, y1;         /*  upper left hand coordinate       */
  gint                 x2, y2;         /*  lower right hand coords          */

  ClayCursorPrecision  saved_precision;

  ClayToolWidget      *widget;
  ClayToolWidget      *grab_widget;
};

struct _ClayPerspectiveCloneToolClass
{
  ClayBrushToolClass  parent_class;
};


void    clay_perspective_clone_tool_register (ClayToolRegisterCallback  callback,
                                              gpointer                  data);

GType   clay_perspective_clone_tool_get_type (void);
