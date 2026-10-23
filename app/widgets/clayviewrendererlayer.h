/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererlayer.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "clayviewrendererdrawable.h"


#define CLAY_TYPE_VIEW_RENDERER_LAYER (clay_view_renderer_layer_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayViewRendererLayer,
                          clay_view_renderer_layer,
                          CLAY, VIEW_RENDERER_LAYER,
                          ClayViewRendererDrawable)


struct _ClayViewRendererLayerClass
{
  ClayViewRendererDrawableClass  parent_class;
};
