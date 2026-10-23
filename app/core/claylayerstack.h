/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claylayerstack.h
 * Copyright (C) 2017 Ell
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

#include "claydrawablestack.h"


#define CLAY_TYPE_LAYER_STACK            (clay_layer_stack_get_type ())
#define CLAY_LAYER_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LAYER_STACK, ClayLayerStack))
#define CLAY_LAYER_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LAYER_STACK, ClayLayerStackClass))
#define CLAY_IS_LAYER_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LAYER_STACK))
#define CLAY_IS_LAYER_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LAYER_STACK))


typedef struct _ClayLayerStackClass ClayLayerStackClass;

struct _ClayLayerStack
{
  ClayDrawableStack  parent_instance;
};

struct _ClayLayerStackClass
{
  ClayDrawableStackClass  parent_class;
};


GType           clay_layer_stack_get_type  (void);
ClayContainer * clay_layer_stack_new       (GType layer_type);
