/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayfilterstack.h
 * Copyright (C) 2008-2013 Michael Natterer <mitch@gimp.org>
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

#include "claylist.h"


#define CLAY_TYPE_FILTER_STACK            (clay_filter_stack_get_type ())
#define CLAY_FILTER_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FILTER_STACK, ClayFilterStack))
#define CLAY_FILTER_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FILTER_STACK, ClayFilterStackClass))
#define CLAY_IS_FILTER_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FILTER_STACK))
#define CLAY_IS_FILTER_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FILTER_STACK))


typedef struct _ClayFilterStackClass ClayFilterStackClass;

struct _ClayFilterStack
{
  ClayList  parent_instance;

  GeglNode *graph;
};

struct _ClayFilterStackClass
{
  ClayListClass  parent_class;
};


GType           clay_filter_stack_get_type  (void);
ClayContainer * clay_filter_stack_new       (GType            filter_type);

GeglNode *      clay_filter_stack_get_graph (ClayFilterStack *stack);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayFilterStack, g_object_unref);
