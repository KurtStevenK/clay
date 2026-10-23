/* tiff for Clay
 *  -Peter Mattis
 */

#ifndef __FILE_TIFF_H__
#define __FILE_TIFF_H__

#define LOAD_PROC                   "file-tiff-load"
#define EXPORT_PROC                 "file-tiff-export"

#define TIFF_PSD_COMPOSITE_PARASITE "file-tiff-psd-composite"

typedef enum
{
 CLAY_COMPRESSION_NONE,
 CLAY_COMPRESSION_LZW,
 CLAY_COMPRESSION_PACKBITS,
 CLAY_COMPRESSION_ADOBE_DEFLATE,
 CLAY_COMPRESSION_JPEG,
 CLAY_COMPRESSION_CCITTFAX3,
 CLAY_COMPRESSION_CCITTFAX4
} ClayCompression;


typedef enum
{
  CLAY_TIFF_FORMAT_STANDARD_TIFF,
  CLAY_TIFF_FORMAT_PHOTOSHOP_TIFF,
} ClayFormat;

gint              clay_compression_to_tiff_compression (ClayCompression  compression);
ClayCompression   tiff_compression_to_clay_compression (gint             compression);
const gchar     * clay_compression_to_nick             (ClayCompression  compression);


#endif /* __FILE_TIFF_H__ */
