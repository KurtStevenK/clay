/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrushcache.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_BRUSH_CACHE            (clay_brush_cache_get_type ())
#define CLAY_BRUSH_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_CACHE, ClayBrushCache))
#define CLAY_BRUSH_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_CACHE, ClayBrushCacheClass))
#define CLAY_IS_BRUSH_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_CACHE))
#define CLAY_IS_BRUSH_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_CACHE))
#define CLAY_BRUSH_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_CACHE, ClayBrushCacheClass))


typedef struct _ClayBrushCacheClass ClayBrushCacheClass;

struct _ClayBrushCache
{
  ClayObject      parent_instance;

  GDestroyNotify  data_destroy;

  GList          *cached_units;

  gchar           debug_hit;
  gchar           debug_miss;
};

struct _ClayBrushCacheClass
{
  ClayObjectClass  parent_class;
};


GType            clay_brush_cache_get_type (void);

ClayBrushCache * clay_brush_cache_new      (GDestroyNotify  data_destory,
                                            gchar           debug_hit,
                                            gchar           debug_miss);

void             clay_brush_cache_clear    (ClayBrushCache *cache);

gconstpointer    clay_brush_cache_get      (ClayBrushCache *cache,
                                            gint            width,
                                            gint            height,
                                            gdouble         scale,
                                            gdouble         aspect_ratio,
                                            gdouble         angle,
                                            gboolean        reflect,
                                            gdouble         hardness);
void             clay_brush_cache_add      (ClayBrushCache *cache,
                                            gpointer        data,
                                            gint            width,
                                            gint            height,
                                            gdouble         scale,
                                            gdouble         aspect_ratio,
                                            gdouble         angle,
                                            gboolean        reflect,
                                            gdouble         hardness);
