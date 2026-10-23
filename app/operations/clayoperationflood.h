/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationflood.h
 * Copyright (C) 2016 Ell
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


#define CLAY_TYPE_OPERATION_FLOOD            (clay_operation_flood_get_type ())
#define CLAY_OPERATION_FLOOD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_FLOOD, ClayOperationFlood))
#define CLAY_OPERATION_FLOOD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_FLOOD, ClayOperationFloodClass))
#define CLAY_IS_OPERATION_FLOOD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_FLOOD))
#define CLAY_IS_OPERATION_FLOOD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_FLOOD))
#define CLAY_OPERATION_FLOOD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_FLOOD, ClayOperationFloodClass))


typedef struct _ClayOperationFlood      ClayOperationFlood;
typedef struct _ClayOperationFloodClass ClayOperationFloodClass;

struct _ClayOperationFlood
{
  GeglOperationFilter  parent_instance;
};

struct _ClayOperationFloodClass
{
  GeglOperationFilterClass  parent_class;
};


GType   clay_operation_flood_get_type (void);
