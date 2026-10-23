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
#include <gexiv2/gexiv2.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "clay.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-metadata.h"
#include "clayimage-private.h"
#include "clayimage-rotate.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer-new.h"


/* public functions */


ClayMetadata *
clay_image_get_metadata (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  return private->metadata;
}

void
clay_image_set_metadata (ClayImage    *image,
                         ClayMetadata *metadata,
                         gboolean      push_undo)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (metadata != private->metadata)
    {
      if (push_undo)
        clay_image_undo_push_image_metadata (image, NULL);

      g_set_object (&private->metadata, metadata);

      if (private->metadata)
        {
          clay_image_metadata_update_pixel_size      (image);
          clay_image_metadata_update_bits_per_sample (image);
          clay_image_metadata_update_resolution      (image);
          clay_image_metadata_update_colorspace      (image);
        }

      g_object_notify (G_OBJECT (image), "metadata");
    }
}

void
clay_image_metadata_update_pixel_size (ClayImage *image)
{
  ClayMetadata *metadata;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  metadata = clay_image_get_metadata (image);

  if (metadata)
    {
      clay_metadata_set_pixel_size (metadata,
                                    clay_image_get_width  (image),
                                    clay_image_get_height (image));
    }
}

void
clay_image_metadata_update_bits_per_sample (ClayImage *image)
{
  ClayMetadata *metadata;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  metadata = clay_image_get_metadata (image);

  if (metadata)
    {
      switch (clay_image_get_component_type (image))
        {
        case CLAY_COMPONENT_TYPE_U8:
          clay_metadata_set_bits_per_sample (metadata, 8);
          break;

        case CLAY_COMPONENT_TYPE_U16:
        case CLAY_COMPONENT_TYPE_HALF:
          clay_metadata_set_bits_per_sample (metadata, 16);
          break;

        case CLAY_COMPONENT_TYPE_U32:
        case CLAY_COMPONENT_TYPE_FLOAT:
          clay_metadata_set_bits_per_sample (metadata, 32);
          break;

        case CLAY_COMPONENT_TYPE_DOUBLE:
          clay_metadata_set_bits_per_sample (metadata, 64);
          break;
        }
    }
}

void
clay_image_metadata_update_resolution (ClayImage *image)
{
  ClayMetadata *metadata;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  metadata = clay_image_get_metadata (image);

  if (metadata)
    {
      gdouble xres, yres;

      clay_image_get_resolution (image, &xres, &yres);
      clay_metadata_set_resolution (metadata, xres, yres,
                                    clay_image_get_unit (image));
    }
}

void
clay_image_metadata_update_colorspace (ClayImage *image)
{
  ClayMetadata *metadata;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  metadata = clay_image_get_metadata (image);

  if (metadata)
    {
      /*  See the discussions in issue #3532 and issue #301  */

      ClayColorProfile       *profile = clay_image_get_color_profile (image);
      ClayMetadataColorspace  space   = CLAY_METADATA_COLORSPACE_UNSPECIFIED;

      if (profile)
        {
          static ClayColorProfile *adobe = NULL;

          if (! adobe)
            adobe = clay_color_profile_new_rgb_adobe ();

          if (clay_color_profile_is_equal (profile, adobe))
            space = CLAY_METADATA_COLORSPACE_ADOBERGB;
        }
      else
        {
          space = CLAY_METADATA_COLORSPACE_SRGB;
        }

      clay_metadata_set_colorspace (metadata, space);
    }
}

/**
 * clay_image_metadata_load_thumbnail:
 * @clay:              The #Clay object.
 * @file:              A #GFile image.
 * @full_image_width:  the width of the full image (not the thumbnail).
 * @full_image_height: the height of the full image (not the thumbnail).
 * @error:             Return location for error message
 *
 * Retrieves a thumbnail from metadata in @file if present.
 * It does not need to actually load the full image, only the metadata through
 * GExiv2, which makes it fast.
 *
 * Returns: (transfer none) (nullable): a #ClayImage of the @file thumbnail.
 */
ClayImage *
clay_image_metadata_load_thumbnail (Clay        *clay,
                                    GFile       *file,
                                    gint        *full_image_width,
                                    gint        *full_image_height,
                                    const Babl **format,
                                    GError     **error)
{
  ClayMetadata *metadata;
  GInputStream *input_stream;
  GdkPixbuf    *pixbuf;
  guint8       *thumbnail_buffer;
  gint          thumbnail_size;
  ClayImage    *image = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = clay_metadata_load_from_file (file, error);
  if (! metadata)
    return NULL;

  if (! gexiv2_metadata_get_exif_thumbnail (GEXIV2_METADATA (metadata),
                                            &thumbnail_buffer,
                                            &thumbnail_size))
    {
      g_object_unref (metadata);
      return NULL;
    }

  input_stream = g_memory_input_stream_new_from_data (thumbnail_buffer,
                                                      thumbnail_size,
                                                      (GDestroyNotify) g_free);
  pixbuf = gdk_pixbuf_new_from_stream (input_stream, NULL, error);
  g_object_unref (input_stream);

  if (pixbuf)
    {
      ClayLayer *layer;

      image = clay_image_new (clay,
                              gdk_pixbuf_get_width  (pixbuf),
                              gdk_pixbuf_get_height (pixbuf),
                              CLAY_RGB, CLAY_PRECISION_U8_NON_LINEAR);
      clay_image_undo_disable (image);

      /* XXX This is possibly wrong, because an image of a given color space may
       * have a thumbnail stored in a different colorspace. This is even more
       * true with Clay which always exports RGB thumbnails (see code in
       * clay_image_metadata_save_filter()), even for say grayscale images.
       * Nevertheless other software may store thumbnail using the same
       * colorspace.
       */
      *format = clay_pixbuf_get_format (pixbuf);
      layer = clay_layer_new_from_pixbuf (pixbuf, image,
                                          clay_image_get_layer_format (image, FALSE),
                                          /* No need to localize; this image is short-lived. */
                                          "Background",
                                          CLAY_OPACITY_OPAQUE,
                                          clay_image_get_default_new_layer_mode (image));
      g_object_unref (pixbuf);

      clay_image_add_layer (image, layer, NULL, 0, FALSE);

      clay_image_apply_metadata_orientation (image, clay_get_user_context (clay), metadata, NULL);
    }

  /* This is the unoriented dimensions. Should we switch when there is a 90 or
   * 270 degree rotation? We don't actually know if the metadata orientation is
   * correct.
   */
  *full_image_width  = gexiv2_metadata_get_pixel_width (GEXIV2_METADATA (metadata));
  *full_image_height = gexiv2_metadata_get_pixel_height (GEXIV2_METADATA (metadata));

  if (*full_image_width < 1 || *full_image_height < 1)
    {
      /* Dimensions stored in metadata might be less accurate, yet it's still
       * informational.
       */
      *full_image_width  = gexiv2_metadata_try_get_metadata_pixel_width (GEXIV2_METADATA (metadata), NULL);
      *full_image_height = gexiv2_metadata_try_get_metadata_pixel_width (GEXIV2_METADATA (metadata), NULL);
    }
  if (*full_image_width < 1 || *full_image_height < 1)
    {
      *full_image_width  = 0;
      *full_image_height = 0;
    }

  g_object_unref (metadata);

  return image;
}
