/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasrectangleguides.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CANVAS_RECTANGLE_GUIDES (clay_canvas_rectangle_guides_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasRectangleGuides,
                          clay_canvas_rectangle_guides,
                          CLAY, CANVAS_RECTANGLE_GUIDES,
                          ClayCanvasItem)


struct _ClayCanvasRectangleGuidesClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_rectangle_guides_new (ClayDisplayShell *shell,
                                                   gdouble           x,
                                                   gdouble           y,
                                                   gdouble           width,
                                                   gdouble           height,
                                                   ClayGuidesType    type,
                                                   gint              n_guides);

void             clay_canvas_rectangle_guides_set (ClayCanvasItem   *rectangle,
                                                   gdouble           x,
                                                   gdouble           y,
                                                   gdouble           width,
                                                   gdouble           height,
                                                   ClayGuidesType    type,
                                                   gint              n_guides);
