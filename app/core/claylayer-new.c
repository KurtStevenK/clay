/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "claybuffer.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "claylayer.h"
#include "claylayer-new.h"


/*  local function prototypes  */

static void   clay_layer_new_convert_buffer (ClayLayer         *layer,
                                             GeglBuffer        *src_buffer,
                                             ClayColorProfile  *src_profile,
                                             GError           **error);


/*  public functions  */

ClayLayer *
clay_layer_new (ClayImage     *image,
                gint           width,
                gint           height,
                const Babl    *format,
                const gchar   *name,
                gdouble        opacity,
                ClayLayerMode  mode)
{
  ClayLayer *layer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  layer = CLAY_LAYER (clay_drawable_new (CLAY_TYPE_LAYER,
                                         image, name,
                                         0, 0, width, height,
                                         format));

  clay_layer_set_opacity (layer, opacity, FALSE);
  clay_layer_set_mode (layer, mode, FALSE);

  return layer;
}

/**
 * clay_layer_new_from_buffer:
 * @buffer:     The buffer to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @format:     The #Babl format of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %buffer to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Returns: The new layer.
 **/
ClayLayer *
clay_layer_new_from_buffer (ClayBuffer    *buffer,
                            ClayImage     *dest_image,
                            const Babl    *format,
                            const gchar   *name,
                            gdouble        opacity,
                            ClayLayerMode  mode)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  return clay_layer_new_from_gegl_buffer (clay_buffer_get_buffer (buffer),
                                          dest_image, format,
                                          name, opacity, mode,
                                          clay_buffer_get_color_profile (buffer));
}

/**
 * clay_layer_new_from_gegl_buffer:
 * @buffer:     The buffer to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @format:     The #Babl format of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %buffer to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Returns: The new layer.
 **/
ClayLayer *
clay_layer_new_from_gegl_buffer (GeglBuffer       *buffer,
                                 ClayImage        *dest_image,
                                 const Babl       *format,
                                 const gchar      *name,
                                 gdouble           opacity,
                                 ClayLayerMode     mode,
                                 ClayColorProfile *buffer_profile)
{
  ClayLayer           *layer;
  const GeglRectangle *extent;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (buffer_profile == NULL ||
                        CLAY_IS_COLOR_PROFILE (buffer_profile), NULL);

  extent = gegl_buffer_get_extent (buffer);

  /*  do *not* use the buffer's format because this function gets
   *  buffers of any format passed, and converts them
   */
  layer = clay_layer_new (dest_image,
                          extent->width, extent->height,
                          format,
                          name, opacity, mode);

  if (extent->x != 0 || extent->y != 0)
    clay_item_translate (CLAY_ITEM (layer), extent->x, extent->y, FALSE);

  clay_layer_new_convert_buffer (layer, buffer, buffer_profile, NULL);

  return layer;
}

/**
 * clay_layer_new_from_pixbuf:
 * @pixbuf:     The pixbuf to make the new layer from.
 * @dest_image: The image the new layer will be added to.
 * @format:     The #Babl format of the new layer.
 * @name:       The new layer's name.
 * @opacity:    The new layer's opacity.
 * @mode:       The new layer's mode.
 *
 * Copies %pixbuf to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 *
 * Returns: The new layer.
 **/
ClayLayer *
clay_layer_new_from_pixbuf (GdkPixbuf     *pixbuf,
                            ClayImage     *dest_image,
                            const Babl    *format,
                            const gchar   *name,
                            gdouble        opacity,
                            ClayLayerMode  mode)
{
  ClayLayer        *layer;
  GeglBuffer       *buffer;
  guint8           *icc_data;
  gsize             icc_len;
  ClayColorProfile *profile = NULL;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  layer = clay_layer_new (dest_image,
                          gdk_pixbuf_get_width  (pixbuf),
                          gdk_pixbuf_get_height (pixbuf),
                          format, name, opacity, mode);

  buffer = clay_pixbuf_create_buffer (pixbuf);

  icc_data = clay_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      profile = clay_color_profile_new_from_icc_profile (icc_data, icc_len,
                                                         NULL);
      g_free (icc_data);
    }

  clay_layer_new_convert_buffer (layer, buffer, profile, NULL);

  if (profile)
    g_object_unref (profile);

  g_object_unref (buffer);

  return layer;
}


/*  private functions  */

static void
clay_layer_new_convert_buffer (ClayLayer         *layer,
                               GeglBuffer        *src_buffer,
                               ClayColorProfile  *src_profile,
                               GError           **error)
{
  ClayDrawable     *drawable    = CLAY_DRAWABLE (layer);
  GeglBuffer       *dest_buffer = clay_drawable_get_buffer (drawable);
  ClayColorProfile *dest_profile;

  if (! src_profile)
    {
      const Babl *src_format = gegl_buffer_get_format (src_buffer);

      src_profile = clay_babl_format_get_color_profile (src_format);
    }
  else
    {
      g_object_ref (src_profile);
    }

  dest_profile =
    clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (layer));

  clay_gegl_convert_color_profile (src_buffer,  NULL, src_profile,
                                   dest_buffer, NULL, dest_profile,
                                   CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                   TRUE, NULL);

  g_object_unref (src_profile);
}
