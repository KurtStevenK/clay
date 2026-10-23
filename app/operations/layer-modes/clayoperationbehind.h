/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationbehind.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_OPERATION_BEHIND            (clay_operation_behind_get_type ())
#define CLAY_OPERATION_BEHIND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_BEHIND, ClayOperationBehind))
#define CLAY_OPERATION_BEHIND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_BEHIND, ClayOperationBehindClass))
#define CLAY_IS_OPERATION_BEHIND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_BEHIND))
#define CLAY_IS_OPERATION_BEHIND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_BEHIND))
#define CLAY_OPERATION_BEHIND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_BEHIND, ClayOperationBehindClass))


typedef struct _ClayOperationBehind      ClayOperationBehind;
typedef struct _ClayOperationBehindClass ClayOperationBehindClass;

struct _ClayOperationBehind
{
  ClayOperationLayerMode  parent_instance;
};

struct _ClayOperationBehindClass
{
  ClayOperationLayerModeClass  parent_class;
};


GType    clay_operation_behind_get_type (void);
