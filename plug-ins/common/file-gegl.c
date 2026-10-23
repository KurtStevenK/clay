/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-gegl.c -- GEGL based file format plug-in
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
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

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define PLUG_IN_BINARY "file-gegl"


typedef struct _FileFormat FileFormat;

struct _FileFormat
{
  const gchar *file_type;
  const gchar *mime_type;
  const gchar *extensions;
  const gchar *magic;

  const gchar *load_proc;
  const gchar *load_blurb;
  const gchar *load_help;
  const gchar *load_op;

  const gchar *export_proc;
  const gchar *export_blurb;
  const gchar *export_help;
  const gchar *export_op;
};


typedef struct _Goat      Goat;
typedef struct _GoatClass GoatClass;

struct _Goat
{
  ClayPlugIn      parent_instance;
};

struct _GoatClass
{
  ClayPlugInClass parent_class;
};


#define GOAT_TYPE  (goat_get_type ())
#define GOAT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOAT_TYPE, Goat))

GType                   goat_get_type         (void);

static GList          * goat_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * goat_create_procedure (ClayPlugIn            *plug_in,
                                               const gchar           *name);

static ClayValueArray * goat_load             (ClayProcedure         *procedure,
                                               ClayRunMode            run_mode,
                                               GFile                 *file,
                                               ClayMetadata          *metadata,
                                               ClayMetadataLoadFlags *flags,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);
static ClayValueArray * goat_export           (ClayProcedure         *procedure,
                                               ClayRunMode            run_mode,
                                               ClayImage             *image,
                                               GFile                 *file,
                                               ClayExportOptions     *options,
                                               ClayMetadata          *metadata,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);

static ClayImage      * load_image            (GFile                 *file,
                                               const gchar           *gegl_op,
                                               GError               **error);
static gboolean         export_image          (GFile                 *file,
                                               const gchar           *gegl_op,
                                               ClayImage             *image,
                                               ClayDrawable          *drawable,
                                               GError               **error);


G_DEFINE_TYPE (Goat, goat, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (GOAT_TYPE)
DEFINE_STD_SET_I18N


static const FileFormat file_formats[] =
{
  {
    N_("Radiance RGBE"),
    "image/vnd.radiance",
    "hdr",
    "0,string,#?",

    "file-rgbe-load",
    N_("Loads files in the RGBE file format"),
    N_("This procedure loads images in the RGBE format, using \"gegl:rgbe-load\""),
    "gegl:rgbe-load",

    "file-rgbe-export",
    N_("Exports files in the RGBE file format"),
    N_("This procedure exports images in the RGBE format, using \"gegl:rgbe-save\""),
    "gegl:rgbe-save",
  },
  {
    N_("OpenEXR image"),
    "image/x-exr",
    "exr",
    "0,lelong,20000630",

    /* no EXR loading (implemented in native Clay plug-in) */
    NULL, NULL, NULL, NULL,

    "file-exr-export",
    N_("Exports files in the OpenEXR file format"),
    N_("This procedure exports images in the OpenEXR format, using \"gegl:exr-save\""),
    "gegl:exr-save"
  }
};


static void
goat_class_init (GoatClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = goat_query_procedures;
  plug_in_class->create_procedure = goat_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
goat_init (Goat *goat)
{
}

static GList *
goat_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;
  gint   i;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];

      if (format->load_proc)
        list = g_list_append (list, g_strdup (format->load_proc));

      if (format->export_proc)
        list = g_list_append (list, g_strdup (format->export_proc));
    }

  return list;
}

static ClayProcedure *
goat_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;
  gint           i;

  for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];

      if (! g_strcmp0 (name, format->load_proc))
        {
          procedure = clay_load_procedure_new (plug_in, name,
                                               CLAY_PDB_PROC_TYPE_PLUGIN,
                                               goat_load,
                                               (gpointer) format, NULL);

          clay_procedure_set_menu_label (procedure, _(format->file_type));

          clay_procedure_set_documentation (procedure,
                                            _(format->load_blurb),
                                            _(format->load_help),
                                            name);

          clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                              format->extensions);
          clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                          format->magic);
        }
      else if (! g_strcmp0 (name, format->export_proc))
        {
          procedure = clay_export_procedure_new (plug_in, name,
                                                 CLAY_PDB_PROC_TYPE_PLUGIN,
                                                 FALSE, goat_export,
                                                 (gpointer) format, NULL);

          clay_procedure_set_image_types (procedure, "*");

          clay_procedure_set_menu_label (procedure, _(format->file_type));

          clay_procedure_set_documentation (procedure,
                                            _(format->export_blurb),
                                            _(format->export_help),
                                            name);

          clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                              format->extensions);

          clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                                  CLAY_EXPORT_CAN_HANDLE_RGB     |
                                                  CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                                  CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                                  CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                                  NULL, NULL, NULL);
        }
    }

  return procedure;
}

