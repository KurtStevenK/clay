/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayimagecolorprofile.h
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

#ifndef __CLAY_IMAGE_COLOR_PROFILE_H__
#define __CLAY_IMAGE_COLOR_PROFILE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


ClayColorProfile * clay_image_get_color_profile           (ClayImage                 *image);
gboolean           clay_image_set_color_profile           (ClayImage                 *image,
                                                           ClayColorProfile          *profile);

ClayColorProfile * clay_image_get_simulation_profile      (ClayImage                 *image);
gboolean           clay_image_set_simulation_profile      (ClayImage                 *image,
                                                           ClayColorProfile          *profile);

ClayColorProfile * clay_image_get_effective_color_profile (ClayImage                 *image);

gboolean           clay_image_convert_color_profile       (ClayImage                 *image,
                                                           ClayColorProfile          *profile,
                                                           ClayColorRenderingIntent   intent,
                                                           gboolean                   bpc);


G_END_DECLS

#endif /* __CLAY_IMAGE_COLOR_PROFILE_H__ */
