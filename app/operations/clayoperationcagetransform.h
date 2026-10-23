/* xyz.kainzmayer.ClayDesign
 *
 * clayoperationcagetransform.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
#include <operation/gegl-operation-composer.h>


#define CLAY_TYPE_OPERATION_CAGE_TRANSFORM            (clay_operation_cage_transform_get_type ())
#define CLAY_OPERATION_CAGE_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_CAGE_TRANSFORM, ClayOperationCageTransform))
#define CLAY_OPERATION_CAGE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_CAGE_TRANSFORM, ClayOperationCageTransformClass))
#define CLAY_IS_OPERATION_CAGE_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_CAGE_TRANSFORM))
#define CLAY_IS_OPERATION_CAGE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_CAGE_TRANSFORM))
#define CLAY_OPERATION_CAGE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_CAGE_TRANSFORM, ClayOperationCageTransformClass))


typedef struct _ClayOperationCageTransform      ClayOperationCageTransform;
typedef struct _ClayOperationCageTransformClass ClayOperationCageTransformClass;

struct _ClayOperationCageTransform
{
  GeglOperationComposer  parent_instance;

  ClayCageConfig        *config;
  gboolean               fill_plain_color;

  const Babl            *format_coords;
};

struct _ClayOperationCageTransformClass
{
  GeglOperationComposerClass  parent_class;
};


GType   clay_operation_cage_transform_get_type (void);
