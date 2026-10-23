/* xyz.kainzmayer.ClayDesign
 *
 * claynpointdeformationtool.h
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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
#include <npd/npd_common.h>


#define CLAY_TYPE_N_POINT_DEFORMATION_TOOL            (clay_n_point_deformation_tool_get_type ())
#define CLAY_N_POINT_DEFORMATION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_N_POINT_DEFORMATION_TOOL, ClayNPointDeformationTool))
#define CLAY_N_POINT_DEFORMATION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_N_POINT_DEFORMATION_TOOL, ClayNPointDeformationToolClass))
#define CLAY_IS_N_POINT_DEFORMATION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_N_POINT_DEFORMATION_TOOL))
#define CLAY_IS_N_POINT_DEFORMATION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_N_POINT_DEFORMATION_TOOL))
#define CLAY_N_POINT_DEFORMATION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_N_POINT_DEFORMATION_TOOL, ClayNPointDeformationToolClass))

#define CLAY_N_POINT_DEFORMATION_TOOL_GET_OPTIONS(t)  (CLAY_N_POINT_DEFORMATION_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayNPointDeformationTool      ClayNPointDeformationTool;
typedef struct _ClayNPointDeformationToolClass ClayNPointDeformationToolClass;

struct _ClayNPointDeformationTool
{
  ClayDrawTool      parent_instance;

  guint             draw_timeout_id;
  GThread          *deform_thread;

  GeglNode         *graph;
  GeglNode         *source;
  GeglNode         *npd_node;
  GeglNode         *sink;

  GeglBuffer       *preview_buffer;

  NPDModel         *model;
  NPDControlPoint  *selected_cp;    /* last selected control point     */
  GList            *selected_cps;   /* list of selected control points */
  NPDControlPoint  *hovering_cp;

  ClayVector2      *lattice_points;

  gdouble           start_x;
  gdouble           start_y;

  gdouble           last_x;
  gdouble           last_y;

  gdouble           cursor_x;
  gdouble           cursor_y;

  gint              offset_x;
  gint              offset_y;

  gfloat            cp_scaled_radius;  /* radius of a control point scaled
                                        * according to display shell's scale
                                        */

  gboolean          active;
  volatile gboolean deformation_active;
  gboolean          rubber_band;
};

struct _ClayNPointDeformationToolClass
{
  ClayDrawToolClass parent_class;
};

void    clay_n_point_deformation_tool_register (ClayToolRegisterCallback  callback,
                                                gpointer                  data);

GType   clay_n_point_deformation_tool_get_type (void);
