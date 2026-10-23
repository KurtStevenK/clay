/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationgradient.h
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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

#include <gegl-plugin.h>


#define CLAY_TYPE_OPERATION_GRADIENT            (clay_operation_gradient_get_type ())
#define CLAY_OPERATION_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_GRADIENT, ClayOperationGradient))
#define CLAY_OPERATION_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_GRADIENT, ClayOperationGradientClass))
#define CLAY_IS_OPERATION_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_GRADIENT))
#define CLAY_IS_OPERATION_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_GRADIENT))
#define CLAY_OPERATION_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_GRADIENT, ClayOperationGradientClass))


typedef struct _ClayOperationGradient      ClayOperationGradient;
typedef struct _ClayOperationGradientClass ClayOperationGradientClass;

struct _ClayOperationGradient
{
  GeglOperationFilter          parent_instance;

  ClayContext                 *context;

  ClayGradient                *gradient;
  gdouble                      start_x, start_y, end_x, end_y;
  ClayGradientType             gradient_type;
  ClayRepeatMode               gradient_repeat;
  gdouble                      offset;
  gboolean                     gradient_reverse;
  ClayGradientBlendColorSpace  gradient_blend_color_space;

  gboolean                     supersample;
  gint                         supersample_depth;
  gdouble                      supersample_threshold;

  gboolean                     dither;

  gdouble                     *gradient_cache;
  gint                         gradient_cache_size;
  GMutex                       gradient_cache_mutex;
};

struct _ClayOperationGradientClass
{
  GeglOperationFilterClass  parent_class;
};


GType   clay_operation_gradient_get_type (void);

