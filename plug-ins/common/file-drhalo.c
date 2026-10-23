/*   xyz.kainzmayer.ClayDesign
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Dr. Halo plug-in
 *
 *   Copyright (C) 2026 Jacob Boerema.
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


#define LOAD_PROC      "file-drhalo-cut-load"
#define PLUG_IN_BINARY "file-drhalo"
#define PLUG_IN_ROLE   "clay-file-drhalo"

typedef struct _Drhalo      Drhalo;
typedef struct _DrhaloClass DrhaloClass;

struct _Drhalo
{
  ClayPlugIn      parent_instance;
};

struct _DrhaloClass
{
  ClayPlugInClass parent_class;
};


#define DRHALO_TYPE  (drhalo_get_type ())
#define DRHALO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DRHALO_TYPE, Drhalo))

GType                   drhalo_get_type         (void);


static GList          * drhalo_query_procedures   (ClayPlugIn            *plug_in);
static ClayProcedure  * drhalo_create_procedure   (ClayPlugIn            *plug_in,
                                                   const gchar           *name);

static ClayValueArray * drhalo_load               (ClayProcedure         *procedure,
                                                   ClayRunMode            run_mode,
                                                   GFile                 *file,
                                                   ClayMetadata          *metadata,
                                                   ClayMetadataLoadFlags *flags,
                                                   ClayProcedureConfig   *config,
                                                   gpointer               run_data);

static ClayImage      * load_image                (GFile                 *file,
                                                   GObject               *config,
                                                   ClayRunMode            run_mode,
                                                   GError               **error);


G_DEFINE_TYPE (Drhalo, drhalo, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (DRHALO_TYPE)
DEFINE_STD_SET_I18N

typedef struct _HaloHeader
{
  guint16   Width;          /* 00h   Image Width in Pixels */
  guint16   Height;         /* 02h   Image Height in Scan Lines */
  guint16   Reserved;       /* 04h   Reserved Field (set to 0) */
} HaloHeader;

static void
drhalo_class_init (DrhaloClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = drhalo_query_procedures;
  plug_in_class->create_procedure = drhalo_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
drhalo_init (Drhalo *drhalo)
{
}

static GList *
drhalo_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static ClayProcedure *
drhalo_create_procedure (ClayPlugIn  *plug_in,
                         const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           drhalo_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("Dr. Halo CUT image"));

      clay_procedure_set_documentation (procedure,
                                        _("Load Dr. Halo CUT image format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Jacob Boerema",
                                      "Jacob Boerema",
                                      "2026");
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-cut");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "cut");
    }

  return procedure;
}

