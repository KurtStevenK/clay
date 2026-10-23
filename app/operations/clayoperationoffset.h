/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationoffset.h
 * Copyright (C) 2019 Ell
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


#define CLAY_TYPE_OPERATION_OFFSET            (clay_operation_offset_get_type ())
#define CLAY_OPERATION_OFFSET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_OFFSET, ClayOperationOffset))
#define CLAY_OPERATION_OFFSET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_OFFSET, ClayOperationOffsetClass))
#define CLAY_IS_OPERATION_OFFSET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_OFFSET))
#define CLAY_IS_OPERATION_OFFSET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_OFFSET))
#define CLAY_OPERATION_OFFSET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_OFFSET, ClayOperationOffsetClass))


typedef struct _ClayOperationOffset      ClayOperationOffset;
typedef struct _ClayOperationOffsetClass ClayOperationOffsetClass;

struct _ClayOperationOffset
{
  GeglOperationFilter  parent_instance;

  GeglColor           *color;
  ClayOffsetType       type;
  gint                 x;
  gint                 y;
};

struct _ClayOperationOffsetClass
{
  GeglOperationFilterClass  parent_class;
};


GType   clay_operation_offset_get_type (void);
