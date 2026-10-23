/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claylayer.h
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

#ifndef __CLAY_LAYER_H__
#define __CLAY_LAYER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libclay/claydrawable.h>


#define CLAY_TYPE_LAYER (clay_layer_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayLayer, clay_layer, CLAY, LAYER, ClayDrawable)

struct _ClayLayerClass
{
  ClayDrawableClass parent_class;

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

ClayLayer * clay_layer_get_by_id          (gint32           layer_id);

ClayLayer * clay_layer_new_from_pixbuf    (ClayImage       *image,
                                           const gchar     *name,
                                           GdkPixbuf       *pixbuf,
                                           gdouble          opacity,
                                           ClayLayerMode    mode,
                                           gdouble          progress_start,
                                           gdouble          progress_end) G_GNUC_WARN_UNUSED_RESULT;
ClayLayer * clay_layer_new_from_surface   (ClayImage       *image,
                                           const gchar     *name,
                                           cairo_surface_t *surface,
                                           gdouble          progress_start,
                                           gdouble          progress_end) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* __CLAY_LAYER_H__ */
