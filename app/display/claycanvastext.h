/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvastext.h
 * Copyright (C) 2023 mr.fantastic <mrfantastic@firemail.cc>
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


#define CLAY_TYPE_CANVAS_TEXT (clay_canvas_text_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasText,
                          clay_canvas_text,
                          CLAY, CANVAS_TEXT,
                          ClayCanvasItem)


struct _ClayCanvasTextClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_text_new (ClayDisplayShell *shell,
                                       gdouble           x,
                                       gdouble           y,
                                       gdouble           font_size,
                                       gchar            *text);