static ClayValueArray *
goat_load (ClayProcedure         *procedure,
           ClayRunMode            run_mode,
           GFile                 *file,
           ClayMetadata          *metadata,
           ClayMetadataLoadFlags *flags,
           ClayProcedureConfig   *config,
           gpointer               run_data)
{
  const FileFormat *format = run_data;
  ClayValueArray   *return_vals;
  ClayImage        *image;
  GError           *error  = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, format->load_op, &error);

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
goat_export (ClayProcedure        *procedure,
             ClayRunMode           run_mode,
             ClayImage            *image,
             GFile                *file,
             ClayExportOptions    *options,
             ClayMetadata         *metadata,
             ClayProcedureConfig  *config,
             gpointer              run_data)
{
  const FileFormat  *format = run_data;
  ClayPDBStatusType  status = CLAY_PDB_SUCCESS;
  ClayExportReturn   export = CLAY_EXPORT_IGNORE;
  GList             *drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export = clay_export_options_get_image (options, &image);
  drawables = clay_image_list_layers (image);

  if (! export_image (file, format->export_op, image, drawables->data,
                    &error))
    {
      status = CLAY_PDB_EXECUTION_ERROR;
    }

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_list_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayImage *
load_image (GFile        *file,
            const gchar  *gegl_op,
            GError      **error)
{
  ClayImage         *image;
  ClayLayer         *layer;
  ClayImageType      image_type;
  ClayImageBaseType  base_type;
  ClayPrecision      precision;
  gint               width;
  gint               height;
  GeglNode          *graph;
  GeglNode          *sink;
  GeglNode          *source;
  GeglBuffer        *src_buf  = NULL;
  GeglBuffer        *dest_buf = NULL;
  const Babl        *format;

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  graph = gegl_node_new ();

  source = gegl_node_new_child (graph,
                                "operation", gegl_op,
                                "path",      g_file_peek_path (file),
                                NULL);
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:buffer-sink",
                              "buffer",    &src_buf,
                              NULL);

  gegl_node_link (source, sink);

  gegl_node_process (sink);

  g_object_unref (graph);

  if (! src_buf)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s'"),
                   clay_file_get_utf8_name (file));
      return NULL;
    }

  clay_progress_update (0.33);

  width  = gegl_buffer_get_width (src_buf);
  height = gegl_buffer_get_height (src_buf);
  format = gegl_buffer_get_format (src_buf);

  if (babl_format_is_palette (format))
    {
      base_type = CLAY_INDEXED;

      if (babl_format_has_alpha (format))
        image_type = CLAY_INDEXEDA_IMAGE;
      else
        image_type = CLAY_INDEXED_IMAGE;

      precision = CLAY_PRECISION_U8_NON_LINEAR;
    }
  else
    {
      const Babl *model  = babl_format_get_model (format);
      const Babl *type   = babl_format_get_type (format, 0);
      gboolean    linear = TRUE;

      if (model == babl_model ("Y")  ||
          model == babl_model ("Y'") ||
          model == babl_model ("YA") ||
          model == babl_model ("Y'A"))
        {
          base_type = CLAY_GRAY;

          if (babl_format_has_alpha (format))
            image_type = CLAY_GRAYA_IMAGE;
          else
            image_type = CLAY_GRAY_IMAGE;

          if (model == babl_model ("Y'") ||
              model == babl_model ("Y'A"))
            linear = FALSE;
        }
      else
        {
          base_type = CLAY_RGB;

          if (babl_format_has_alpha (format))
            image_type = CLAY_RGBA_IMAGE;
          else
            image_type = CLAY_RGB_IMAGE;

          if (model == babl_model ("R'G'B'") ||
              model == babl_model ("R'G'B'A"))
            linear = FALSE;
        }

      if (linear)
        {
          if (type == babl_type ("u8"))
            precision = CLAY_PRECISION_U8_LINEAR;
          else if (type == babl_type ("u16"))
            precision = CLAY_PRECISION_U16_LINEAR;
          else if (type == babl_type ("u32"))
            precision = CLAY_PRECISION_U32_LINEAR;
          else if (type == babl_type ("half"))
            precision = CLAY_PRECISION_HALF_LINEAR;
          else
            precision = CLAY_PRECISION_FLOAT_LINEAR;
        }
      else
        {
          if (type == babl_type ("u8"))
            precision = CLAY_PRECISION_U8_NON_LINEAR;
          else if (type == babl_type ("u16"))
            precision = CLAY_PRECISION_U16_NON_LINEAR;
          else if (type == babl_type ("u32"))
            precision = CLAY_PRECISION_U32_NON_LINEAR;
          else if (type == babl_type ("half"))
            precision = CLAY_PRECISION_HALF_NON_LINEAR;
          else
            precision = CLAY_PRECISION_FLOAT_NON_LINEAR;
        }
    }


  image = clay_image_new_with_precision (width, height,
                                         base_type, precision);
  layer = clay_layer_new (image,
                          _("Background"),
                          width, height,
                          image_type,
                          100,
                          clay_image_get_default_new_layer_mode (image));
  clay_image_insert_layer (image, layer, NULL, 0);

  dest_buf = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  clay_progress_update (0.66);

  gegl_buffer_copy (src_buf, NULL, GEGL_ABYSS_NONE, dest_buf, NULL);

  g_object_unref (src_buf);
  g_object_unref (dest_buf);

  clay_progress_update (1.0);

  return image;
}

static gboolean
export_image (GFile         *file,
              const gchar   *gegl_op,
              ClayImage     *image,
              ClayDrawable  *drawable,
              GError       **error)
{
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *sink;
  GeglBuffer *src_buf;

  src_buf = clay_drawable_get_buffer (drawable);

  graph = gegl_node_new ();

  source = gegl_node_new_child (graph,
                                "operation", "gegl:buffer-source",
                                "buffer",    src_buf,
                                NULL);
  sink = gegl_node_new_child (graph,
                              "operation", gegl_op,
                              "path",      g_file_peek_path (file),
                              NULL);

  gegl_node_link (source, sink);

  gegl_node_process (sink);

  g_object_unref (graph);
  g_object_unref (src_buf);

  return TRUE;
}
