/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvassamplepoint.h
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


#define CLAY_TYPE_CANVAS_SAMPLE_POINT (clay_canvas_sample_point_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasSamplePoint,
                          clay_canvas_sample_point,
                          CLAY, CANVAS_SAMPLE_POINT,
                          ClayCanvasItem)


struct _ClayCanvasSamplePointClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_sample_point_new (ClayDisplayShell *shell,
                                               gint              x,
                                               gint              y,
                                               gint              index,
                                               gboolean          sample_point_style);

void             clay_canvas_sample_point_set (ClayCanvasItem   *sample_point,
                                               gint              x,
                                               gint              y);
