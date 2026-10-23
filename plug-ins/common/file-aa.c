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

/**
 * aa.c version 1.0
 * A plugin that uses libaa (ftp://ftp.ta.jcu.cz/pub/aa) to export images as
 * ASCII.
 * NOTE: This plugin *requires* aalib 1.2 or later. Earlier versions will
 * not work.
 * Code copied from all over the Clay source.
 * Tim Newsome <nuisance@cmu.edu>
 */

#include "config.h"

#include <string.h>

#include <aalib.h>

#include "libclaycolor/claycolor-private.h"

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define EXPORT_PROC    "file-aa-export"
#define PLUG_IN_BINARY "file-aa"


typedef struct _Ascii      Ascii;
typedef struct _AsciiClass AsciiClass;

struct _Ascii
{
  ClayPlugIn      parent_instance;
};

struct _AsciiClass
{
  ClayPlugInClass parent_class;
};


#define ASCII_TYPE  (ascii_get_type ())
#define ASCII(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASCII_TYPE, Ascii))

GType                   ascii_get_type         (void);

static GList          * ascii_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * ascii_create_procedure (ClayPlugIn           *plug_in,
                                                const gchar          *name);

static ClayValueArray * ascii_export           (ClayProcedure        *procedure,
                                                ClayRunMode           run_mode,
                                                ClayImage            *image,
                                                GFile                *file,
                                                ClayExportOptions    *options,
                                                ClayMetadata         *metadata,
                                                ClayProcedureConfig  *config,
                                                gpointer              run_data);

static gboolean         export_aa              (GFile                *file,
                                                ClayDrawable         *drawable,
                                                GObject              *config,
                                                GError              **error);
static void             clay2aa                (ClayDrawable         *drawable,
                                                aa_context           *context);

static gboolean         save_dialog            (ClayProcedure        *procedure,
                                                GObject              *config,
                                                ClayImage            *image);


G_DEFINE_TYPE (Ascii, ascii, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (ASCII_TYPE)
DEFINE_STD_SET_I18N


static void
ascii_class_init (AsciiClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ascii_query_procedures;
  plug_in_class->create_procedure = ascii_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
ascii_init (Ascii *ascii)
{
}

static GList *
ascii_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static ClayProcedure *
ascii_create_procedure (ClayPlugIn  *plug_in,
                        const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      gint i;

      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, ascii_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("ASCII art"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("ASCII art"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports grayscale image in various "
                                          "text formats"),
                                        _("This plug-in uses aalib to export "
                                          "grayscale image as ascii art into "
                                          "a variety of text formats"),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Tim Newsome <nuisance@cmu.edu>",
                                      "Tim Newsome <nuisance@cmu.edu>",
                                      "1997");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "text/plain");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "txt,ansi,text");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      for (i = 0; aa_formats[i]; i++);

      clay_procedure_add_int_argument (procedure, "file-type",
                                       _("_Format"),
                                       _("File type to use"),
                                       0, i, 0,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
ascii_export (ClayProcedure        *procedure,
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

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      clay_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = CLAY_PDB_CANCEL;
    }

  export = clay_export_options_get_image (options, &image);
  drawables = clay_image_list_layers (image);

  if (status == CLAY_PDB_SUCCESS)
    {
      if (! export_aa (file, drawables->data, G_OBJECT (config), &error))
        {
          status = CLAY_PDB_EXECUTION_ERROR;
        }
    }

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_list_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}

static gboolean
export_aa (GFile         *file,
           ClayDrawable  *drawable,
           GObject       *config,
           GError       **error)
{
  aa_savedata  savedata;
  aa_context  *context;
  aa_format    format = { 0 };
  gint         output_type;

  g_object_get (config,
                "file-type", &output_type,
                NULL);

  memcpy (&format, aa_formats[output_type], sizeof (aa_format));

  format.width  = clay_drawable_get_width  (drawable) / 2;
  format.height = clay_drawable_get_height (drawable) / 2;

  /* Get a libaa context which will save its output to filename. */
  savedata.name   = g_file_get_path (file);
  savedata.format = &format;

  context = aa_init (&save_d, &aa_defparams, &savedata);
  if (! context)
    return FALSE;

  clay2aa (drawable, context);

  aa_flush (context);
  aa_close (context);

  return TRUE;
}

static void
clay2aa (ClayDrawable *drawable,
         aa_context   *context)
{
  GeglBuffer      *buffer;
  const Babl      *format;
  aa_renderparams *renderparams;
  gint             width;
  gint             height;
  gint             x, y;
  gint             bpp;
  guchar          *buf;
  guchar          *p;

  buffer = clay_drawable_get_buffer (drawable);

  width  = aa_imgwidth  (context);
  height = aa_imgheight (context);

  switch (clay_drawable_type (drawable))
    {
    case CLAY_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case CLAY_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case CLAY_RGB_IMAGE:
    case CLAY_INDEXED_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case CLAY_RGBA_IMAGE:
    case CLAY_INDEXEDA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    default:
      g_return_if_reached ();
      break;
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  buf = g_new (guchar, width * bpp);

  for (y = 0; y < height; y++)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      switch (bpp)
        {
        case 1:  /* GRAY */
          for (x = 0, p = buf; x < width; x++, p++)
            aa_putpixel (context, x, y, *p);
          break;

        case 2:  /* GRAYA, blend over black */
          for (x = 0, p = buf; x < width; x++, p += 2)
            aa_putpixel (context, x, y, (p[0] * (p[1] + 1)) >> 8);
          break;

        case 3:  /* RGB */
          for (x = 0, p = buf; x < width; x++, p += 3)
            aa_putpixel (context, x, y,
                         CLAY_RGB_LUMINANCE (p[0], p[1], p[2]) + 0.5);
          break;

        case 4:  /* RGBA, blend over black */
          for (x = 0, p = buf; x < width; x++, p += 4)
            aa_putpixel (context, x, y,
                         ((guchar) (CLAY_RGB_LUMINANCE (p[0], p[1], p[2]) + 0.5)
                          * (p[3] + 1)) >> 8);
          break;

        default:
          g_assert_not_reached ();
          break;
        }
    }

  g_free (buf);

  g_object_unref (buffer);

  renderparams = aa_getrenderparams ();
  renderparams->dither = AA_FLOYD_S;

  aa_render (context, renderparams, 0, 0,
             aa_scrwidth (context), aa_scrheight (context));
}

static gboolean
save_dialog (ClayProcedure *procedure,
             GObject       *config,
             ClayImage     *image)
{
  GtkWidget    *dialog;
  GtkListStore *store;
  GtkWidget    *combo;
  gint          i;
  gboolean      run;

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  store = g_object_new (CLAY_TYPE_INT_STORE, NULL);

  for (i = 0; aa_formats[i]; i++)
    gtk_list_store_insert_with_values (store, NULL, -1,
                                       CLAY_INT_STORE_VALUE, i,
                                       CLAY_INT_STORE_LABEL, aa_formats[i]->formatname,
                                       -1);

  combo = clay_procedure_dialog_get_int_combo (CLAY_PROCEDURE_DIALOG (dialog),
                                               "file-type",
                                               CLAY_INT_STORE (store));
  g_object_set (combo, "margin", 12, NULL);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog), NULL);
  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
