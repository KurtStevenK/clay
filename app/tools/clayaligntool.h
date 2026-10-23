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


/*  tool function/operation/state/mode  */
typedef enum
{
  ALIGN_TOOL_NO_ACTION,

  ALIGN_TOOL_REF_IDLE,
  ALIGN_TOOL_REF_PICK_LAYER,
  ALIGN_TOOL_REF_PICK_GUIDE,
  ALIGN_TOOL_REF_PICK_PATH,
  ALIGN_TOOL_REF_DRAG_BOX,

  ALIGN_TOOL_ALIGN_IDLE,
  ALIGN_TOOL_ALIGN_PICK_GUIDE,
  ALIGN_TOOL_ALIGN_ADD_GUIDE,
} ClayAlignToolFunction;


#define CLAY_TYPE_ALIGN_TOOL            (clay_align_tool_get_type ())
#define CLAY_ALIGN_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ALIGN_TOOL, ClayAlignTool))
#define CLAY_ALIGN_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ALIGN_TOOL, ClayAlignToolClass))
#define CLAY_IS_ALIGN_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ALIGN_TOOL))
#define CLAY_IS_ALIGN_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ALIGN_TOOL))
#define CLAY_ALIGN_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ALIGN_TOOL, ClayAlignToolClass))

#define CLAY_ALIGN_TOOL_GET_OPTIONS(t)  (CLAY_ALIGN_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayAlignTool      ClayAlignTool;
typedef struct _ClayAlignToolClass ClayAlignToolClass;

struct _ClayAlignTool
{
  ClayDrawTool           parent_instance;

  ClayAlignToolFunction  function;
  GList                 *selected_objects;

  gint                   x1, y1, x2, y2;   /* rubber-band rectangle */

  gboolean               set_reference;
};

struct _ClayAlignToolClass
{
  ClayDrawToolClass parent_class;
};


void    clay_align_tool_register (ClayToolRegisterCallback  callback,
                                  gpointer                  data);

GType   clay_align_tool_get_type (void);
