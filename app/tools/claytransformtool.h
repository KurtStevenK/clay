/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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


/* This is not the number of items in the enum above, but the max size
 * of the enums at the top of each transformation tool, stored in
 * trans_info and related
 */
#define TRANS_INFO_SIZE 17

typedef gdouble TransInfo[TRANS_INFO_SIZE];


#define CLAY_TYPE_TRANSFORM_TOOL            (clay_transform_tool_get_type ())
#define CLAY_TRANSFORM_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRANSFORM_TOOL, ClayTransformTool))
#define CLAY_TRANSFORM_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRANSFORM_TOOL, ClayTransformToolClass))
#define CLAY_IS_TRANSFORM_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRANSFORM_TOOL))
#define CLAY_IS_TRANSFORM_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TRANSFORM_TOOL))
#define CLAY_TRANSFORM_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRANSFORM_TOOL, ClayTransformToolClass))

#define CLAY_TRANSFORM_TOOL_GET_OPTIONS(t)  (CLAY_TRANSFORM_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayTransformToolClass ClayTransformToolClass;

struct _ClayTransformTool
{
  ClayDrawTool       parent_instance;

  GList             *objects;            /*  List of ClayObject initially
                                             selected and set for
                                             transform processing.        */

  gint               x1, y1;             /*  upper left hand coordinate   */
  gint               x2, y2;             /*  lower right hand coords      */

  ClayMatrix3        transform;          /*  transformation matrix        */
  gboolean           transform_valid;    /*  whether the matrix is valid  */

  gboolean           restore_type;
  ClayTransformType  saved_type;
};

struct _ClayTransformToolClass
{
  ClayDrawToolClass  parent_class;

  /*  virtual functions  */
  void                     (* recalc_matrix) (ClayTransformTool  *tr_tool);
  gchar                  * (* get_undo_desc) (ClayTransformTool  *tr_tool);
  ClayTransformDirection   (* get_direction) (ClayTransformTool  *tr_tool);
  GeglBuffer             * (* transform)     (ClayTransformTool  *tr_tool,
                                              GList              *objects,
                                              GeglBuffer         *orig_buffer,
                                              gint                orig_offset_x,
                                              gint                orig_offset_y,
                                              ClayColorProfile  **buffer_profile,
                                              gint               *new_offset_x,
                                              gint               *new_offset_y);

  const gchar *undo_desc;
  const gchar *progress_text;
};


GType        clay_transform_tool_get_type            (void);

GList     * clay_transform_tool_get_selected_objects (ClayTransformTool  *tr_tool,
                                                      ClayDisplay        *display);
GList   * clay_transform_tool_check_selected_objects (ClayTransformTool  *tr_tool,
                                                      ClayDisplay        *display,
                                                      GError            **error);

gboolean     clay_transform_tool_bounds              (ClayTransformTool  *tr_tool,
                                                      ClayDisplay        *display);
void         clay_transform_tool_recalc_matrix       (ClayTransformTool  *tr_tool,
                                                      ClayDisplay        *display);

gboolean     clay_transform_tool_transform           (ClayTransformTool  *tr_tool,
                                                      ClayDisplay        *display);

void         clay_transform_tool_set_type            (ClayTransformTool  *tr_tool,
                                                      ClayTransformType   type);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayTransformTool, g_object_unref);
