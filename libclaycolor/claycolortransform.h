/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claycolortransform.h
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *                     Elle Stone <ellestone@ninedegreesbelow.com>
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

#if !defined (__CLAY_COLOR_H_INSIDE__) && !defined (CLAY_COLOR_COMPILATION)
#error "Only <libclaycolor/claycolor.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_TRANSFORM_H__
#define __CLAY_COLOR_TRANSFORM_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayColorTransformFlags:
 * @CLAY_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE: optimize for accuracy rather
 *   than for speed
 * @CLAY_COLOR_TRANSFORM_FLAGS_GAMUT_CHECK: mark out of gamut colors in the
 *   transform result
 * @CLAY_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION: do black point
 *   compensation
 *
 * Flags for modifying #ClayColorTransform's behavior.
 **/
typedef enum
{
  CLAY_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE               = 0x0100,
  CLAY_COLOR_TRANSFORM_FLAGS_GAMUT_CHECK              = 0x1000,
  CLAY_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION = 0x2000,
} ClayColorTransformFlags;


#define CLAY_TYPE_COLOR_TRANSFORM (clay_color_transform_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorTransform, clay_color_transform, CLAY, COLOR_TRANSFORM, GObject)


ClayColorTransform *
        clay_color_transform_new              (ClayColorProfile         *src_profile,
                                               const Babl               *src_format,
                                               ClayColorProfile         *dest_profile,
                                               const Babl               *dest_format,
                                               ClayColorRenderingIntent  rendering_intent,
                                               ClayColorTransformFlags   flags);

ClayColorTransform *
        clay_color_transform_new_proofing     (ClayColorProfile         *src_profile,
                                               const Babl               *src_format,
                                               ClayColorProfile         *dest_profile,
                                               const Babl               *dest_format,
                                               ClayColorProfile         *proof_profile,
                                               ClayColorRenderingIntent  proof_intent,
                                               ClayColorRenderingIntent  display_intent,
                                               ClayColorTransformFlags   flags);

void    clay_color_transform_process_pixels   (ClayColorTransform       *transform,
                                               const Babl               *src_format,
                                               gconstpointer             src_pixels,
                                               const Babl               *dest_format,
                                               gpointer                  dest_pixels,
                                               gsize                     length);

void    clay_color_transform_process_buffer   (ClayColorTransform       *transform,
                                               GeglBuffer               *src_buffer,
                                               const GeglRectangle      *src_rect,
                                               GeglBuffer               *dest_buffer,
                                               const GeglRectangle      *dest_rect);

gboolean clay_color_transform_can_gegl_copy   (ClayColorProfile         *src_profile,
                                               ClayColorProfile         *dest_profile);


G_END_DECLS

#endif  /* __CLAY_COLOR_TRANSFORM_H__ */
