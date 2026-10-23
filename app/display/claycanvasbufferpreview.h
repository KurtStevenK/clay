/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasbufferpreview.h
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

#include "claycanvasitem.h"


#define CLAY_TYPE_CANVAS_BUFFER_PREVIEW (clay_canvas_buffer_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasBufferPreview,
                          clay_canvas_buffer_preview,
                          CLAY, CANVAS_BUFFER_PREVIEW,
                          ClayCanvasItem)


struct _ClayCanvasBufferPreviewClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_buffer_preview_new (ClayDisplayShell  *shell,
                                                 GeglBuffer        *buffer);