static ClayValueArray *
drhalo_load (ClayProcedure         *procedure,
             ClayRunMode            run_mode,
             GFile                 *file,
             ClayMetadata          *metadata,
             ClayMetadataLoadFlags *flags,
             ClayProcedureConfig   *config,
             gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), run_mode, &error);

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
load_image (GFile        *file,
            GObject      *config,
            ClayRunMode   run_mode,
            GError      **error)
{
  ClayImage  *image      = NULL;
  ClayLayer  *layer;
  GeglBuffer *buffer;
  guchar     *pixels     = NULL;
  guchar     *line_data  = NULL;
  FILE       *fp;
  HaloHeader  header;
  gint        pixel;
  gint        max_pixels;
  gint        y;
  gint        highest_index = 0;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fseek (fp, 0, SEEK_SET);

  if (fread (&header, sizeof (HaloHeader), 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Cannot open file for reading: %s\n"),
                   g_file_peek_path (file));
      fclose (fp);
      return NULL;
    }

  header.Width  = GUINT16_FROM_LE (header.Width);
  header.Height = GUINT16_FROM_LE (header.Height);

  if (header.Width == 0 || header.Height == 0)
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, 0,
                   _("Invalid dimensions: %d x %d"),
                   (gint) header.Width, (gint) header.Height * 2);
      fclose (fp);
      return NULL;
    }

  /* FIXME: Check if there is a .PAL palette file with the same name.
   * If found: load the palette and use that for this image
   * and set the mode to INDEXED in that case.
   * NB Check KISS image plug-in if there are similarities...
   */

  image = clay_image_new_with_precision (header.Width, header.Height, CLAY_GRAY,
                                         CLAY_PRECISION_U8_NON_LINEAR);

  layer = clay_layer_new (image, _("Background"), header.Width, header.Height,
                          CLAY_GRAY_IMAGE,
                          100, clay_image_get_default_new_layer_mode (image));

  clay_image_insert_layer (image, layer, NULL, 0);

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  max_pixels = (gint) header.Width * header.Height;
  pixels     = g_try_malloc0 (max_pixels);
  /* We don't know the size of a compressed line in advance. Assume 3 times width */
  line_data  = g_try_malloc0 (3 * header.Width);

  if (! pixels || ! line_data)
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the "
                     "operation."));
      fclose (fp);
      g_free (pixels);
      g_free (line_data);
      return NULL;
    }

  pixel = 0;
  y     = 0;
  while (y < header.Height && pixel < max_pixels)
    {
      guint16 line_size;
      gint8   run_count, value;
      gint    i, x;

      if (fread (&line_size, 2, 1, fp) == 0)
        {
          /* Only return if no lines have been processed yet.
           * Otherwise, just show partial image with a message.
           */
          if (y == 0)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              fclose (fp);
              g_free (pixels);
              g_free (line_data);
              return NULL;
            }
          clay_message (_("Error reading data. Image may be corrupt."));
          break;
        }

      line_size = GUINT16_FROM_LE (line_size);

      if (line_size > 3 * header.Width ||
          fread (line_data, line_size, 1, fp) == 0)
        {
          /* Only return if no lines have been processed yet.
           * Otherwise, just show partial image with a message.
           */
          if (y == 0)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              fclose (fp);
              g_free (pixels);
              g_free (line_data);
              return NULL;
            }
          clay_message (_("Error reading data. Image may be corrupt."));
          break;
        }

      i = 0;
      x = 0;
      while (i < line_size)
        {
          run_count = line_data[i++];
          if (run_count == 0) /* End of line */
            {
              break;
            }
          else if (x >= header.Width || i >= line_size)
            {
              clay_message (_("Invalid data: image may be corrupt."));
              break;
            }

          value = run_count & 0x7f;

          if ((run_count & 0x80) == 0x80)
            {
              /* Repeat next byte value times*/
              guchar repeat_val = line_data[i++];

              if (repeat_val > highest_index)
                highest_index = repeat_val;

              if (pixel + value >= max_pixels)
                {
                  clay_message (_("Invalid data: image may be corrupt."));
                  pixel += value;
                  break;
                }
              memset (&pixels[pixel], repeat_val, value);
              pixel += value;
              x += value;
            }
          else /* if ((run_count & 0x80) == 0) */
            {
              if (i + value >= line_size || pixel + value >= max_pixels)
                {
                  clay_message (_("Invalid data: image may be corrupt."));
                  pixel += value;
                  break;
                }
              for (gint j = 0; j < value; j ++)
                {
                  pixels[pixel++] = line_data[i++];
                  if (line_data[i - 1] > highest_index)
                    highest_index = line_data[i - 1];
                }
              x += value;
            }
        }

      y++;
    }

  fclose (fp);

  /* If we don't have a palette (grayscale), and highest_index == 1,
   * we have apparently a B/W image: Set the grayscale values to 255.
   */
  if (highest_index == 1)
    {
      for (gint pix = 0; pix < (gint) header.Width * header.Height; pix++)
        {
          if (pixels[pix] == 1)
            pixels[pix] = 255;
        }
    }
  else if (highest_index < 128)
    {
      gint shift;

      /* If not the entire 8-bit width is used, upscale the pixel values. */
      if (highest_index < 4)
        shift = 6;
      else if (highest_index < 8)
        shift = 5;
      else if (highest_index < 16)
        shift = 4;
      else if (highest_index < 32)
        shift = 3;
      else if (highest_index < 64)
        shift = 2;
      else
        shift = 1;

      for (gint pix = 0; pix < (gint) header.Width * header.Height; pix++)
        pixels[pix] = (guchar) ((((guint16) pixels[pix] + 1) << shift) - 1);
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, header.Width, header.Height), 0,
                   NULL, pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);
  g_free (pixels);
  g_free (line_data);

  return image;
}
