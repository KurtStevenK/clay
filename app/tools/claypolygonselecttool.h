/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is polygon software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Polygon Software Foundation; either version 3 of the License, or
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


#define CLAY_TYPE_POLYGON_SELECT_TOOL            (clay_polygon_select_tool_get_type ())
#define CLAY_POLYGON_SELECT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_POLYGON_SELECT_TOOL, ClayPolygonSelectTool))
#define CLAY_POLYGON_SELECT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_POLYGON_SELECT_TOOL, ClayPolygonSelectToolClass))
#define CLAY_IS_POLYGON_SELECT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_POLYGON_SELECT_TOOL))
#define CLAY_IS_POLYGON_SELECT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_POLYGON_SELECT_TOOL))
#define CLAY_POLYGON_SELECT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_POLYGON_SELECT_TOOL, ClayPolygonSelectToolClass))


typedef struct _ClayPolygonSelectTool        ClayPolygonSelectTool;
typedef struct _ClayPolygonSelectToolPrivate ClayPolygonSelectToolPrivate;
typedef struct _ClayPolygonSelectToolClass   ClayPolygonSelectToolClass;

struct _ClayPolygonSelectTool
{
  ClaySelectionTool             parent_instance;

  ClayPolygonSelectToolPrivate *priv;
};

struct _ClayPolygonSelectToolClass
{
  ClaySelectionToolClass  parent_class;

  /*  virtual functions  */
  void (* change_complete) (ClayPolygonSelectTool *poly_sel,
                            ClayDisplay           *display);
  void (* confirm)         (ClayPolygonSelectTool *poly_sel,
                            ClayDisplay           *display);
};


GType      clay_polygon_select_tool_get_type   (void);

gboolean   clay_polygon_select_tool_is_closed  (ClayPolygonSelectTool  *poly_sel);
void       clay_polygon_select_tool_get_points (ClayPolygonSelectTool  *poly_sel,
                                                const ClayVector2     **points,
                                                gint                   *n_points);

/*  protected functions */
gboolean   clay_polygon_select_tool_is_grabbed (ClayPolygonSelectTool  *poly_sel);

void       clay_polygon_select_tool_halt       (ClayPolygonSelectTool  *poly_sel);

