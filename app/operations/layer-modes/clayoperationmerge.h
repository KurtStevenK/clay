/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationmerge.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2017 Ell
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


#define CLAY_TYPE_OPERATION_MERGE            (clay_operation_merge_get_type ())
#define CLAY_OPERATION_MERGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_MERGE, ClayOperationMerge))
#define CLAY_OPERATION_MERGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_MERGE, ClayOperationMergeClass))
#define CLAY_IS_OPERATION_MERGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_MERGE))
#define CLAY_IS_OPERATION_MERGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_MERGE))
#define CLAY_OPERATION_MERGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_MERGE, ClayOperationMergeClass))


typedef struct _ClayOperationMerge      ClayOperationMerge;
typedef struct _ClayOperationMergeClass ClayOperationMergeClass;

struct _ClayOperationMerge
{
  ClayOperationLayerMode  parent_instance;
};

struct _ClayOperationMergeClass
{
  ClayOperationLayerModeClass  parent_class;
};


GType   clay_operation_merge_get_type (void);
