/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationfillsource.h
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


#define CLAY_TYPE_OPERATION_FILL_SOURCE            (clay_operation_fill_source_get_type ())
#define CLAY_OPERATION_FILL_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_FILL_SOURCE, ClayOperationFillSource))
#define CLAY_OPERATION_FILL_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_FILL_SOURCE, ClayOperationFillSourceClass))
#define CLAY_IS_OPERATION_FILL_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_FILL_SOURCE))
#define CLAY_IS_OPERATION_FILL_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_FILL_SOURCE))
#define CLAY_OPERATION_FILL_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_FILL_SOURCE, ClayOperationFillSourceClass))


typedef struct _ClayOperationFillSource      ClayOperationFillSource;
typedef struct _ClayOperationFillSourceClass ClayOperationFillSourceClass;

struct _ClayOperationFillSource
{
  GeglOperationSource  parent_instance;

  ClayFillOptions     *options;
  ClayDrawable        *drawable;
  gint                 pattern_offset_x;
  gint                 pattern_offset_y;
};

struct _ClayOperationFillSourceClass
{
  GeglOperationSourceClass  parent_class;
};


GType   clay_operation_fill_source_get_type (void);
