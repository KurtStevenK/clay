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

#include <glib/gstdio.h>
#include <libraw.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"

#define PLUG_IN_BINARY    "file-libraw"
#define LOAD_THUMB_PROC   "file-libraw-load-thumb"

typedef struct _FileLibRaw      FileLibRaw;
typedef struct _FileLibRawClass FileLibRawClass;

struct _FileLibRaw
{
  ClayPlugIn      parent_instance;
};

struct _FileLibRawClass
{
  ClayPlugInClass parent_class;
};


#define FILELIBRAW_TYPE  (file_libraw_get_type ())
#define FILELIBRAW(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FILELIBRAW_TYPE, FileLibRaw))

GType                   file_libraw_get_type         (void);

static GList          * file_libraw_init_procedures  (ClayPlugIn            *plug_in);
static ClayProcedure  * file_libraw_create_procedure (ClayPlugIn            *plug_in,
                                                      const gchar           *name);

static ClayValueArray * file_libraw_load             (ClayProcedure         *procedure,
                                                      ClayRunMode            run_mode,
                                                      GFile                 *file,
                                                      ClayMetadata          *metadata,
                                                      ClayMetadataLoadFlags *flags,
                                                      ClayProcedureConfig   *config,
                                                      gpointer               run_data);
static ClayValueArray * file_libraw_load_thumb       (ClayProcedure         *procedure,
                                                      GFile                 *file,
                                                      gint                   size,
                                                      ClayProcedureConfig   *config,
                                                      gpointer               run_data);

static ClayImage      * load_image                   (GFile                 *file,
                                                      libraw_data_t         *raw_info,
                                                      ClayRunMode            run_mode,
                                                      ClayProcedureConfig   *config,
                                                      GError               **error);
static ClayImage      * load_thumbnail_image         (GFile                 *file,
                                                      gint                   thumb_size,
                                                      GError               **error);


static gboolean         load_dialog                  (ClayProcedure         *procedure,
                                                      GObject               *config,
                                                      libraw_data_t         *raw_info);


G_DEFINE_TYPE (FileLibRaw, file_libraw, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (FILELIBRAW_TYPE)
DEFINE_STD_SET_I18N


static void
file_libraw_class_init (FileLibRawClass *klass)
{
  ClayPlugInClass *plug_in_class  = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = file_libraw_init_procedures;
  plug_in_class->create_procedure = file_libraw_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
file_libraw_init (FileLibRaw *file_libraw)
{
}

static GList *
file_libraw_init_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));

  for (gint i = 0; i < G_N_ELEMENTS (file_formats); i++)
    {
      const FileFormat *format = &file_formats[i];
      gchar            *load_proc;

      load_proc = g_strdup_printf (format->load_proc_format, "libraw");

      list = g_list_append (list, load_proc);
    }

  return list;
}

static ClayProcedure *
file_libraw_create_procedure (ClayPlugIn  *plug_in,
                              const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                file_libraw_load_thumb,
                                                NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        _("Load thumbnail from a raw image"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alx Sa",
                                      "Alx Sa",
                                      "2026");
    }
  else
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;
          gchar            *load_blurb;
          gchar            *load_help;

          load_proc = g_strdup_printf (format->load_proc_format, "libraw");

          if (strcmp (name, load_proc))
            {
              g_free (load_proc);
              continue;
            }

          load_blurb = g_strdup_printf (format->load_blurb_format,
                                        "libraw");
          load_help  = g_strdup_printf (format->load_help_format,
                                        "libraw");

          procedure = clay_load_procedure_new (plug_in, name,
                                               CLAY_PDB_PROC_TYPE_PLUGIN,
                                               file_libraw_load,
                                               (gpointer) format, NULL);

          clay_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          clay_procedure_set_attribution (procedure,
                                          "Alx Sa",
                                          "Alx Sa",
                                          "2026");

          clay_procedure_add_boolean_argument (procedure, "enable-interpolation",
                                               _("Enable interpolation"),
                                               _("Enable denoising and demosaicing steps"),
                                               TRUE,
                                               G_PARAM_READWRITE);

          /* The list of denoising and demosaicing algorithms are
           * hardcoded in libraw, and there doesn't even seem to be any
           * enum values in the C API.
           */

          clay_procedure_add_choice_argument (procedure, "noise-reduction",
                                              _("Noise reduction"),
                                              _("Controls FBDD (Fake Before Demosaicing Denoising) noise reduction before demosaic"),
                                              clay_choice_new_with_values ("disabled",   0, _("Disabled"),             NULL,
                                                                           "light-fbdd", 1, _("Light FBDD reduction"), NULL,
                                                                           "full-fbdd",  2, _("Full FBDD reduction"),  NULL,
                                                                           NULL),
                                              "disabled", G_PARAM_READWRITE);

          clay_procedure_add_choice_argument (procedure, "demosaicing",
                                              _("Demosaicing"),
                                              _("Interpolation method to reconstruct a matrix of colored pixels"
                                                " from raw sensor data"),
                                              clay_choice_new_with_values ("linear",       0,  _("Linear"),       NULL,
                                                                           "vng",          1,  _("VNG"),          NULL,
                                                                           "ppg",          2,  _("PPG"),          NULL,
                                                                           "ahd",          3,  _("AHD"),          NULL,
                                                                           "dcb",          4,  _("DCB"),          NULL,
                                                                           "dht",          11, _("DHT"),          NULL,
                                                                           "modified-ahd", 12, _("Modified AHD"), NULL,
                                                                           NULL),
                                              "ahd", G_PARAM_READWRITE);

          /* Settings only for DCB demosaicing. */
          clay_procedure_add_boolean_argument (procedure, "dcb-enhance",
                                              _("DCB color enhance"),
                                              _("Turns on DCB color enhance mode"),
                                               TRUE, G_PARAM_READWRITE);
          clay_procedure_add_int_argument (procedure, "dcb-iterations",
                                           _("DCB iterations"),
                                           _("Number of DCB correction passes"),
                                           /* XXX unsure what would be an acceptable max iteration number! */
                                           0, 10, 0, G_PARAM_READWRITE);

          clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                               format->file_type);
          clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                              format->extensions);
          clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                          format->magic);

          clay_load_procedure_set_handles_raw (CLAY_LOAD_PROCEDURE (procedure),
                                               TRUE);
          clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static ClayValueArray *
