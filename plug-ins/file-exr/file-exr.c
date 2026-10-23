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

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"

#include "openexr-wrapper.h"

#define LOAD_PROC       "file-exr-load"
#define PLUG_IN_BINARY  "file-exr"
#define PLUG_IN_VERSION "0.0.0"


typedef struct _Exr      Exr;
typedef struct _ExrClass ExrClass;

struct _Exr
{
  ClayPlugIn      parent_instance;
};

struct _ExrClass
{
  ClayPlugInClass parent_class;
};


#define EXR_TYPE  (exr_get_type ())
#define EXR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXR_TYPE, Exr))

GType                   exr_get_type         (void);

static GList          * exr_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * exr_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * exr_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);

static ClayImage      * load_image           (GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              gboolean               interactive,
                                              GError               **error);
static void             sanitize_comment     (gchar                 *comment);
void                    load_dialog          (EXRImageType           image_type);


G_DEFINE_TYPE (Exr, exr, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (EXR_TYPE)
DEFINE_STD_SET_I18N


static void
exr_class_init (ExrClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = exr_query_procedures;
  plug_in_class->create_procedure = exr_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
exr_init (Exr *exr)
{
}

static GList *
exr_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (LOAD_PROC));
}

static ClayProcedure *
exr_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           exr_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("OpenEXR image"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads files in the OpenEXR file format"),
                                        "This plug-in loads OpenEXR files. ",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Dominik Ernst <dernst@gmx.de>, "
                                      "Mukund Sivaraman <muks@banu.com>",
                                      "Dominik Ernst <dernst@gmx.de>, "
                                      "Mukund Sivaraman <muks@banu.com>",
                                      NULL);

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-exr");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "exr");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,long,0x762f3101");
    }

  return procedure;
}

