/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claylayer.c
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

#include "config.h"

#include <string.h>

#include "clay.h"



G_DEFINE_TYPE (ClayLayer, clay_layer, CLAY_TYPE_DRAWABLE)

#define parent_class clay_layer_parent_class


static void
clay_layer_class_init (ClayLayerClass *klass)
{
}

static void
clay_layer_init (ClayLayer *layer)
{
}


/* Public API. */

/**
 * clay_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #ClayLayer representing @layer_id. This function calls
 * clay_item_get_by_id() and returns the item if it is layer or %NULL
 * otherwise.
 *
 * Returns: (nullable) (transfer none): a #ClayLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid layer. The
 *          object belongs to libclay and you must not modify or unref
 *          it.
 *
 * Since: 3.0
 **/
ClayLayer *
clay_layer_get_by_id (gint32 layer_id)
{
  ClayItem *item = clay_item_get_by_id (layer_id);

  if (CLAY_IS_LAYER (item))
    return (ClayLayer *) item;

  return NULL;
}

/**
 * clay_layer_new_from_pixbuf:
 * @image:          The RGB image to which to add the layer.
 * @name:           The layer name.
 * @pixbuf:         A GdkPixbuf.
 * @opacity:        The layer opacity.
 * @mode:           The layer combination mode.
 * @progress_start: start of progress
 * @progress_end:   end of progress
 *
 * Create a new layer from a %GdkPixbuf.
 *
 * This procedure creates a new layer from the given %GdkPixbuf.  The
 * image has to be an RGB image and just like with clay_layer_new()
 * you will still need to add the layer to it.
 *
 * If you pass @progress_end > @progress_start to this function,
 * clay_progress_update() will be called for. You have to call
 * clay_progress_init() beforehand then.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libclay and you should not free it.
 *
 * Since: 2.2
 */
ClayLayer *
clay_layer_new_from_pixbuf (ClayImage     *image,
                            const gchar   *name,
                            GdkPixbuf     *pixbuf,
                            gdouble        opacity,
                            ClayLayerMode  mode,
                            gdouble        progress_start,
                            gdouble        progress_end)
{
  GeglBuffer *dest_buffer;
  ClayLayer  *layer;
  gint        width;
  gint        height;
  gint        bpp;
  gdouble     range = progress_end - progress_start;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  if (clay_image_get_base_type (image) != CLAY_RGB)
    {
      g_warning ("clay_layer_new_from_pixbuf() needs an RGB image");
      return NULL;
    }

  if (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB)
    {
      g_warning ("clay_layer_new_from_pixbuf() assumes that GdkPixbuf is RGB");
      return NULL;
    }

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  bpp    = gdk_pixbuf_get_n_channels (pixbuf);

  layer = clay_layer_new (image, name, width, height,
                          bpp == 3 ? CLAY_RGB_IMAGE : CLAY_RGBA_IMAGE,
                          opacity, mode);

  if (! layer)
    return NULL;

  dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   clay_pixbuf_get_format (pixbuf),
                   gdk_pixbuf_get_pixels (pixbuf),
                   gdk_pixbuf_get_rowstride (pixbuf));

  g_object_unref (dest_buffer);

  if (range > 0.0)
    clay_progress_update (progress_end);

  return layer;
}

/**
 * clay_layer_new_from_surface:
 * @image:           The RGB image to which to add the layer.
 * @name:            The layer name.
 * @surface:         A Cairo image surface.
 * @progress_start:  start of progress
 * @progress_end:    end of progress
 *
 * Create a new layer from a [type@cairo.Surface].
 *
 * This procedure creates a new layer from the given
 * [type@cairo.Surface]. The image has to be an RGB image and just like
 * with clay_layer_new() you will still need to add the layer to it.
 *
 * If you pass @progress_end > @progress_start to this function,
 * clay_progress_update() will be called for. You have to call
 * clay_progress_init() beforehand then.
 *
 * Returns: (transfer none): The newly created layer.
 *          The object belongs to libclay and you should not free it.
 *
 * Since: 2.8
 */
ClayLayer *
clay_layer_new_from_surface (ClayImage       *image,
                             const gchar     *name,
                             cairo_surface_t *surface,
                             gdouble          progress_start,
                             gdouble          progress_end)
{
  GeglBuffer    *src_buffer;
  GeglBuffer    *dest_buffer;
  ClayLayer     *layer;
  gint           width;
  gint           height;
  cairo_format_t format;
  gdouble        range = progress_end - progress_start;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) ==
                        CAIRO_SURFACE_TYPE_IMAGE, NULL);

  if (clay_image_get_base_type (image) != CLAY_RGB)
    {
      g_warning ("clay_layer_new_from_surface() needs an RGB image");
      return NULL;
    }

  width  = cairo_image_surface_get_width (surface);
  height = cairo_image_surface_get_height (surface);
  format = cairo_image_surface_get_format (surface);

  if (format != CAIRO_FORMAT_ARGB32 &&
      format != CAIRO_FORMAT_RGB24)
    {
      g_warning ("clay_layer_new_from_surface() assumes that surface is RGB");
      return NULL;
    }

  layer = clay_layer_new (image, name, width, height,
                          format == CAIRO_FORMAT_RGB24 ?
                          CLAY_RGB_IMAGE : CLAY_RGBA_IMAGE,
                          100.0,
                          clay_image_get_default_new_layer_mode (image));

  if (layer == NULL)
    return NULL;

  src_buffer = clay_cairo_surface_get_buffer (surface, NULL, FALSE);
  dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE,
                    dest_buffer, NULL);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  if (range > 0.0)
    clay_progress_update (progress_end);

  return layer;
}