file_libraw_load (ClayProcedure          *procedure,
                  ClayRunMode             run_mode,
                  GFile                  *file,
                  ClayMetadata           *metadata,
                  ClayMetadataLoadFlags  *flags,
                  ClayProcedureConfig    *config,
                  gpointer                run_data)
{
  ClayValueArray    *return_vals;
  ClayImage         *image;
  libraw_data_t     *raw_info;
  GError            *error  = NULL;
  ClayPDBStatusType  status = CLAY_PDB_SUCCESS;

  raw_info = libraw_init (LIBRAW_OPTIONS_NO_DATAERR_CALLBACK);
  if (raw_info == NULL)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("Error initializing LibRaw."));
      status = CLAY_PDB_EXECUTION_ERROR;
    }
  else if (libraw_open_file (raw_info, g_file_peek_path (file)) != LIBRAW_SUCCESS)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      status = CLAY_PDB_CALLING_ERROR;
    }

  if (status == CLAY_PDB_SUCCESS && run_mode == CLAY_RUN_INTERACTIVE)
    {
      clay_ui_init (PLUG_IN_BINARY);
      if (! load_dialog (procedure, G_OBJECT (config), raw_info))
        status = CLAY_PDB_CANCEL;
    }

  if (status == CLAY_PDB_SUCCESS &&
      (image = load_image (file, raw_info,
                           run_mode, config, &error)) == NULL)
    status = CLAY_PDB_EXECUTION_ERROR;

  return_vals = clay_procedure_new_return_values (procedure, status, error);
  if (status == CLAY_PDB_SUCCESS)
    CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  if (raw_info != NULL)
    libraw_close (raw_info);

  return return_vals;
}

static ClayValueArray *
file_libraw_load_thumb (ClayProcedure        *procedure,
                        GFile                *file,
                        gint                  size,
                        ClayProcedureConfig  *config,
                        gpointer              run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error = NULL;

  image = load_thumbnail_image (file, size, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);
  CLAY_VALUES_SET_INT   (return_vals, 2, 0);
  CLAY_VALUES_SET_INT   (return_vals, 3, 0);
  CLAY_VALUES_SET_ENUM  (return_vals, 4, CLAY_RGB_IMAGE);
  CLAY_VALUES_SET_INT   (return_vals, 5, 1);

  clay_value_array_truncate (return_vals, 6);

  return return_vals;
}