static ClayValueArray *
exr_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error  = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, metadata, flags, run_mode == CLAY_RUN_INTERACTIVE,
                      &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals =  clay_procedure_new_return_values (procedure,
                                                   CLAY_PDB_SUCCESS,
                                                   NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayImage *
load_image (GFile                 *file,
            ClayMetadata          *metadata,
            ClayMetadataLoadFlags *flags,
            gboolean               interactive,
            GError               **error)
{
  EXRLoader        *loader;
  gint              width;
  gint              height;
  gboolean          has_alpha;
  ClayImageBaseType image_type;
  ClayPrecision     image_precision;
  ClayImage        *image = NULL;
  ClayImageType     layer_type;
  gint              layer_count = 0;
  gboolean          layers_only;
  const Babl       *format;
  gint              bpp;
  gint              tile_height;
  gchar            *pixels = NULL;
  gint              begin;
  gint32            success = FALSE;
  gchar            *comment = NULL;
  ClayColorProfile *profile = NULL;
  guchar           *exif_data;
  guint             exif_size;
  guchar           *xmp_data;
  guint             xmp_size;

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  loader = exr_loader_new (g_file_peek_path (file));

  if (! loader)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error opening file '%s'"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  width  = exr_loader_get_width (loader);
  height = exr_loader_get_height (loader);

  if (width < 1 || height < 1 ||
      width > CLAY_MAX_IMAGE_SIZE || height > CLAY_MAX_IMAGE_SIZE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image dimensions from '%s'"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  has_alpha = exr_loader_has_alpha (loader) ? TRUE : FALSE;

  switch (exr_loader_get_precision (loader))
    {
    case PREC_UINT:
      image_precision = CLAY_PRECISION_U32_LINEAR;
      break;
    case PREC_HALF:
      image_precision = CLAY_PRECISION_HALF_LINEAR;
      break;
    case PREC_FLOAT:
      image_precision = CLAY_PRECISION_FLOAT_LINEAR;
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image precision from '%s'"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  switch (exr_loader_get_image_type (loader))
    {
    case IMAGE_TYPE_RGB:
    case IMAGE_TYPE_YUV:
      image_type = CLAY_RGB;
      layer_type = has_alpha ? CLAY_RGBA_IMAGE : CLAY_RGB_IMAGE;
      break;
    case IMAGE_TYPE_GRAY:
    case IMAGE_TYPE_UNKNOWN_1_CHANNEL:
      image_type = CLAY_GRAY;
      layer_type = has_alpha ? CLAY_GRAYA_IMAGE : CLAY_GRAY_IMAGE;
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image type from '%s'"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  image = clay_image_new_with_precision (width, height,
                                         image_type, image_precision);
  if (! image)
    {
      g_set_error (error, 0, 0,
                   _("Could not create new image for '%s': %s"),
                   clay_file_get_utf8_name (file),
                   clay_pdb_get_last_error (clay_get_pdb ()));
      goto out;
    }

  if (interactive                                                         &&
      (exr_loader_get_image_type (loader) == IMAGE_TYPE_UNKNOWN_1_CHANNEL))
    load_dialog (exr_loader_get_image_type (loader));

  /* try to load an icc profile, it will be generated on the fly if
   * chromaticities are given
   */
  if (image_type == CLAY_RGB)
    {
      profile = exr_loader_get_profile (loader);

      if (profile)
        clay_image_set_color_profile (image, profile);
    }

  exr_loader_get_layer_info (loader, &layer_count, &layers_only);

  /* i == -1 represents an image with no named layers, just raw channels.
   * If layers_only is TRUE, there are only named layers and we skip these
   * entirely and just read the layers */
  for (gint i = (-1 + layers_only); i < layer_count; i++)
    {
      ClayLayer  *layer;
      GeglBuffer *buffer     = NULL;
      gchar      *layer_name = NULL;

      if (i > -1)
        layer_name = exr_loader_get_layer_name (loader, i);
      else
        layer_name = _("Background");

      layer = clay_layer_new (image, layer_name, width, height,
                              layer_type, 100,
                              clay_image_get_default_new_layer_mode (image));
      clay_image_insert_layer (image, layer, NULL, -1);

      if (i > -1)
        g_free (layer_name);

      buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));
      format = clay_drawable_get_format (CLAY_DRAWABLE (layer));
      bpp = babl_format_get_bytes_per_pixel (format);

      tile_height = clay_tile_height ();
      pixels = g_new0 (gchar, tile_height * width * bpp);

      for (begin = 0; begin < height; begin += tile_height)
        {
          gint end;
          gint num;

          end = MIN (begin + tile_height, height);
          num = end - begin;

          for (gint j = 0; j < num; j++)
            {
              gint retval;

              retval = exr_loader_read_pixel_row (loader,
                                                  pixels + (j * width * bpp),
                                                  bpp, begin + j, i);
              if (retval < 0)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Error reading pixel data from '%s'"),
                               clay_file_get_utf8_name (file));
                  goto out;
                }
            }

          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, begin, width, num),
                           0, NULL, pixels, GEGL_AUTO_ROWSTRIDE);

          clay_progress_update ((gdouble) begin / (gdouble) height);
        }

      g_clear_object (&buffer);
      g_clear_pointer (&pixels, g_free);
    }

  /* try to read the file comment */
  comment = exr_loader_get_comment (loader);
  if (comment)
    {
      ClayParasite *parasite;

      sanitize_comment (comment);
      parasite = clay_parasite_new ("clay-comment",
                                    CLAY_PARASITE_PERSISTENT,
                                    strlen (comment) + 1,
                                    comment);
      clay_image_attach_parasite (image, parasite);
      clay_parasite_free (parasite);
    }

  /* check if the image contains Exif or Xmp data and read it */
  exif_data = exr_loader_get_exif (loader, &exif_size);
  xmp_data  = exr_loader_get_xmp  (loader, &xmp_size);

  if (metadata && (exif_data || xmp_data))
    {
      if (exif_data)
        {
          GError *error = NULL;

          if (! clay_metadata_set_from_exif (metadata, exif_data, exif_size, &error))
            {
              g_message (_("Failed to load metadata: %s"),
                         error ? error->message : _("Unknown reason"));
              g_clear_error (&error);
            }

          g_free (exif_data);
        }

      if (xmp_data)
        {
          GError *error = NULL;

          if (! clay_metadata_set_from_xmp (metadata, xmp_data, xmp_size, &error))
            {
              g_message (_("Failed to load metadata: %s"),
                         error ? error->message : _("Unknown reason"));
              g_clear_error (&error);
            }
          g_free (xmp_data);
        }

      if (comment)
        *flags &= ~CLAY_METADATA_LOAD_COMMENT;

      if (profile)
        *flags &= ~CLAY_METADATA_LOAD_COLORSPACE;

    }

  clay_progress_update (1.0);

  success = TRUE;

 out:
  g_clear_object (&profile);
  g_clear_pointer (&comment, g_free);
  g_clear_pointer (&loader, exr_loader_unref);

  if (success)
    return image;

  if (image)
    clay_image_delete (image);

  return NULL;
}

/* copy & pasted from file-jpeg/jpeg-load.c */
static void
sanitize_comment (gchar *comment)
{
  const gchar *start_invalid;

  if (! g_utf8_validate (comment, -1, &start_invalid))
    {
      guchar *c;

      for (c = (guchar *) start_invalid; *c; c++)
        {
          if (*c > 126 || (*c < 32 && *c != '\t' && *c != '\n' && *c != '\r'))
            *c = '?';
        }
    }
}

void
load_dialog (EXRImageType image_type)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *vbox;
  gchar     *label_text = NULL;

  clay_ui_init (PLUG_IN_BINARY);

  dialog = clay_dialog_new (_("Import OpenEXR"),
                            "openexr-notice",
                            NULL, 0, NULL, NULL,
                            _("_OK"), GTK_RESPONSE_OK,
                            NULL);

  clay_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  if (image_type == IMAGE_TYPE_UNKNOWN_1_CHANNEL)
    label_text = g_strdup_printf ("<b>%s</b>\n%s", _("Unknown Channel Name"),
                                  _("The image contains a single unknown channel.\n"
                                    "It has been converted to grayscale."));

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), label_text);

  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_yalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_set_visible (label, TRUE);

  if (label_text)
    g_free (label_text);

  gtk_widget_set_visible (dialog, TRUE);

  /* run the dialog */
  clay_dialog_run (CLAY_DIALOG (dialog));

  gtk_widget_destroy (dialog);
}
