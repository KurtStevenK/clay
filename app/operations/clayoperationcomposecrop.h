/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationcomposecrop.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2016 Massimo Valentini <mvalentini@src.gnome.org>
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


#define CLAY_TYPE_OPERATION_COMPOSE_CROP            (clay_operation_compose_crop_get_type ())
#define CLAY_OPERATION_COMPOSE_CROP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_COMPOSE_CROP, ClayOperationComposeCrop))
#define CLAY_OPERATION_COMPOSE_CROP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_COMPOSE_CROP, ClayOperationComposeCropClass))
#define CLAY_IS_OPERATION_COMPOSE_CROP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_COMPOSE_CROP))
#define CLAY_IS_OPERATION_COMPOSE_CROP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_COMPOSE_CROP))
#define CLAY_OPERATION_COMPOSE_CROP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_COMPOSE_CROP, ClayOperationComposeCropClass))


typedef struct _ClayOperationComposeCrop      ClayOperationComposeCrop;
typedef struct _ClayOperationComposeCropClass ClayOperationComposeCropClass;

struct _ClayOperationComposeCrop
{
  GeglOperationPointComposer  parent_instance;

  GeglRectangle               rect;
};

struct _ClayOperationComposeCropClass
{
  GeglOperationPointComposerClass  parent_class;
};


GType   clay_operation_compose_crop_get_type (void);
