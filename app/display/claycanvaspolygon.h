/* xyz.kainzmayer.ClayDesign Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
 *
 * claycanvaspolygon.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "claycanvasitem.h"


#define CLAY_TYPE_CANVAS_POLYGON (clay_canvas_polygon_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasPolygon,
                          clay_canvas_polygon,
                          CLAY, CANVAS_POLYGON,
                          ClayCanvasItem)


struct _ClayCanvasPolygonClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_polygon_new             (ClayDisplayShell  *shell,
                                                      const ClayVector2 *points,
                                                      gint               n_points,
                                                      ClayMatrix3       *transform,
                                                      gboolean           filled);
ClayCanvasItem * clay_canvas_polygon_new_from_coords (ClayDisplayShell  *shell,
                                                      const ClayCoords  *coords,
                                                      gint               n_coords,
                                                      ClayMatrix3       *transform,
                                                      gboolean           filled);

void             clay_canvas_polygon_set_points      (ClayCanvasItem    *polygon,
                                                      const ClayVector2 *points,
                                                      gint               n_points);
