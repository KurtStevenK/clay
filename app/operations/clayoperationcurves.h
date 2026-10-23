/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationcurves.h
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


#define CLAY_TYPE_OPERATION_CURVES            (clay_operation_curves_get_type ())
#define CLAY_OPERATION_CURVES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_CURVES, ClayOperationCurves))
#define CLAY_OPERATION_CURVES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_CURVES, ClayOperationCurvesClass))
#define CLAY_IS_OPERATION_CURVES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_CURVES))
#define CLAY_IS_OPERATION_CURVES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_CURVES))
#define CLAY_OPERATION_CURVES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_CURVES, ClayOperationCurvesClass))


typedef struct _ClayOperationCurves      ClayOperationCurves;
typedef struct _ClayOperationCurvesClass ClayOperationCurvesClass;

struct _ClayOperationCurves
{
  ClayOperationPointFilter  parent_instance;
};

struct _ClayOperationCurvesClass
{
  ClayOperationPointFilterClass  parent_class;
};


GType   clay_operation_curves_get_type (void);
