/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrenderergradient.h
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

#include "clayviewrenderer.h"


#define CLAY_TYPE_VIEW_RENDERER_GRADIENT            (clay_view_renderer_gradient_get_type ())
#define CLAY_VIEW_RENDERER_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEW_RENDERER_GRADIENT, ClayViewRendererGradient))
#define CLAY_VIEW_RENDERER_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEW_RENDERER_GRADIENT, ClayViewRendererGradientClass))
#define CLAY_IS_VIEW_RENDERER_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_VIEW_RENDERER_GRADIENT))
#define CLAY_IS_VIEW_RENDERER_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEW_RENDERER_GRADIENT))
#define CLAY_VIEW_RENDERER_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEW_RENDERER_GRADIENT, ClayViewRendererGradientClass))


typedef struct _ClayViewRendererGradientClass  ClayViewRendererGradientClass;

struct _ClayViewRendererGradient
{
  ClayViewRenderer            parent_instance;

  gdouble                     left;
  gdouble                     right;
  gboolean                    reverse;
  ClayGradientBlendColorSpace blend_color_space;
  gboolean                    has_fg_bg_segments;
};

struct _ClayViewRendererGradientClass
{
  ClayViewRendererClass  parent_class;
};


GType   clay_view_renderer_gradient_get_type    (void);

void    clay_view_renderer_gradient_set_offsets (ClayViewRendererGradient    *renderer,
                                                 gdouble                      left,
                                                 gdouble                      right);
void    clay_view_renderer_gradient_set_reverse (ClayViewRendererGradient    *renderer,
                                                 gboolean                     reverse);
void    clay_view_renderer_gradient_set_blend_color_space
                                                (ClayViewRendererGradient    *renderer,
                                                 ClayGradientBlendColorSpace  blend_color_space);
