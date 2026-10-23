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

#ifndef  __CLAY_GRADIENT_TOOL_H__
#define  __CLAY_GRADIENT_TOOL_H__


#include "claydrawtool.h"


#define CLAY_TYPE_GRADIENT_TOOL            (clay_gradient_tool_get_type ())
#define CLAY_GRADIENT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GRADIENT_TOOL, ClayGradientTool))
#define CLAY_GRADIENT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GRADIENT_TOOL, ClayGradientToolClass))
#define CLAY_IS_GRADIENT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GRADIENT_TOOL))
#define CLAY_IS_GRADIENT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GRADIENT_TOOL))
#define CLAY_GRADIENT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GRADIENT_TOOL, ClayGradientToolClass))

#define CLAY_GRADIENT_TOOL_GET_OPTIONS(t)  (CLAY_GRADIENT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayGradientTool      ClayGradientTool;
typedef struct _ClayGradientToolClass ClayGradientToolClass;

struct _ClayGradientTool
{
  ClayDrawTool        parent_instance;

  ClayGradient       *gradient;
  ClayGradient       *tentative_gradient;

  gdouble             start_x;    /*  starting x coord  */
  gdouble             start_y;    /*  starting y coord  */
  gdouble             end_x;      /*  ending x coord    */
  gdouble             end_y;      /*  ending y coord    */

  ClayToolWidget     *widget;
  ClayToolWidget     *grab_widget;

  GeglNode           *graph;
  GeglNode           *render_node;
#if 0
  GeglNode           *subtract_node;
  GeglNode           *divide_node;
#endif
  GeglNode           *dist_node;
  GeglBuffer         *dist_buffer;
  ClayDrawableFilter *filter;

  ClayDrawableFilter *existing_filter;

  /*  editor  */

  gint                block_handlers_count;

  gint                edit_count;
  GSList             *undo_stack;
  GSList             *redo_stack;

  guint               flush_idle_id;

  ClayToolGui        *gui;
  GtkWidget          *endpoint_editor;
  GtkWidget          *endpoint_se;
  GtkWidget          *endpoint_color_panel;
  GtkWidget          *endpoint_type_combo;
  GtkWidget          *stop_editor;
  GtkWidget          *stop_se;
  GtkWidget          *stop_left_color_panel;
  GtkWidget          *stop_left_type_combo;
  GtkWidget          *stop_right_color_panel;
  GtkWidget          *stop_right_type_combo;
  GtkWidget          *stop_chain_button;
  GtkWidget          *midpoint_editor;
  GtkWidget          *midpoint_se;
  GtkWidget          *midpoint_type_combo;
  GtkWidget          *midpoint_color_combo;
  GtkWidget          *midpoint_new_stop_button;
  GtkWidget          *midpoint_center_button;
};

struct _ClayGradientToolClass
{
  ClayDrawToolClass  parent_class;
};


void    clay_gradient_tool_register               (ClayToolRegisterCallback  callback,
                                                   gpointer                  data);

GType   clay_gradient_tool_get_type               (void);

void    clay_gradient_tool_set_existing_filter    (ClayGradientTool         *gradient_tool,
                                                   ClayDisplay              *display,
                                                   gint                      drawable_id,
                                                   gint                      filter_index);

/*  protected functions  */

void    clay_gradient_tool_set_tentative_gradient (ClayGradientTool         *gradient_tool,
                                                   ClayGradient             *gradient);


#endif  /*  __CLAY_GRADIENT_TOOL_H__  */
