/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationdissolve.h
 * Copyright (C) 2012 Ville Sokk <ville.sokk@gmail.com>
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

#include "clayoperationlayermode.h"


#define CLAY_TYPE_OPERATION_DISSOLVE            (clay_operation_dissolve_get_type ())
#define CLAY_OPERATION_DISSOLVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_DISSOLVE, ClayOperationDissolve))
#define CLAY_OPERATION_DISSOLVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_DISSOLVE, ClayOperationDissolveClass))
#define CLAY_IS_OPERATION_DISSOLVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_DISSOLVE))
#define CLAY_IS_OPERATION_DISSOLVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_DISSOLVE))
#define CLAY_OPERATION_DISSOLVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_DISSOLVE, ClayOperationDissolveClass))


typedef struct _ClayOperationDissolve      ClayOperationDissolve;
typedef struct _ClayOperationDissolveClass ClayOperationDissolveClass;

struct _ClayOperationDissolveClass
{
  ClayOperationLayerModeClass parent_class;
};

struct _ClayOperationDissolve
{
  ClayOperationLayerMode parent_instance;
};


GType   clay_operation_dissolve_get_type (void);
