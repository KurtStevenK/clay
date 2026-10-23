/*   xyz.kainzmayer.ClayDesign
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Quite OK Image (QOI) plug-in
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

#define QOI_IMPLEMENTATION
#include "qoi.h"

#include "libclay/stdplugins-intl.h"


#define LOAD_PROC      "file-qoi-load"
#define EXPORT_PROC    "file-qoi-export"
#define PLUG_IN_BINARY "file-qoi"
#define PLUG_IN_ROLE   "clay-file-qoi"

typedef struct _Qoi      Qoi;
typedef struct _QoiClass QoiClass;

struct _Qoi
{
  ClayPlugIn      parent_instance;
};

struct _QoiClass
{
  ClayPlugInClass parent_class;
};


#define QOI_TYPE  (qoi_get_type ())
#define QOI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), QOI_TYPE, Qoi))

GType                   qoi_get_type         (void);


static GList          * qoi_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * qoi_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * qoi_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * qoi_export           (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              ClayImage             *image,
                                              GFile                 *file,
                                              ClayExportOptions     *options,
                                              ClayMetadata          *metadata,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);

static ClayImage      * load_image           (GFile                 *file,
                                              GObject               *config,
                                              ClayRunMode            run_mode,
                                              GError               **error);
static gboolean         export_image         (GFile                 *file,
                                              ClayImage             *image,
                                              ClayDrawable          *drawable,
                                              GError               **error);


G_DEFINE_TYPE (Qoi, qoi, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (QOI_TYPE)
DEFINE_STD_SET_I18N


static void
qoi_class_init (QoiClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = qoi_query_procedures;
  plug_in_class->create_procedure = qoi_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
qoi_init (Qoi *qoi)
{
}

static GList *
qoi_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static ClayProcedure *
qoi_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           qoi_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("Quite OK Image"));

      clay_procedure_set_documentation (procedure,
                                        _("Load file in the QOI file format"),
                                        _("Load file in the QOI file format "
                                          "(Quite OK Image)"),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/qoi");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "qoi");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,qoif");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, qoi_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Quite OK Image"));

      clay_procedure_set_documentation (procedure,
                                        _("Export image in the QOI file format"),
                                        _("Export image in the QOI file format "
                                          "(Quite OK Image)"),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/qoi");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "qoi");

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
qoi_load (ClayProcedure         *procedure,
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
qoi_export (ClayProcedure        *procedure,
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
  qoi_desc    desc;
  void       *pixels = NULL;
  const Babl *format = NULL;
  FILE       *fp;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fclose (fp);

  pixels = qoi_read (g_file_peek_path (file), &desc, 0);

  if (! pixels)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Failed to read QOI file"));
      return NULL;
    }

  image = clay_image_new_with_precision (desc.width, desc.height, CLAY_RGB,
                                         desc.colorspace ?
                                         CLAY_PRECISION_U8_LINEAR :
                                         CLAY_PRECISION_U8_NON_LINEAR);

  layer = clay_layer_new (image, _("Background"), desc.width, desc.height,
                          (desc.channels == 4) ? CLAY_RGBA_IMAGE : CLAY_RGB_IMAGE,
                          100, clay_image_get_default_new_layer_mode (image));
  clay_image_insert_layer (image, layer, NULL, 0);

  format = babl_format ((desc.channels == 4) ? "R'G'B'A u8" : "R'G'B' u8");

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, desc.width, desc.height), 0,
                   format, pixels, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);
  g_free (pixels);

  return image;
}

static gboolean
export_image (GFile         *file,
              ClayImage     *image,
              ClayDrawable  *drawable,
              GError       **error)
{
  GeglBuffer *buffer;
  const Babl *format;
  qoi_desc    desc;
  void       *pixels = NULL;
  gsize       pixel_size;
  gboolean    has_alpha;
  gint        success;

  has_alpha = clay_drawable_has_alpha (drawable);

  buffer = clay_drawable_get_buffer (drawable);

  desc.width    = gegl_buffer_get_width  (buffer);
  desc.height   = gegl_buffer_get_height (buffer);
  desc.channels = has_alpha ? 4 : 3;
  switch (clay_image_get_precision (image))
    {
      case CLAY_PRECISION_U8_LINEAR:
      case CLAY_PRECISION_U16_LINEAR:
      case CLAY_PRECISION_U32_LINEAR:
      case CLAY_PRECISION_HALF_LINEAR:
      case CLAY_PRECISION_FLOAT_LINEAR:
      case CLAY_PRECISION_DOUBLE_LINEAR:
        desc.colorspace = QOI_LINEAR;
        break;

      default:
        desc.colorspace = QOI_SRGB;
        break;
    }

  clay_progress_init_printf (_("Exporting '%s'"),
                             clay_file_get_utf8_name (file));

  format = babl_format (has_alpha ? "R'G'B'A u8" : "R'G'B' u8");

  if (! g_size_checked_mul (&pixel_size, desc.width, desc.height)   ||
      ! g_size_checked_mul (&pixel_size, pixel_size, desc.channels) ||
      ! (pixels = (guchar *) g_try_malloc0 (pixel_size)))
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, desc.width, desc.height),
                   1.0, format, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  success = qoi_write (g_file_peek_path (file), pixels, &desc);

  g_object_unref (buffer);
  g_free (pixels);

  if (! success)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   clay_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}
