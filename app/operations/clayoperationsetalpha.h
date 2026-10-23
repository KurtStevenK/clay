/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationsetalpha.h
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

#include <gegl-plugin.h>


#define CLAY_TYPE_OPERATION_SET_ALPHA            (clay_operation_set_alpha_get_type ())
#define CLAY_OPERATION_SET_ALPHA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_SET_ALPHA, ClayOperationSetAlpha))
#define CLAY_OPERATION_SET_ALPHA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_SET_ALPHA, ClayOperationSetAlphaClass))
#define CLAY_IS_OPERATION_SET_ALPHA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_SET_ALPHA))
#define CLAY_IS_OPERATION_SET_ALPHA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_SET_ALPHA))
#define CLAY_OPERATION_SET_ALPHA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_SET_ALPHA, ClayOperationSetAlphaClass))


typedef struct _ClayOperationSetAlpha      ClayOperationSetAlpha;
typedef struct _ClayOperationSetAlphaClass ClayOperationSetAlphaClass;

struct _ClayOperationSetAlpha
{
  GeglOperationPointComposer  parent_instance;

  gdouble                     value;
};

struct _ClayOperationSetAlphaClass
{
  GeglOperationPointComposerClass  parent_class;
};


GType   clay_operation_set_alpha_get_type (void);
