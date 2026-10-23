/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationoverwrite.h
 * Copyright (C) 2025 Woynert <woynertgamer@gmail.com>
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


#define CLAY_TYPE_OPERATION_OVERWRITE            (clay_operation_overwrite_get_type ())
#define CLAY_OPERATION_OVERWRITE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_OVERWRITE, ClayOperationOverwrite))
#define CLAY_OPERATION_OVERWRITE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_OVERWRITE, ClayOperationOverwriteClass))
#define CLAY_IS_OPERATION_OVERWRITE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_OVERWRITE))
#define CLAY_IS_OPERATION_OVERWRITE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_OVERWRITE))
#define CLAY_OPERATION_OVERWRITE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_OVERWRITE, ClayOperationOverwriteClass))


typedef struct _ClayOperationOverwrite      ClayOperationOverwrite;
typedef struct _ClayOperationOverwriteClass ClayOperationOverwriteClass;

struct _ClayOperationOverwrite
{
  ClayOperationLayerMode  parent_instance;
};

struct _ClayOperationOverwriteClass
{
  ClayOperationLayerModeClass  parent_class;
};


GType   clay_operation_overwrite_get_type (void);