static ClayImage *
load_image (GFile                *file,
            libraw_data_t        *raw_info,
            ClayRunMode           run_mode,
            ClayProcedureConfig  *config,
            GError              **error)
{
  ClayImage                *image                 = NULL;
  libraw_processed_image_t *image_data;
  gint                      raw_error             = 0;
  gint                      demosaicing           = 3;
  gint                      denoising             = 0;
  gboolean                  dcb_enhance           = 0;
  gboolean                  dcb_iterations        = 0;
  gboolean                  enable_interpolation = TRUE;

  g_object_get (config,
                "enable-interpolation", &enable_interpolation,
                "dcb-enhance",          &dcb_enhance,
                "dcb-iterations",       &dcb_iterations,
                NULL);

  raw_error = libraw_unpack (raw_info);
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      return NULL;
    }

  /* Just disable the full interpolation step altogether. */
  raw_info->params.no_interpolation = enable_interpolation ? 0 : 1;

  denoising = clay_procedure_config_get_choice_id (config, "noise-reduction");
  libraw_set_fbdd_noiserd (raw_info, denoising);

  demosaicing = clay_procedure_config_get_choice_id (config, "demosaicing");
  libraw_set_demosaic (raw_info, demosaicing);

  raw_info->params.dcb_iterations = dcb_iterations;
  raw_info->params.dcb_enhance_fl = dcb_enhance ? 1 : 0;

  /* Ensure image is imported as 16 bpc */
  raw_info->params.output_bps    = 16;
  /* Try to use camera white balance */
  raw_info->params.use_camera_wb = 1;

  raw_error = libraw_dcraw_process (raw_info);
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      return NULL;
    }

  image_data = libraw_dcraw_make_mem_image (raw_info, &raw_error);
  if (raw_error == LIBRAW_SUCCESS)
    {
      if (image_data->type == LIBRAW_IMAGE_BITMAP)
        {
          ClayImageBaseType  image_type = CLAY_RGB;
          ClayImageType      layer_type = CLAY_RGB_IMAGE;
          ClayPrecision      precision;
          ClayLayer         *layer;
          ClayColorProfile  *profile = NULL;
          GeglBuffer        *buffer;

          switch (image_data->colors)
            {
            case 1:
            case 2:
              image_type = CLAY_GRAY;
              layer_type = (image_data->colors == 1) ? CLAY_GRAY_IMAGE :
                                                       CLAY_GRAYA_IMAGE;
              break;

            case 3:
            case 4:
              image_type = CLAY_RGB;
              layer_type = (image_data->colors == 3) ? CLAY_RGB_IMAGE :
                                                       CLAY_RGBA_IMAGE;
              break;

            default:
              if (image_data != NULL)
                libraw_dcraw_clear_mem (image_data);

              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              return NULL;
            }

          if (image_data->bits == 8)
            {
              precision = CLAY_PRECISION_U8_NON_LINEAR;
            }
          else if (image_data->bits == 16)
            {
              precision = CLAY_PRECISION_U16_NON_LINEAR;
            }
          else
            {
              if (image_data != NULL)
                libraw_dcraw_clear_mem (image_data);

              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              return NULL;
            }

          if (raw_info->color.profile != NULL)
            {
              profile =
                clay_color_profile_new_from_icc_profile (raw_info->color.profile,
                                                         raw_info->color.profile_length,
                                                         NULL);
            }

          image = clay_image_new_with_precision (image_data->width,
                                                 image_data->height, image_type,
                                                 precision);

          if (profile)
            {
              clay_image_set_color_profile (image, profile);
              g_object_unref (profile);
            }

          layer = clay_layer_new (image, _("Background"), image_data->width,
                                  image_data->height, layer_type, 100,
                                  clay_image_get_default_new_layer_mode (image));
          clay_image_insert_layer (image, layer, NULL, 0);

          buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, image_data->width,
                                                   image_data->height),
                           0, NULL, image_data->data, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (buffer);
        }
    }

  if (image_data != NULL)
    libraw_dcraw_clear_mem (image_data);

  return image;
}

