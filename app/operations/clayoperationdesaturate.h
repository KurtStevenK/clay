/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationdesaturate.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "clayoperationpointfilter.h"


#define CLAY_TYPE_OPERATION_DESATURATE            (clay_operation_desaturate_get_type ())
#define CLAY_OPERATION_DESATURATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_DESATURATE, ClayOperationDesaturate))
#define CLAY_OPERATION_DESATURATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_DESATURATE, ClayOperationDesaturateClass))
#define CLAY_IS_OPERATION_DESATURATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_DESATURATE))
#define CLAY_IS_OPERATION_DESATURATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_DESATURATE))
#define CLAY_OPERATION_DESATURATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_DESATURATE, ClayOperationDesaturateClass))


typedef struct _ClayOperationDesaturate      ClayOperationDesaturate;
typedef struct _ClayOperationDesaturateClass ClayOperationDesaturateClass;

struct _ClayOperationDesaturate
{
  ClayOperationPointFilter  parent_instance;

  ClayDesaturateMode        mode;
};

struct _ClayOperationDesaturateClass
{
  ClayOperationPointFilterClass  parent_class;
};


GType   clay_operation_desaturate_get_type (void);
