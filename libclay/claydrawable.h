/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claydrawable.h
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

#ifndef __CLAY_DRAWABLE_H__
#define __CLAY_DRAWABLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libclay/clayitem.h>


#define CLAY_TYPE_DRAWABLE (clay_drawable_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDrawable, clay_drawable, CLAY, DRAWABLE, ClayItem)


struct _ClayDrawableClass
{
  ClayItemClass parent_class;

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


ClayDrawable       * clay_drawable_get_by_id              (gint32                 drawable_id);

GeglBuffer         * clay_drawable_get_buffer             (ClayDrawable           *drawable) G_GNUC_WARN_UNUSED_RESULT;
GeglBuffer         * clay_drawable_get_shadow_buffer      (ClayDrawable           *drawable) G_GNUC_WARN_UNUSED_RESULT;

const Babl         * clay_drawable_get_format             (ClayDrawable           *drawable);
const Babl         * clay_drawable_get_thumbnail_format   (ClayDrawable           *drawable);

GBytes             * clay_drawable_get_thumbnail_data     (ClayDrawable           *drawable,
                                                           gint                    width,
                                                           gint                    height,
                                                           gint                   *actual_width,
                                                           gint                   *actual_height,
                                                           gint                   *bpp);
GdkPixbuf          * clay_drawable_get_thumbnail          (ClayDrawable           *drawable,
                                                           gint                    width,
                                                           gint                    height,
                                                           ClayPixbufTransparency  alpha);

GBytes             * clay_drawable_get_sub_thumbnail_data (ClayDrawable           *drawable,
                                                           gint                    src_x,
                                                           gint                    src_y,
                                                           gint                    src_width,
                                                           gint                    src_height,
                                                           gint                    dest_width,
                                                           gint                    dest_height,
                                                           gint                   *actual_width,
                                                           gint                   *actual_height,
                                                           gint                   *bpp);
GdkPixbuf          * clay_drawable_get_sub_thumbnail      (ClayDrawable           *drawable,
                                                           gint                    src_x,
                                                           gint                    src_y,
                                                           gint                    src_width,
                                                           gint                    src_height,
                                                           gint                    dest_width,
                                                           gint                    dest_height,
                                                           ClayPixbufTransparency  alpha);

void                 clay_drawable_append_filter          (ClayDrawable           *drawable,
                                                           ClayDrawableFilter     *filter);
void                 clay_drawable_merge_filter           (ClayDrawable           *drawable,
                                                           ClayDrawableFilter     *filter);

ClayDrawableFilter * clay_drawable_append_new_filter      (ClayDrawable           *drawable,
                                                           const gchar            *operation_name,
                                                           const gchar            *name,
                                                           ClayLayerMode           mode,
                                                           gdouble                 opacity,
                                                           ...) G_GNUC_NULL_TERMINATED;
void                 clay_drawable_merge_new_filter       (ClayDrawable           *drawable,
                                                           const gchar            *operation_name,
                                                           const gchar            *name,
                                                           ClayLayerMode           mode,
                                                           gdouble                 opacity,
                                                           ...) G_GNUC_NULL_TERMINATED;


G_END_DECLS

#endif /* __CLAY_DRAWABLE_H__ */