static ClayImage *
load_thumbnail_image (GFile   *file,
                      gint     thumb_size,
                      GError **error)
{
  libraw_data_t            *raw_info;
  libraw_processed_image_t *thumbnail;
  guint                     flags     = LIBRAW_OPTIONS_NO_DATAERR_CALLBACK;
  gint                      raw_error = 0;
  ClayImage                *image     = NULL;

  raw_info = libraw_init (flags);

  if (raw_info == NULL)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      libraw_close (raw_info);
      return NULL;
    }

  raw_error = libraw_open_file (raw_info, g_file_peek_path (file));
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      libraw_close (raw_info);
      return NULL;
    }

  raw_error = libraw_unpack_thumb (raw_info);
  if (raw_error != LIBRAW_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Error reading data. Image may be corrupt."));
      libraw_close (raw_info);
      return NULL;
    }

  /* Try to use camera white balance */
  raw_info->params.use_camera_wb = 1;

  thumbnail = libraw_dcraw_make_mem_thumb (raw_info, &raw_error);
  if (raw_error == LIBRAW_SUCCESS)
    {
      if (thumbnail->type == LIBRAW_IMAGE_JPEG)
        {
          GFile          *temp_file      = NULL;
          FILE           *fp;
          ClayValueArray *return_vals    = NULL;
          ClayProcedure  *procedure;

          temp_file = clay_temp_file ("jpeg");
          fp        = g_fopen (g_file_peek_path (temp_file), "wb");

          if (! fp)
            {
              g_set_error (error, G_FILE_ERROR, 0,
                           _("Error reading data. Image may be corrupt."));
              g_file_delete (temp_file, NULL, NULL);
              g_object_unref (temp_file);
              return NULL;
            }

          fwrite (thumbnail->data, sizeof (guchar), thumbnail->data_size, fp);
          fclose (fp);

          procedure   = clay_pdb_lookup_procedure (clay_get_pdb (),
                                                   "file-jpeg-load");
          return_vals = clay_procedure_run (procedure,
                                            "run-mode", CLAY_RUN_NONINTERACTIVE,
                                            "file",     temp_file,
                                            NULL);

         if (return_vals)
           image =
             g_value_get_object (clay_value_array_index (return_vals, 1));

          g_file_delete (temp_file, NULL, NULL);
          g_object_unref (temp_file);
          g_clear_pointer (&return_vals, clay_value_array_unref);
        }
      else if (thumbnail->type == LIBRAW_IMAGE_BITMAP)
        {
          ClayLayer  *layer;
          GeglBuffer *buffer;

          image = clay_image_new_with_precision (thumbnail->width,
                                                 thumbnail->height, CLAY_RGB,
                                                 CLAY_PRECISION_U8_NON_LINEAR);

          layer = clay_layer_new (image, _("Background"), thumbnail->width,
                                  thumbnail->height, CLAY_RGB_IMAGE, 100,
                                  clay_image_get_default_new_layer_mode (image));
          clay_image_insert_layer (image, layer, NULL, 0);

          buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, thumbnail->width,
                                                   thumbnail->height), 0,
                           NULL, thumbnail->data, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (buffer);
        }
    }

  if (thumbnail != NULL)
    libraw_dcraw_clear_mem (thumbnail);

  libraw_close (raw_info);

  return image;
}

static gboolean
load_dialog (ClayProcedure *procedure,
             GObject       *config,
             libraw_data_t *raw_info)
{
  GtkWidget      *dialog;
  gchar          *title;
  ClayValueArray *values;
  GValue          value = G_VALUE_INIT;
  gboolean        run;

  /* TRANSLATORS: the "%s" will be the name of a RAW format, e.g. "Raw Canon". */
  title = g_strdup_printf (_("Develop: %s"),
                           clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure)));
  dialog = clay_procedure_dialog_new (procedure, CLAY_PROCEDURE_CONFIG (config), title);
  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog), "noise-reduction",
                                    GTK_TYPE_COMBO_BOX);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                  "interpolation-step",
                                  "noise-reduction", "demosaicing",
                                  "dcb-enhance", "dcb-iterations", NULL);
  clay_procedure_dialog_fill_expander (CLAY_PROCEDURE_DIALOG (dialog), "interpolation-expander",
                                       "enable-interpolation", FALSE, "interpolation-step");
  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "interpolation-expander", NULL);

  /* The whole logic under comes from reading LibRaw::dcraw_process()
   * code. This is unfortunately not cleanly documented and there are no
   * programmatic ways to actually verify if some settings should be
   * disabled or not. This means that this code could become wrong if
   * upstream implementation changes so we have to keep it in sync. :-(
   */
  if (raw_info->idata.filters == 0)
    {
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "interpolation-expander", FALSE, NULL, NULL, FALSE);
    }
  else if (raw_info->idata.filters <= 1000 || raw_info->idata.colors != 3)
    {
      /* Note: there is actually another case where noise reduction should
       * be disabled, based on a computation of "real_colors" but I wanted
       * to avoid duplicating a big piece of code including some hardcoded
       * 16*16 integer matrix. So let's accept that in some edge case,
       * this setting will be available while not doing anything.
       */
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "noise-reduction", FALSE, NULL, NULL, FALSE);
      if (raw_info->idata.filters <= 1000)
        {
          GParamSpec *pspec;
          ClayChoice *choice;

          pspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "demosaicing");
          choice = clay_param_spec_choice_get_choice (pspec);
          clay_choice_set_sensitive (choice, "ppg", FALSE);
        }
    }

  values = clay_value_array_new (1);
  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, "dcb");
  clay_value_array_append (values, &value);
  g_value_unset (&value);
  clay_procedure_dialog_set_sensitive_if_in (CLAY_PROCEDURE_DIALOG (dialog),
                                             "dcb-enhance", NULL, "demosaicing", values, TRUE);
  clay_procedure_dialog_set_sensitive_if_in (CLAY_PROCEDURE_DIALOG (dialog),
                                             "dcb-iterations", NULL, "demosaicing",
                                             clay_value_array_ref (values), TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);
  g_free (title);

  return run;
}
