/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayfilteredcontainer.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *               2011 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_FILTERED_CONTAINER            (clay_filtered_container_get_type ())
#define CLAY_FILTERED_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FILTERED_CONTAINER, ClayFilteredContainer))
#define CLAY_FILTERED_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FILTERED_CONTAINER, ClayFilteredContainerClass))
#define CLAY_IS_FILTERED_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FILTERED_CONTAINER))
#define CLAY_IS_FILTERED_CONTAINER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), CLAY_TYPE_FILTERED_CONTAINER))
#define CLAY_FILTERED_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FILTERED_CONTAINER, ClayFilteredContainerClass))


typedef struct _ClayFilteredContainerClass ClayFilteredContainerClass;

struct _ClayFilteredContainer
{
  ClayList              parent_instance;

  ClayContainer        *src_container;
  ClayObjectFilterFunc  filter_func;
  gpointer              filter_data;
};

struct _ClayFilteredContainerClass
{
  ClayContainerClass  parent_class;

  void (* src_add)    (ClayFilteredContainer *filtered_container,
                       ClayObject            *object);
  void (* src_remove) (ClayFilteredContainer *filtered_container,
                       ClayObject            *object);
  void (* src_freeze) (ClayFilteredContainer *filtered_container);
  void (* src_thaw)   (ClayFilteredContainer *filtered_container);
};


GType           clay_filtered_container_get_type (void);

ClayContainer * clay_filtered_container_new      (ClayContainer        *src_container,
                                                  ClayObjectFilterFunc  filter_func,
                                                  gpointer              filter_data);
