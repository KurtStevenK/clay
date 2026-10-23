/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationequalize.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_OPERATION_EQUALIZE            (clay_operation_equalize_get_type ())
#define CLAY_OPERATION_EQUALIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_EQUALIZE, ClayOperationEqualize))
#define CLAY_OPERATION_EQUALIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_EQUALIZE, ClayOperationEqualizeClass))
#define CLAY_IS_OPERATION_EQUALIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_EQUALIZE))
#define CLAY_IS_OPERATION_EQUALIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_EQUALIZE))
#define CLAY_OPERATION_EQUALIZE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_EQUALIZE, ClayOperationEqualizeClass))


typedef struct _ClayOperationEqualize      ClayOperationEqualize;
typedef struct _ClayOperationEqualizeClass ClayOperationEqualizeClass;

struct _ClayOperationEqualize
{
  ClayOperationPointFilter  parent_instance;

  ClayHistogram            *histogram;
  gdouble                  *values;
  gint                      n_bins;
};

struct _ClayOperationEqualizeClass
{
  ClayOperationPointFilterClass  parent_class;
};


GType   clay_operation_equalize_get_type (void);
