/*   xyz.kainzmayer.ClayDesign
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Bohemia Interactive PAA graphics plug-in
 *
 *   Copyright (C) 2025 Alex S.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>


#include "libclay/stdplugins-intl.h"


#define LOAD_PROC      "file-paa-load"
#define PLUG_IN_BINARY "file-paa"
#define PLUG_IN_ROLE   "clay-file-paa"


typedef enum
{
  RGBA_4444  = 0x4444,
  RGBA_5551  = 0x1555,
  GRAY_ALPHA = 0x8080,
  RGBA_8888  = 0x8888,
  DXT1       = 0xFF01,
  DXT2       = 0xFF02,
  DXT3       = 0xFF03,
  DXT4       = 0xFF04,
  DXT5       = 0xFF05
} PaaType;

typedef struct _Paa      Paa;
typedef struct _PaaClass PaaClass;

struct _Paa
{
  ClayPlugIn      parent_instance;
};

struct _PaaClass
{
  ClayPlugInClass parent_class;
};


#define PAA_TYPE  (paa_get_type ())
#define PAA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAA_TYPE, Paa))

GType                           paa_get_type          (void);


static GList          *         paa_query_procedures  (ClayPlugIn            *plug_in);
static ClayProcedure  *         paa_create_procedure  (ClayPlugIn            *plug_in,
                                                       const gchar           *name);

static ClayValueArray *         paa_load              (ClayProcedure         *procedure,
                                                       ClayRunMode            run_mode,
                                                       GFile                 *file,
                                                       ClayMetadata          *metadata,
                                                       ClayMetadataLoadFlags *flags,
                                                       ClayProcedureConfig   *config,
                                                       gpointer               run_data);

static ClayImage      *         load_image            (GFile                 *file,
                                                       ClayProcedureConfig   *config,
                                                       ClayRunMode            run_mode,
                                                       FILE                  *fp,
                                                       GError               **error);

static gboolean                 read_tag              (FILE                  *fp,
                                                       GError               **error);
static gboolean                 decode_lzss           (guchar                *raw_data,
                                                       guint32                raw_data_size,
                                                       guchar                *uncompressed_data,
                                                       gint                   estimated_size);

static void                     convert_from_a1r5g5b5 (gushort               data,
                                                       guint                 index,
                                                       guchar               *pixel);
static void                     convert_from_a4r4g4b4 (gushort               data,
                                                       guint                 index,
                                                       guchar               *pixel);

G_DEFINE_TYPE (Paa, paa, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (PAA_TYPE)
DEFINE_STD_SET_I18N


static void
paa_class_init (PaaClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = paa_query_procedures;
  plug_in_class->create_procedure = paa_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
paa_init (Paa *paa)
{
}

static GList *
paa_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static ClayProcedure *
paa_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           paa_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("PAA Image"));

      clay_procedure_set_documentation (procedure,
                                        _("Load file in the PAA file format"),
                                        _("Load file in the Bohemia Interactive "
                                          "PAA file format"),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Gruppe Adler",
                                      "Gruppe Adler",
                                      "2020");

      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "paa");
    }

  return procedure;
}

static ClayValueArray *
paa_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  FILE           *fp;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return clay_procedure_new_return_values (procedure,
                                               CLAY_PDB_EXECUTION_ERROR,
                                               error);
    }

  image = load_image (file, config, run_mode, fp, &error);
  fclose (fp);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);
  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayImage *
load_image (GFile                *file,
            ClayProcedureConfig  *config,
            ClayRunMode           run_mode,
            FILE                 *fp,
            GError              **error)
{
  ClayImage         *image = NULL;
  ClayLayer         *layer = NULL;
  GeglBuffer        *buffer;
  ClayImageBaseType  image_type;
  ClayImageType      layer_type;
  gushort            paa_type;
  gushort            palette_index;
  gint               num_mipmaps = 0;

  if (fread (&paa_type, 2, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   clay_file_get_utf8_name (file));
      return NULL;
    }

  paa_type = GUINT16_FROM_LE (paa_type);

  switch (paa_type)
    {
    case RGBA_4444:
    case RGBA_5551:
    case RGBA_8888:
      image_type = CLAY_RGB;
      layer_type = CLAY_RGBA_IMAGE;
      break;

    case GRAY_ALPHA:
      image_type = CLAY_GRAY;
      layer_type = CLAY_GRAYA_IMAGE;
      break;

    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Currently unsupported PAA format: '%d'"),
                   paa_type);
      return NULL;
    }

  /* Run through tags */
  while (read_tag (fp, error));

  if (error && *error)
    return NULL;

  if (fread (&palette_index, 2, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   clay_file_get_utf8_name (file));
      return NULL;
    }

  while (TRUE)
    {
      gushort  width;
      gushort  height;
      guchar   block_size_array[3];
      guint32  block_size;
      guchar  *raw_data;

      if (fread (&width, 2, 1, fp) == 0  ||
          fread (&height, 2, 1, fp) == 0 ||
          fread (block_size_array, 3, 1, fp) == 0)
        {
          /* If we have at least one layer and we get to the end of the file,
           * it's valid. Otherwise, we assume it's an error */
          if (! image)
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Could not read header from '%s'"),
                         clay_file_get_utf8_name (file));
          return image;
        }
      width  = GUINT16_FROM_LE (width);
      height = GUINT16_FROM_LE (height);

      block_size = ((guint32) block_size_array[2] << 16) +
                   ((guint32) block_size_array[1] << 8)  +
                   block_size_array[0];

      raw_data = g_try_malloc0 (block_size);
      if (raw_data == NULL)
        {
          g_set_error (error, G_FILE_ERROR, 0,
                       _("Memory could not be allocated."));
          return image;
        }

      if (fread (raw_data, block_size, 1, fp) == 0)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't read image data from '%s'"),
                       clay_file_get_utf8_name (file));
          return image;
        }

      if (image == NULL)
        image = clay_image_new (width, height, image_type);
      layer = clay_layer_new (image, _("Main surface"), width, height,
                              layer_type, 100,
                              clay_image_get_default_new_layer_mode (image));
      clay_image_insert_layer (image, layer, NULL, num_mipmaps);

      if (num_mipmaps > 0)
        {
          gchar *layer_name;

          layer_name = g_strdup_printf ("Mipmap: %dx%d", width, height);

          clay_item_set_name (CLAY_ITEM (layer), layer_name);
          g_free (layer_name);
        }
      buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

      /* Non-DDS textures are compressed with LZSS */
      if (paa_type <= RGBA_8888)
        {
          guchar *uncompressed_data;
          gsize   estimated_size;
          guchar *pixels;

          if (! g_size_checked_mul (&estimated_size, (guint32) width, height) ||
              ! g_size_checked_mul (&estimated_size, estimated_size, 4)       ||
              (pixels = g_try_malloc0 (estimated_size)) == NULL)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Memory could not be allocated."));
              g_object_unref (buffer);
              return NULL;
            }

          if (paa_type != RGBA_8888)
            estimated_size /= 2;

          uncompressed_data = g_try_malloc0 (estimated_size);
          if (uncompressed_data == NULL)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Memory could not be allocated."));
              g_object_unref (buffer);
              return NULL;
            }

          if (! decode_lzss (raw_data, block_size, uncompressed_data,
                             estimated_size))
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Couldn't decompress image data from '%s'"),
                           clay_file_get_utf8_name (file));

              g_free (raw_data);
              g_free (uncompressed_data);
              g_object_unref (buffer);
              return NULL;
            }

          switch (paa_type)
            {
              case RGBA_4444:
                {
                  for (gint j = 0; j < estimated_size; j += 2)
                    {
                      gushort condensed = ((guint16) uncompressed_data[j + 1] << 8) +
                                           uncompressed_data[j];

                      convert_from_a4r4g4b4 (condensed, j / 2, pixels);
                    }

                  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);
                }
                break;

              case RGBA_5551:
                {
                  for (gint j = 0; j < estimated_size; j += 2)
                    {
                      gushort condensed = ((guint16) uncompressed_data[j + 1] << 8) +
                                           uncompressed_data[j];

                      convert_from_a1r5g5b5 (condensed, j / 2, pixels);
                    }

                  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);
                }
                break;

              case RGBA_8888:
              case GRAY_ALPHA:
                gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                                 NULL, uncompressed_data, GEGL_AUTO_ROWSTRIDE);
                break;

              default:
                /* Shouldn't get here */
                break;
            }
          g_free (uncompressed_data);
          g_free (pixels);

          num_mipmaps++;
        }

      g_free (raw_data);
      g_object_unref (buffer);
    }

  return image;
}

