/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claytaggedcontainer.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "clayfilteredcontainer.h"


#define CLAY_TYPE_TAGGED_CONTAINER            (clay_tagged_container_get_type ())
#define CLAY_TAGGED_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TAGGED_CONTAINER, ClayTaggedContainer))
#define CLAY_TAGGED_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TAGGED_CONTAINER, ClayTaggedContainerClass))
#define CLAY_IS_TAGGED_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TAGGED_CONTAINER))
#define CLAY_IS_TAGGED_CONTAINER_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), CLAY_TYPE_TAGGED_CONTAINER))
#define CLAY_TAGGED_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TAGGED_CONTAINER, ClayTaggedContainerClass))


typedef struct _ClayTaggedContainerClass ClayTaggedContainerClass;

struct _ClayTaggedContainer
{
  ClayFilteredContainer  parent_instance;

  GList                 *filter;
  GHashTable            *tag_ref_counts;
  gint                   tag_count;
};

struct _ClayTaggedContainerClass
{
  ClayFilteredContainerClass  parent_class;

  void (* tag_count_changed) (ClayTaggedContainer *container,
                              gint                 count);
};


GType           clay_tagged_container_get_type      (void);

ClayContainer * clay_tagged_container_new           (ClayContainer       *src_container);

void            clay_tagged_container_set_filter    (ClayTaggedContainer *tagged_container,
                                                     GList               *tags);
const GList   * clay_tagged_container_get_filter    (ClayTaggedContainer *tagged_container);

gint            clay_tagged_container_get_tag_count (ClayTaggedContainer *container);
