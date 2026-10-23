/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytagcache.h
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

#include "clayobject.h"


#define CLAY_TYPE_TAG_CACHE            (clay_tag_cache_get_type ())
#define CLAY_TAG_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TAG_CACHE, ClayTagCache))
#define CLAY_TAG_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TAG_CACHE, ClayTagCacheClass))
#define CLAY_IS_TAG_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TAG_CACHE))
#define CLAY_IS_TAG_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TAG_CACHE))
#define CLAY_TAG_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TAG_CACHE, ClayTagCacheClass))


typedef struct _ClayTagCacheClass   ClayTagCacheClass;
typedef struct _ClayTagCachePrivate ClayTagCachePrivate;

struct _ClayTagCache
{
  ClayObject           parent_instance;

  ClayTagCachePrivate *priv;
};

struct _ClayTagCacheClass
{
  ClayObjectClass  parent_class;
};


GType           clay_tag_cache_get_type      (void);

ClayTagCache *  clay_tag_cache_new           (void);

void            clay_tag_cache_save          (ClayTagCache  *cache);
void            clay_tag_cache_load          (ClayTagCache  *cache);

void            clay_tag_cache_add_container (ClayTagCache  *cache,
                                              ClayContainer *container);
