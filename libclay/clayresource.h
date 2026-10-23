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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_RESOURCE_H__
#define __CLAY_RESOURCE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_RESOURCE (clay_resource_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayResource, clay_resource, CLAY, RESOURCE, GObject)


struct _ClayResourceClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};

gboolean       clay_resources_loaded     (GType         resource_type);

gint32         clay_resource_get_id      (ClayResource *resource);
ClayResource * clay_resource_get_by_id   (gint32        resource_id);
ClayResource * clay_resource_get_by_name (GType         resource_type,
                                          const gchar  *resource_name);

gboolean       clay_resource_is_valid    (ClayResource *resource);
gboolean       clay_resource_is_brush    (ClayResource *resource);
gboolean       clay_resource_is_pattern  (ClayResource *resource);
gboolean       clay_resource_is_gradient (ClayResource *resource);
gboolean       clay_resource_is_palette  (ClayResource *resource);
gboolean       clay_resource_is_font     (ClayResource *resource);


G_END_DECLS

#endif /* __CLAY_RESOURCE_H__ */
