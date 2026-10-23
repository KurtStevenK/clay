/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_MEMSIZE_H__
#define __CLAY_MEMSIZE_H__

G_BEGIN_DECLS


/**
 * CLAY_TYPE_MEMSIZE:
 *
 * #CLAY_TYPE_MEMSIZE is a #GType derived from #G_TYPE_UINT64.
 **/

#define CLAY_TYPE_MEMSIZE               (clay_memsize_get_type ())
#define CLAY_VALUE_HOLDS_MEMSIZE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_MEMSIZE))

GType      clay_memsize_get_type         (void);

gchar    * clay_memsize_serialize        (guint64      memsize) G_GNUC_MALLOC;
gboolean   clay_memsize_deserialize      (const gchar *string,
                                          guint64     *memsize);


/*
 * CLAY_TYPE_PARAM_MEMSIZE
 */

#define CLAY_TYPE_PARAM_MEMSIZE           (clay_param_memsize_get_type ())
#define CLAY_IS_PARAM_SPEC_MEMSIZE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_MEMSIZE))

GType        clay_param_memsize_get_type  (void);

GParamSpec * clay_param_spec_memsize      (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           guint64       minimum,
                                           guint64       maximum,
                                           guint64       default_value,
                                           GParamFlags   flags);


G_END_DECLS

#endif  /* __CLAY_MEMSIZE_H__ */
