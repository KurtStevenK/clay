/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvashandle.h
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


#define CLAY_CANVAS_HANDLE_SIZE_CIRCLE    13
#define CLAY_CANVAS_HANDLE_SIZE_CROSS     15
#define CLAY_CANVAS_HANDLE_SIZE_CROSSHAIR 43
#define CLAY_CANVAS_HANDLE_SIZE_LARGE     25
#define CLAY_CANVAS_HANDLE_SIZE_SMALL     7


#define CLAY_TYPE_CANVAS_HANDLE (clay_canvas_handle_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasHandle,
                          clay_canvas_handle,
                          CLAY, CANVAS_HANDLE,
                          ClayCanvasItem)


struct _ClayCanvasHandleClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_handle_new          (ClayDisplayShell *shell,
                                                  ClayHandleType    type,
                                                  ClayHandleAnchor  anchor,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  gint              width,
                                                  gint              height);

void             clay_canvas_handle_get_position (ClayCanvasItem   *handle,
                                                  gdouble          *x,
                                                  gdouble          *y);
void             clay_canvas_handle_set_position (ClayCanvasItem   *handle,
                                                  gdouble           x,
                                                  gdouble           y);

gint             clay_canvas_handle_calc_size    (ClayCanvasItem   *item,
                                                  gdouble           mouse_x,
                                                  gdouble           mouse_y,
                                                  gint              normal_size,
                                                  gint              hover_size);

void             clay_canvas_handle_get_size     (ClayCanvasItem   *handle,
                                                  gint             *width,
                                                  gint             *height);
void             clay_canvas_handle_set_size     (ClayCanvasItem   *handle,
                                                  gint              width,
                                                  gint              height);

void             clay_canvas_handle_set_angles   (ClayCanvasItem   *handle,
                                                  gdouble           start_handle,
                                                  gdouble           slice_handle);
