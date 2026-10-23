/*   xyz.kainzmayer.ClayDesign
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Farbfeld Image Format plug-in
 *
 *   Copyright (C) 2023 Alex S.
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


#define LOAD_PROC      "file-farbfeld-load"
#define EXPORT_PROC    "file-farbfeld-export"
#define PLUG_IN_BINARY "file-farbfeld"
#define PLUG_IN_ROLE   "clay-file-farbfeld"


typedef struct _Farbfeld      Farbfeld;
typedef struct _FarbfeldClass FarbfeldClass;

struct _Farbfeld
{
  ClayPlugIn      parent_instance;
};

struct _FarbfeldClass
{
  ClayPlugInClass parent_class;
};


#define FARBFELD_TYPE  (farbfeld_get_type ())
#define FARBFELD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FARBFELD_TYPE, Farbfeld))

GType                   farbfeld_get_type         (void);


static GList          * farbfeld_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * farbfeld_create_procedure (ClayPlugIn            *plug_in,
                                                   const gchar           *name);

static ClayValueArray * farbfeld_load             (ClayProcedure         *procedure,
                                                   ClayRunMode            run_mode,
                                                   GFile                 *file,
                                                   ClayMetadata          *metadata,
                                                   ClayMetadataLoadFlags *flags,
                                                   ClayProcedureConfig   *config,
                                                   gpointer               run_data);
static ClayValueArray * farbfeld_export           (ClayProcedure         *procedure,
                                                   ClayRunMode            run_mode,
                                                   ClayImage             *image,
                                                   GFile                 *file,
                                                   ClayExportOptions     *options,
                                                   ClayMetadata          *metadata,
                                                   ClayProcedureConfig   *config,
                                                   gpointer               run_data);

static ClayImage      * load_image                (GFile                 *file,
                                                   GObject               *config,
                                                   ClayRunMode            run_mode,
                                                   GError               **error);
static gboolean         export_image              (GFile                 *file,
                                                   ClayImage             *image,
                                                   ClayDrawable          *drawable,
                                                   GError               **error);


G_DEFINE_TYPE (Farbfeld, farbfeld, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (FARBFELD_TYPE)
DEFINE_STD_SET_I18N


static void
farbfeld_class_init (FarbfeldClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = farbfeld_query_procedures;
  plug_in_class->create_procedure = farbfeld_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
farbfeld_init (Farbfeld *farbfeld)
{
}

static GList *
farbfeld_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static ClayProcedure *
farbfeld_create_procedure (ClayPlugIn  *plug_in,
                           const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           farbfeld_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("Farbfeld"));

      clay_procedure_set_documentation (procedure,
                                        _("Load file in the Farbfeld file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "ff");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,farbfeld");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, farbfeld_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Farbfeld"));

      clay_procedure_set_documentation (procedure,
                                        _("Export image in the Farbfeld file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "ff");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);
    }

  return procedure;
}

static ClayValueArray *
farbfeld_load (ClayProcedure         *procedure,
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

static ClayValueArray *
farbfeld_export (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 GFile                *file,
                 ClayExportOptions    *options,
                 ClayMetadata         *metadata,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  ClayPDBStatusType  status = CLAY_PDB_SUCCESS;
  ClayExportReturn   export = CLAY_EXPORT_IGNORE;
  GList             *drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export = clay_export_options_get_image (options, &image);
  drawables = clay_image_list_layers (image);

  if (! export_image (file, image, drawables->data, &error))
    status = CLAY_PDB_EXECUTION_ERROR;

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_list_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayImage *
load_image (GFile        *file,
            GObject      *config,
            ClayRunMode   run_mode,
            GError      **error)
{
  ClayImage  *image  = NULL;
  ClayLayer  *layer;
  GeglBuffer *buffer;
  guint16    *pixels;
  guchar      magic_number[8];
  guint32     width;
  guint32     height;
  gsize       row_size;
  const Babl *format = babl_format ("R'G'B'A u16");
  FILE       *fp;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Load the header */
  if (! fread (magic_number, 8, 1, fp)          ||
      ! fread (&width, sizeof (guint32), 1, fp) ||
      ! fread (&height, sizeof (guint32), 1, fp))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Failed to read Farbfeld header"));
      fclose (fp);
      return NULL;
    }

  /* Header information is stored in Big-Endian format */
  width = GUINT32_FROM_BE (width);
  height = GUINT32_FROM_BE (height);

  if (width > CLAY_MAX_IMAGE_SIZE  ||
      height > CLAY_MAX_IMAGE_SIZE ||
      ! g_size_checked_mul (&row_size, width, (sizeof (guint16) * 4)))
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   width, height);
      fclose (fp);
      return NULL;
    }

  image = clay_image_new_with_precision (width, height, CLAY_RGB,
                                         CLAY_PRECISION_U16_NON_LINEAR);

  layer = clay_layer_new (image, _("Background"), width, height,
                          CLAY_RGBA_IMAGE, 100,
                          clay_image_get_default_new_layer_mode (image));
  clay_image_insert_layer (image, layer, NULL, 0);

  pixels = g_try_malloc (row_size);
  if (pixels == NULL)
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the "
                     "operation."));
      fclose (fp);
      return NULL;
    }

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));
  for (gint i = 0; i < height; i++)
    {
      if (! fread (pixels, row_size, 1, fp))
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Premature end of Farbfeld pixel data"));
          return NULL;
        }

      /* Pixels are also stored in Big-Endian format */
      for (gint j = 0; j < (width * 4); j++)
        pixels[j] = GUINT16_FROM_BE (pixels[j]);

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, i, width, 1), 0,
                       format, pixels, GEGL_AUTO_ROWSTRIDE);
    }
  g_free (pixels);

  fclose (fp);
  g_object_unref (buffer);

  return image;
}

static gboolean
export_image (GFile         *file,
              ClayImage     *image,
              ClayDrawable  *drawable,
              GError       **error)
{
  FILE       *fp;
  GeglBuffer *buffer;
  guint16    *pixels;
  const Babl *format = babl_format ("R'G'B'A u16");
  gchar      *magic_number;
  guint32     image_width;
  guint32     image_height;
  guint32     export_width;
  guint32     export_height;

  clay_progress_init_printf (_("Exporting '%s'"),
                             clay_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  buffer = clay_drawable_get_buffer (drawable);

  image_width = gegl_buffer_get_width (buffer);
  image_height = gegl_buffer_get_height (buffer);
  /* Farbfeld values are Big-Endian */
  export_width = GUINT32_TO_BE (image_width);
  export_height = GUINT32_TO_BE (image_height);

  /* Write header */
  magic_number = "farbfeld";
  for (gint i = 0; i < 8; i++)
    fputc (magic_number[i], fp);
  fwrite ((gchar *) &export_width, 1, 4, fp);
  fwrite ((gchar *) &export_height, 1, 4, fp);

  /* Write pixel data */
  for (gint i = 0; i < image_height; i++)
    {
      pixels = g_malloc (image_width * sizeof (guint16) * 4);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, image_width, 1),
                       1.0, format, pixels,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (gint j = 0; j < (image_width * 4); j++)
        {
          pixels[j] = GUINT16_TO_BE (pixels[j]);
          fwrite ((gchar *) &pixels[j], 1, 2, fp);
        }

      g_free (pixels);
      clay_progress_update (i / (gdouble) image_height);
    }

  clay_progress_update (1.0);

  fclose (fp);

  if (buffer)
    g_object_unref (buffer);

  return TRUE;
}
