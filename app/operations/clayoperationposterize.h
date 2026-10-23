/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationposterize.h
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


#define CLAY_TYPE_OPERATION_POSTERIZE            (clay_operation_posterize_get_type ())
#define CLAY_OPERATION_POSTERIZE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_POSTERIZE, ClayOperationPosterize))
#define CLAY_OPERATION_POSTERIZE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_POSTERIZE, ClayOperationPosterizeClass))
#define CLAY_IS_OPERATION_POSTERIZE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_POSTERIZE))
#define CLAY_IS_OPERATION_POSTERIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_POSTERIZE))
#define CLAY_OPERATION_POSTERIZE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_POSTERIZE, ClayOperationPosterizeClass))


typedef struct _ClayOperationPosterize      ClayOperationPosterize;
typedef struct _ClayOperationPosterizeClass ClayOperationPosterizeClass;

struct _ClayOperationPosterize
{
  ClayOperationPointFilter  parent_instance;

  gint                      levels;
};

struct _ClayOperationPosterizeClass
{
  ClayOperationPointFilterClass  parent_class;
};


GType   clay_operation_posterize_get_type (void);
