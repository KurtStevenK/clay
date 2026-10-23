/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvastransformpreview.h
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


#define CLAY_TYPE_CANVAS_TRANSFORM_PREVIEW (clay_canvas_transform_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCanvasTransformPreview,
                          clay_canvas_transform_preview,
                          CLAY, CANVAS_TRANSFORM_PREVIEW,
                          ClayCanvasItem)


struct _ClayCanvasTransformPreviewClass
{
  ClayCanvasItemClass  parent_class;
};


ClayCanvasItem * clay_canvas_transform_preview_new (ClayDisplayShell  *shell,
                                                    ClayPickable      *pickable,
                                                    const ClayMatrix3 *transform,
                                                    gdouble            x1,
                                                    gdouble            y1,
                                                    gdouble            x2,
                                                    gdouble            y2);
