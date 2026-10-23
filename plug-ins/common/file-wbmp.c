/*
 * xyz.kainzmayer.ClayDesign
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

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"

#define LOAD_PROC      "file-wbmp-load"
#define LOAD_OTB_PROC  "file-otb-load"
#define PLUG_IN_BINARY "file-wbmp"
#define PLUG_IN_ROLE   "clay-file-wbmp"

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)

typedef struct _Wbmp      Wbmp;
typedef struct _WbmpClass WbmpClass;

struct _Wbmp
{
  ClayPlugIn      parent_instance;
};

struct _WbmpClass
{
  ClayPlugInClass parent_class;
};

#define WBMP_TYPE  (wbmp_get_type ())
#define WBMP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WBMP_TYPE, Wbmp))

GType                   wbmp_get_type         (void);

static GList          * wbmp_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * wbmp_create_procedure (ClayPlugIn            *plug_in,
                                               const gchar           *name);

static ClayValueArray * wbmp_load             (ClayProcedure         *procedure,
                                               ClayRunMode            run_mode,
                                               GFile                 *file,
                                               ClayMetadata          *metadata,
                                               ClayMetadataLoadFlags *flags,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);
static ClayValueArray * otb_load              (ClayProcedure         *procedure,
                                               ClayRunMode            run_mode,
                                               GFile                 *file,
                                               ClayMetadata          *metadata,
                                               ClayMetadataLoadFlags *flags,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);

ClayImage             * load_wbmp_image       (GFile                 *file,
                                               GError               **error);
ClayImage             * load_otb_image        (GFile                 *file,
                                               GError               **error);

static ClayImage      * read_image            (FILE                  *fd,
                                               GFile                 *file,
                                               gint                   width,
                                               gint                   height,
                                               GError               **error);

G_DEFINE_TYPE (Wbmp, wbmp, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (WBMP_TYPE)
DEFINE_STD_SET_I18N

static void
wbmp_class_init (WbmpClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = wbmp_query_procedures;
  plug_in_class->create_procedure = wbmp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
wbmp_init (Wbmp *wmp)
{
}

static GList *
wbmp_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_OTB_PROC));

  return list;
}

static ClayProcedure *
wbmp_create_procedure (ClayPlugIn  *plug_in,
                       const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           wbmp_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Wireless BMP image"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads files of Wireless BMP file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Kevin Toyle",
                                      "Kevin Toyle",
                                      "2022");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/vnd.wap.wbmp");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "wbmp");
    }
  else if (! strcmp (name, LOAD_OTB_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           otb_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Nokia Over The Air Bitmap image"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads files in Nokia Over The Air Bitmap file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alx Sa",
                                      "Alx Sa",
                                      "2025");

      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "otb");
    }

  return procedure;
}

static ClayValueArray *
wbmp_load (ClayProcedure         *procedure,
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

  image = load_wbmp_image (file, &error);

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

static ClayValueArray *
otb_load (ClayProcedure         *procedure,
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

  image = load_otb_image (file, &error);

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

ClayImage *
load_wbmp_image (GFile   *file,
                 GError **error)
{
  FILE      *fd;
  ClayImage *image  = NULL;
  gint       width  = 0;
  gint       height = 0;
  gint8      magic;
  guchar     value;

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  fd = g_fopen (g_file_peek_path (file), "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      goto out;
    }

  /* Checking the type field to make sure it is 0. */
  if (! ReadOK (fd, &magic, 1) || magic != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s': Invalid WBMP type value"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  /* Checking the fixed header field to make sure it is 0 */
  if (! ReadOK (fd, &magic, 1) || magic != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s': Unsupported WBMP fixed header value"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  /* The width and height are stored as uintvar values */
  while (ReadOK (fd, &value, 1))
    {
      width = (width << 7) | (value & 0x7F);
      if (value >> 7 != 1)
        break;
    }
  while (ReadOK (fd, &value, 1))
    {
      height = (height << 7) | (value & 0x7F);
      if (value >> 7 != 1)
        break;
    }

  if (width <= 0 || height <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a valid WBMP file"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  image = read_image (fd, file, width, height, error);

out:
  if (fd)
    fclose (fd);

  return image;
}

ClayImage *
load_otb_image (GFile   *file,
                GError **error)
{
  FILE         *fd;
  ClayImage    *image   = NULL;
  ClayLayer    *layer;
  GeglBuffer   *buffer;
  guchar       *dest;
  const guchar  mono[6] = { 255, 255, 255, 0, 0, 0, };
  guint8        magic;
  gushort       width   = 0;
  gushort       height  = 0;
  guint         count   = 0;
  gsize         total_size;
  guchar        value;

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  fd = g_fopen (g_file_peek_path (file), "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      goto out;
    }

  /* Checking the type field to make sure it is 0. */
  if (! ReadOK (fd, &magic, 1))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s': Invalid OTA type value"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  /* If magic is 0, we read width, height, and then depth field.
   * If it's anything else, we may read extension byte first */
  if (magic == 0)
    {
      if (! ReadOK (fd, &width, 1)  ||
          ! ReadOK (fd, &height, 1) ||
          ! ReadOK (fd, &value, 1))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': Invalid OTA header value"),
                       clay_file_get_utf8_name (file));
          goto out;
        }
    }
  else
    {
      gint bytes_to_read = 1;

      /* If extension byte is set */
      if (magic & (1 << 7) &&
          ! ReadOK (fd, &value, 1))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': Invalid OTA header value"),
                       clay_file_get_utf8_name (file));
          goto out;
        }

      /* If width and height are 16 bit integers */
      if (magic & (1 << 4))
        bytes_to_read = 2;

      if (! ReadOK (fd, &width, bytes_to_read)  ||
          ! ReadOK (fd, &height, bytes_to_read) ||
          ! ReadOK (fd, &value, 1))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': Invalid OTA header value"),
                       clay_file_get_utf8_name (file));
          goto out;
        }
      width  = GUINT16_FROM_BE (width);
      height = GUINT16_FROM_BE (height);
    }

  if (width == 0 || height == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                    _("'%s': Invalid dimensions %u x %u"),
                    clay_file_get_utf8_name (file),
                    width, height);
      goto out;
    }

  image = clay_image_new (width, height, CLAY_INDEXED);
  layer = clay_layer_new (image, _("Background"), width, height,
                          CLAY_INDEXED_IMAGE, 100,
                          clay_image_get_default_new_layer_mode (image));

  clay_palette_set_colormap (clay_image_get_palette (image),
                             babl_format ("R'G'B' u8"), (guint8 *) mono, 6);
  clay_image_insert_layer (image, layer, NULL, 0);

  total_size = (gsize) width * height;
  dest       = g_malloc0 (total_size);

  while (ReadOK (fd, &value, 1))
    {
      for (gint i = 7; i >= 0; i--)
        {
          if (value & (1 << i))
            dest[count] = 1;

          count++;
          if (count >= (total_size))
            break;

          if ((count % 5) == 0)
            clay_progress_update ((gdouble) count / (gdouble) total_size);
        }
      if (count >= (total_size))
        break;
    }

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0, NULL, dest,
                   GEGL_AUTO_ROWSTRIDE);
  g_object_unref (buffer);

  g_free (dest);

