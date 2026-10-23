/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claylist.h
 * Copyright (C) 2001-2016 Michael Natterer <mitch@gimp.org>
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

#include "claycontainer.h"


#define CLAY_TYPE_LIST            (clay_list_get_type ())
#define CLAY_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LIST, ClayList))
#define CLAY_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LIST, ClayListClass))
#define CLAY_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LIST))
#define CLAY_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LIST))
#define CLAY_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LIST, ClayListClass))


typedef struct _ClayListClass ClayListClass;

struct _ClayList
{
  ClayContainer  parent_instance;

  GQueue        *queue;
  gboolean       unique_names;
  GCompareFunc   sort_func;
  gboolean       append;
};

struct _ClayListClass
{
  ClayContainerClass  parent_class;
};


GType           clay_list_get_type      (void);

ClayContainer * clay_list_new           (GType         child_type,
                                         gboolean      unique_names);
ClayContainer * clay_list_new_weak      (GType         child_type,
                                         gboolean      unique_names);

void            clay_list_reverse       (ClayList     *list);
void            clay_list_set_sort_func (ClayList     *list,
                                         GCompareFunc  sort_func);
GCompareFunc    clay_list_get_sort_func (ClayList     *list);
void            clay_list_sort          (ClayList     *list,
                                         GCompareFunc  sort_func);
void            clay_list_sort_by_name  (ClayList     *list);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayList, g_object_unref);
