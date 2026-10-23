/* LIBCLAYBASE - The Clay Basic Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claymetadata.h
 * Copyright (C) 2013 Hartmut Kuhse <hartmutkuhse@src.gnome.org>
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

#ifndef __CLAY_METADATA_H__
#define __CLAY_METADATA_H__

G_BEGIN_DECLS

#include <gexiv2/gexiv2.h>


#define CLAY_TYPE_METADATA (clay_metadata_get_type ())
G_DECLARE_FINAL_TYPE (ClayMetadata, clay_metadata, CLAY, METADATA, GExiv2Metadata)


/**
 * ClayMetadataLoadFlags:
 * @CLAY_METADATA_LOAD_NONE:        Do not load the metadata
 * @CLAY_METADATA_LOAD_COMMENT:     Load the comment
 * @CLAY_METADATA_LOAD_RESOLUTION:  Load the resolution
 * @CLAY_METADATA_LOAD_ORIENTATION: Load the orientation (rotation)
 * @CLAY_METADATA_LOAD_COLORSPACE:  Load the colorspace
 * @CLAY_METADATA_LOAD_ALL:         Load all of the above
 *
 * What metadata to load when importing images.
 **/
typedef enum
{
  CLAY_METADATA_LOAD_NONE        = 0,
  CLAY_METADATA_LOAD_COMMENT     = 1 << 0,
  CLAY_METADATA_LOAD_RESOLUTION  = 1 << 1,
  CLAY_METADATA_LOAD_ORIENTATION = 1 << 2,
  CLAY_METADATA_LOAD_COLORSPACE  = 1 << 3,

  CLAY_METADATA_LOAD_ALL         = 0xffffffff
} ClayMetadataLoadFlags;


/**
 * ClayMetadataSaveFlags:
 * @CLAY_METADATA_SAVE_EXIF:          Save EXIF
 * @CLAY_METADATA_SAVE_XMP:           Save XMP
 * @CLAY_METADATA_SAVE_IPTC:          Save IPTC
 * @CLAY_METADATA_SAVE_THUMBNAIL:     Save a thumbnail of the image
 * @CLAY_METADATA_SAVE_COLOR_PROFILE: Save the image's color profile
 *                                    Since: 2.10.10
 * @CLAY_METADATA_SAVE_COMMENT:       Save the image's comment
 *                                    Since: 3.0
 * @CLAY_METADATA_SAVE_UPDATE:        Update metadata automatically
 *                                    Since: 3.2
 * @CLAY_METADATA_SAVE_ALL:           Save all of the above
 *
 * What kinds of metadata to save when exporting images.
 **/
typedef enum
{
  CLAY_METADATA_SAVE_EXIF          = 1 << 0,
  CLAY_METADATA_SAVE_XMP           = 1 << 1,
  CLAY_METADATA_SAVE_IPTC          = 1 << 2,
  CLAY_METADATA_SAVE_THUMBNAIL     = 1 << 3,
  CLAY_METADATA_SAVE_COLOR_PROFILE = 1 << 4,
  CLAY_METADATA_SAVE_COMMENT       = 1 << 5,
  CLAY_METADATA_SAVE_UPDATE        = 1 << 6,

  CLAY_METADATA_SAVE_ALL       = 0xffffffff
} ClayMetadataSaveFlags;


/**
 * ClayMetadataColorspace:
 * @CLAY_METADATA_COLORSPACE_UNSPECIFIED:  Unspecified
 * @CLAY_METADATA_COLORSPACE_UNCALIBRATED: Uncalibrated
 * @CLAY_METADATA_COLORSPACE_SRGB:         sRGB
 * @CLAY_METADATA_COLORSPACE_ADOBERGB:     Adobe RGB
 *
 * Well-defined colorspace information available from metadata
 **/
typedef enum
{
  CLAY_METADATA_COLORSPACE_UNSPECIFIED,
  CLAY_METADATA_COLORSPACE_UNCALIBRATED,
  CLAY_METADATA_COLORSPACE_SRGB,
  CLAY_METADATA_COLORSPACE_ADOBERGB
} ClayMetadataColorspace;


ClayMetadata * clay_metadata_new                 (void);
ClayMetadata * clay_metadata_duplicate           (ClayMetadata           *metadata);

ClayMetadata * clay_metadata_deserialize         (const gchar            *metadata_xml);
gchar        * clay_metadata_serialize           (ClayMetadata           *metadata);
gchar        * clay_metadata_get_guid            (void);

void           clay_metadata_add_xmp_history     (ClayMetadata           *metadata,
                                                  gchar                  *state_status);

ClayMetadata * clay_metadata_load_from_file      (GFile                  *file,
                                                  GError                **error);
gboolean       clay_metadata_save_to_file        (ClayMetadata           *metadata,
                                                  GFile                  *file,
                                                  GError                **error);

gboolean       clay_metadata_set_from_exif       (ClayMetadata           *metadata,
                                                  const guchar           *exif_data,
                                                  gint                    exif_data_length,
                                                  GError                **error);
gboolean       clay_metadata_set_from_iptc       (ClayMetadata           *metadata,
                                                  const guchar           *iptc_data,
                                                  gint                    iptc_data_length,
                                                  GError                **error);
gboolean       clay_metadata_set_from_xmp        (ClayMetadata           *metadata,
                                                  const guchar           *xmp_data,
                                                  gint                    xmp_data_length,
                                                  GError                **error);

void           clay_metadata_set_pixel_size      (ClayMetadata           *metadata,
                                                  gint                    width,
                                                  gint                    height);
void           clay_metadata_set_bits_per_sample (ClayMetadata           *metadata,
                                                  gint                    bits_per_sample);

gboolean       clay_metadata_get_resolution      (ClayMetadata           *metadata,
                                                  gdouble                *xres,
                                                  gdouble                *yres,
                                                  ClayUnit              **unit);
void           clay_metadata_set_resolution      (ClayMetadata           *metadata,
                                                  gdouble                 xres,
                                                  gdouble                 yres,
                                                  ClayUnit               *unit);
void           clay_metadata_set_creation_date   (ClayMetadata           *metadata,
                                                  GDateTime              *datetime);

ClayMetadataColorspace
               clay_metadata_get_colorspace      (ClayMetadata           *metadata);
void           clay_metadata_set_colorspace      (ClayMetadata           *metadata,
                                                  ClayMetadataColorspace  colorspace);

gboolean       clay_metadata_is_tag_supported    (const gchar            *tag,
                                                  const gchar            *mime_type);

G_END_DECLS

#endif /* __CLAY_METADATA_H__ */