out:
  if (fd)
    fclose (fd);

  return image;
}

/* Code referenced from /plug-ins/file-bmp/bmp-load.c */
static ClayImage *
read_image (FILE    *fd,
            GFile   *file,
            gint     width,
            gint     height,
            GError **error)
{
  const guchar mono[6] = { 0, 0, 0, 255, 255, 255 };
  guchar       v;
  gint         xpos    = 0;
  gint         ypos    = 0;
  ClayImage   *image;
  ClayLayer   *layer;
  GeglBuffer  *buffer;
  guchar      *dest, *temp;
  gint         i, cur_progress, max_progress;
  size_t       n_read;

  /* Make a new image in Clay */
  if ((width < 0) || (width > CLAY_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image width: %d"), width);
      return NULL;
    }

  if ((height < 0) || (height > CLAY_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image height: %d"), height);
      return NULL;
    }

  image = clay_image_new (width, height, CLAY_INDEXED);
  layer = clay_layer_new (image, _("Background"), width, height,
                          CLAY_INDEXED_IMAGE, 100,
                          clay_image_get_default_new_layer_mode (image));

  clay_palette_set_colormap (clay_image_get_palette (image),
                             babl_format ("R'G'B' u8"), (guint8 *) mono, 6);

  clay_image_insert_layer (image, layer, NULL, 0);

  dest = g_malloc0 ((gsize) width * height);

  ypos = 0;

  cur_progress = 0;
  max_progress = height;

  while ((n_read = ReadOK (fd, &v, 1)) != 0)
    {
      for (i = 1; (i <= 8) && (xpos < width); i++, xpos++)
        {
          temp = dest + (ypos * width) + xpos;
          *temp = (v & (((1 << 1) - 1) << (8 - i))) >> (8 - i);
        }

      if (xpos == width)
        {
          if (ypos == height - 1)
            break;

          ypos++;
          xpos = 0;

          cur_progress++;
          if ((cur_progress % 5) == 0)
            clay_progress_update ((gdouble) cur_progress / (gdouble) max_progress);
        }

      if (ypos > height - 1)
        break;
    }

  if (n_read == 0)
      g_warning (_("Read failure at position %u. Possibly corrupt image."), ypos * width + xpos);

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0, NULL, dest,
                   GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (dest);

  return image;
}
