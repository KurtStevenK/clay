/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationpointfilter.h
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

#include <gegl-plugin.h>
#include <operation/gegl-operation-point-filter.h>


enum
{
  CLAY_OPERATION_POINT_FILTER_PROP_0,
  CLAY_OPERATION_POINT_FILTER_PROP_TRC,
  CLAY_OPERATION_POINT_FILTER_PROP_CONFIG
};


#define CLAY_TYPE_OPERATION_POINT_FILTER            (clay_operation_point_filter_get_type ())
#define CLAY_OPERATION_POINT_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_POINT_FILTER, ClayOperationPointFilter))
#define CLAY_OPERATION_POINT_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_POINT_FILTER, ClayOperationPointFilterClass))
#define CLAY_IS_OPERATION_POINT_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_POINT_FILTER))
#define CLAY_IS_OPERATION_POINT_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_POINT_FILTER))
#define CLAY_OPERATION_POINT_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_POINT_FILTER, ClayOperationPointFilterClass))


typedef struct _ClayOperationPointFilterClass ClayOperationPointFilterClass;

struct _ClayOperationPointFilter
{
  GeglOperationPointFilter  parent_instance;

  ClayTRCType               trc;
  GObject                  *config;

  GBinding                 *trc_binding;
};

struct _ClayOperationPointFilterClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   clay_operation_point_filter_get_type     (void);

void    clay_operation_point_filter_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
void    clay_operation_point_filter_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
