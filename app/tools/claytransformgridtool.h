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

#include "claytransformtool.h"


/* This is not the number of items in the enum above, but the max size
 * of the enums at the top of each transformation tool, stored in
 * trans_info and related
 */
#define TRANS_INFO_SIZE 17

typedef gdouble TransInfo[TRANS_INFO_SIZE];


#define CLAY_TYPE_TRANSFORM_GRID_TOOL            (clay_transform_grid_tool_get_type ())
#define CLAY_TRANSFORM_GRID_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRANSFORM_GRID_TOOL, ClayTransformGridTool))
#define CLAY_TRANSFORM_GRID_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRANSFORM_GRID_TOOL, ClayTransformGridToolClass))
#define CLAY_IS_TRANSFORM_GRID_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRANSFORM_GRID_TOOL))
#define CLAY_IS_TRANSFORM_GRID_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TRANSFORM_GRID_TOOL))
#define CLAY_TRANSFORM_GRID_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRANSFORM_GRID_TOOL, ClayTransformGridToolClass))

#define CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS(t)  (CLAY_TRANSFORM_GRID_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayTransformGridToolClass ClayTransformGridToolClass;

struct _ClayTransformGridTool
{
  ClayTransformTool   parent_instance;

  TransInfo           init_trans_info;  /*  initial transformation info           */
  TransInfo           trans_infos[2];   /*  forward/backward transformation info  */
  gdouble            *trans_info;       /*  current transformation info           */
  GList              *undo_list;        /*  list of all states,
                                            head is current == prev_trans_info,
                                            tail is original == old_trans_info    */
  GList              *redo_list;        /*  list of all undone states,
                                            NULL when nothing undone */

  GList              *hidden_objects;   /*  the objects that was hidden during
                                            the transform                         */

  ClayToolWidget     *widget;
  ClayToolWidget     *grab_widget;
  GList              *previews;
  ClayCanvasItem     *boundary_in;
  ClayCanvasItem     *boundary_out;
  GPtrArray          *strokes;

  GHashTable         *filters;
  GList              *preview_drawables;

  ClayToolGui        *gui;
};

struct _ClayTransformGridToolClass
{
  ClayTransformToolClass  parent_class;

  /*  virtual functions  */
  gboolean         (* info_to_matrix) (ClayTransformGridTool  *tg_tool,
                                       ClayMatrix3            *transform);
  void             (* matrix_to_info) (ClayTransformGridTool  *tg_tool,
                                       const ClayMatrix3      *transform);
  void             (* apply_info)     (ClayTransformGridTool  *tg_tool,
                                       const TransInfo         info);
  gchar          * (* get_undo_desc)  (ClayTransformGridTool  *tg_tool);
  void             (* dialog)         (ClayTransformGridTool  *tg_tool);
  void             (* dialog_update)  (ClayTransformGridTool  *tg_tool);
  void             (* prepare)        (ClayTransformGridTool  *tg_tool);
  void             (* readjust)       (ClayTransformGridTool  *tg_tool);
  ClayToolWidget * (* get_widget)     (ClayTransformGridTool  *tg_tool);
  void             (* update_widget)  (ClayTransformGridTool  *tg_tool);
  void             (* widget_changed) (ClayTransformGridTool  *tg_tool);
  GeglBuffer     * (* transform)      (ClayTransformGridTool  *tg_tool,
                                       GList                  *objects,
                                       GeglBuffer             *orig_buffer,
                                       gint                    orig_offset_x,
                                       gint                    orig_offset_y,
                                       ClayColorProfile      **buffer_profile,
                                       gint                   *new_offset_x,
                                       gint                   *new_offset_y);

  const gchar *ok_button_label;
};


GType      clay_transform_grid_tool_get_type           (void);

gboolean   clay_transform_grid_tool_info_to_matrix     (ClayTransformGridTool *tg_tool,
                                                        ClayMatrix3           *transform);
void       clay_transform_grid_tool_matrix_to_info     (ClayTransformGridTool *tg_tool,
                                                        const ClayMatrix3     *transform);

void       clay_transform_grid_tool_push_internal_undo (ClayTransformGridTool *tg_tool,
                                                        gboolean               compress);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayTransformGridTool, g_object_unref);
