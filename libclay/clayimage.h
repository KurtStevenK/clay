/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * clayimage.h
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

#ifndef __CLAY_IMAGE_H__
#define __CLAY_IMAGE_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_IMAGE (clay_image_get_type ())
G_DECLARE_FINAL_TYPE (ClayImage, clay_image, CLAY, IMAGE, GObject)


gint32         clay_image_get_id                  (ClayImage    *image);
ClayImage    * clay_image_get_by_id               (gint32        image_id);

gboolean       clay_image_is_valid                (ClayImage    *image);

GList        * clay_list_images                   (void);

GList        * clay_image_list_layers             (ClayImage    *image);
GList        * clay_image_list_channels           (ClayImage    *image);
GList        * clay_image_list_paths              (ClayImage    *image);

GList        * clay_image_list_selected_layers    (ClayImage    *image);
gboolean       clay_image_take_selected_layers    (ClayImage    *image,
                                                   GList        *layers);
GList        * clay_image_list_selected_channels  (ClayImage    *image);
gboolean       clay_image_take_selected_channels  (ClayImage    *image,
                                                   GList        *channels);
GList        * clay_image_list_selected_paths     (ClayImage    *image);
gboolean       clay_image_take_selected_paths     (ClayImage    *image,
                                                   GList        *paths);

GList        * clay_image_list_selected_drawables (ClayImage    *image);

GBytes       * clay_image_get_thumbnail_data      (ClayImage    *image,
                                                   gint         *width,
                                                   gint         *height,
                                                   gint         *bpp);
GdkPixbuf    * clay_image_get_thumbnail           (ClayImage    *image,
                                                   gint          width,
                                                   gint          height,
                                                   ClayPixbufTransparency  alpha);

ClayMetadata * clay_image_get_metadata            (ClayImage    *image);
gboolean       clay_image_set_metadata            (ClayImage    *image,
                                                   ClayMetadata *metadata);


G_END_DECLS

#endif /* __CLAY_IMAGE_H__ */
