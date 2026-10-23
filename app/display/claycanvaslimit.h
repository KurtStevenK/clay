/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvaslimit.h
 * Copyright (C) 2020 Ell
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


#define CLAY_TYPE_CANVAS_LIMIT (clay_canvas_limit_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasLimit,
                          clay_canvas_limit,
                          CLAY, CANVAS_LIMIT,
                          ClayCanvasItem)


struct _ClayCanvasLimitClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_limit_new             (ClayDisplayShell *shell,
                                                    ClayLimitType     type,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble           radius,
                                                    gdouble           aspect_ratio,
                                                    gdouble           angle,
                                                    gboolean          dashed);

void             clay_canvas_limit_get_radii       (ClayCanvasLimit  *limit,
                                                    gdouble          *rx,
                                                    gdouble          *ry);

gboolean         clay_canvas_limit_is_inside       (ClayCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y);
void             clay_canvas_limit_boundary_point  (ClayCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *bx,
                                                    gdouble          *by);
gdouble          clay_canvas_limit_boundary_radius (ClayCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y);

void             clay_canvas_limit_center_point    (ClayCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *cx,
                                                    gdouble          *cy);
