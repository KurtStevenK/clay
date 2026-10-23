/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationprofiletransform.h
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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
#include <operation/gegl-operation-point-filter.h>


#define CLAY_TYPE_OPERATION_PROFILE_TRANSFORM            (clay_operation_profile_transform_get_type ())
#define CLAY_OPERATION_PROFILE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_PROFILE_TRANSFORM, ClayOperationProfileTransform))
#define CLAY_OPERATION_PROFILE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_PROFILE_TRANSFORM, ClayOperationProfileTransformClass))
#define CLAY_IS_OPERATION_PROFILE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_PROFILE_TRANSFORM))
#define CLAY_IS_OPERATION_PROFILE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_PROFILE_TRANSFORM))
#define CLAY_OPERATION_PROFILE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_PROFILE_TRANSFORM, ClayOperationProfileTransformClass))


typedef struct _ClayOperationProfileTransform      ClayOperationProfileTransform;
typedef struct _ClayOperationProfileTransformClass ClayOperationProfileTransformClass;

struct _ClayOperationProfileTransform
{
  GeglOperationPointFilter  parent_instance;

  ClayColorProfile         *src_profile;
  const Babl               *src_format;

  ClayColorProfile         *dest_profile;
  const Babl               *dest_format;

  ClayColorRenderingIntent  rendering_intent;
  gboolean                  black_point_compensation;

  ClayColorTransform       *transform;
};

struct _ClayOperationProfileTransformClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   clay_operation_profile_transform_get_type (void);