static gboolean
read_tag (FILE    *fp,
          GError **error)
{
  gchar    tag[5];
  gchar    tag_name[5];
  guint32  data_length;
  guchar  *data;

  if (fread (tag, 4, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read tag"));

      return FALSE;
    }
  tag[4] = '\0';

  if (g_strcmp0 (tag, "GGAT") != 0)
    {
      fseek (fp, -4, SEEK_CUR);

      return FALSE;
    }

  if (fread (tag_name, 4, 1, fp) == 0 ||
      fread (&data_length, 4, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read tag"));

      return FALSE;
    }
  tag_name[4] = '\0';
  data_length = GUINT32_FROM_LE (data_length);

  data = g_malloc0 (data_length);
  if (fread (data, data_length, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read tag"));
      g_free (data);

      return FALSE;
    }
  g_free (data);

  return TRUE;
}

/* Implementation referenced from ReadLZSS () in
 * https://github.com/PackJC/Paint.NET-PAA-PAC-Importer/
   blob/main/BIS/Core/Compression/LZSS.cs */
static gboolean
decode_lzss (guchar  *raw_data,
             guint32  raw_data_size,
             guchar  *uncompressed_data,
             gint     estimated_size)
{
  gchar  char_array[4113];
  gint   index       = 4078;
  gint   flag        = 0;
  gint   raw_index   = 0;
  gint   data_index  = 0;

  if (raw_data_size <= 0 || estimated_size <= 0)
    return FALSE;

  for (gint i = 0; i < index; i++)
    char_array[i] = ' ';

  while (estimated_size > 0)
    {
      if (raw_index > raw_data_size - 1)
        return FALSE;

      if (((flag >>= 1) & 256) == 0)
        flag = raw_data[raw_index++] | 65280;

      if ((flag & 1) != 0)
        {
          guchar value;

          if (raw_index > raw_data_size - 1)
            return FALSE;

          value = raw_data[raw_index++];
          uncompressed_data[data_index++] = value;
          estimated_size--;

          char_array[index] = value;

          index = (index + 1) & 4095;
        }
      else
        {
          gint b[4];
          gint offset;
          gint end_offset;

          if (raw_index > raw_data_size - 2)
            return FALSE;

          b[0] = raw_data[raw_index++];
          b[1] = raw_data[raw_index++];
          b[2] = b[0] | (b[1] & 0xF0) << 4;
          b[3] = (b[1] & 0x0F) + 2;

          offset     = index - b[2];
          end_offset = b[3] + offset;

          if ((b[3] + 1) > (guint32) estimated_size)
            return FALSE;

          for (; offset <= end_offset; offset++)
            {
              gint value = (gint) char_array[offset & 4095];

              uncompressed_data[data_index++] = (guchar) value;
              estimated_size--;

              char_array[index] = (gchar) value;
              index = (index + 1) & 4095;
            }
        }
    }
  return TRUE;
}

static void
convert_from_a1r5g5b5 (gushort  data,
                       guint    index,
                       guchar  *pixel)
{
  data = GUINT16_FROM_LE (data);

  pixel[index * 4]     = (data & 0x1F) << 3;
  pixel[index * 4 + 1] = ((data >> 5) & 0x1F) << 3;
  pixel[index * 4 + 2] = (data >> 10) << 3;

  if (data & 0xF000)
    pixel[index * 4 + 3] = 255;
  else
    pixel[index * 4 + 3] = 0;
}

static void
convert_from_a4r4g4b4 (gushort  data,
                       guint    index,
                       guchar  *pixel)
{
  data = GUINT16_FROM_LE (data);

  pixel[index * 4]     = (data & 0x000F) << 4;
  pixel[index * 4 + 1] = (data & 0x00F0);
  pixel[index * 4 + 2] = (data & 0x0F00) >> 4;
  pixel[index * 4 + 3] = (data & 0xF000) >> 8;
}
