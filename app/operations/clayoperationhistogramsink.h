/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationhistogramsink.h
 * Copyright (C) 2012 Øyvind Kolås
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
#include <operation/gegl-operation-sink.h>


#define CLAY_TYPE_OPERATION_HISTOGRAM_SINK            (clay_operation_histogram_sink_get_type ())
#define CLAY_OPERATION_HISTOGRAM_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_HISTOGRAM_SINK, ClayOperationHistogramSink))
#define CLAY_OPERATION_HISTOGRAM_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_HISTOGRAM_SINK, ClayOperationHistogramSinkClass))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_HISTOGRAM_SINK))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_HISTOGRAM_SINK))
#define CLAY_OPERATION_HISTOGRAM_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_HISTOGRAM_SINK, ClayOperationHistogramSinkClass))


typedef struct _ClayOperationHistogramSink      ClayOperationHistogramSink;
typedef struct _ClayOperationHistogramSinkClass ClayOperationHistogramSinkClass;

struct _ClayOperationHistogramSink
{
  GeglOperation  parent_instance;

  ClayHistogram *histogram;
};

struct _ClayOperationHistogramSinkClass
{
  GeglOperationSinkClass  parent_class;
};


GType   clay_operation_histogram_sink_get_type (void);

