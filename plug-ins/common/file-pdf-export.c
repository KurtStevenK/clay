/* xyz.kainzmayer.ClayDesign
 *
 * file-pdf-export.c - PDF file exporter, based on the cairo PDF surface
 *
 * Copyright (C) 2010 Barak Itkin <lightningismyname@gmail.com>
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

/* The PDF export plugin has 3 main procedures:
 * 1. file-pdf-export
 *    This is the main procedure. It has 3 options for optimizations of
 *    the pdf file, and it can show a gui. This procedure works on a single
 *    image.
 * 2. file-pdf-export-defaults
 *    This procedures is the one that will be invoked by clay's file-save,
 *    when the pdf extension is chosen. If it's in RUN_INTERACTIVE, it will
 *    pop a user interface with more options, like file-pdf-export. If it's in
 *    RUN_NONINTERACTIVE, it will simply use the default values. Note that on
 *    RUN_WITH_LAST_VALS there will be no gui, however the values will be the
 *    ones that were used in the last interactive run (or the defaults if none
 *    are available.
 * 3. file-pdf-export-multi
 *    This procedures is more advanced, and it allows the creation of multiple
 *    paged pdf files. It will be located in File/Create/Multiple page PDF...
 *
 * It was suggested that file-pdf-export-multi will be removed from the UI as it
 * does not match the product vision (Clay isn't a program for editing multiple
 * paged documents).
 */

/* Known Issues (except for the coding style issues):
 * 1. Grayscale layers are inverted (although layer masks which are not grayscale,
 * are not inverted)
 * 2. Exporting some fonts doesn't work since clay_text_layer_get_font Returns a
 * font which is sometimes incompatible with pango_font_description_from_string
 * (clay_text_layer_get_font sometimes returns suffixes such as "semi-expanded" to
 * the font's name although the Clay's font selection dialog shows the don'ts name
 * normally - This should be checked again in Clay 2.7)
 * 3. Indexed layers can't be optimized yet (Since clay_histogram won't work on
 * indexed layers)
 * 4. Rendering the pango layout requires multiplying the size in PANGO_SCALE. This
 * means I'll need to do some hacking on the markup returned from Clay.
 * 5. When accessing the contents of layer groups is supported, we should do use it
 * (since this plugin should preserve layers).
 *
 * Also, there are 2 things which we should warn the user about:
 * 1. Cairo does not support bitmap masks for text.
 * 2. Currently layer modes are ignored. We do support layers, including
 * transparency and opacity, but layer modes are not supported.
 */

/* Changelog
 *
 * April 29, 2009 | Barak Itkin <lightningismyname@gmail.com>
 *   First version of the plugin. This is only a proof of concept and not a full
 *   working plugin.
 *
 * May 6, 2009 Barak | Itkin <lightningismyname@gmail.com>
 *   Added new features and several bugfixes:
 *   - Added handling for image resolutions
 *   - fixed the behavior of getting font sizes
 *   - Added various optimizations (solid rectangles instead of bitmaps, ignoring
 *     invisible layers, etc.) as a macro flag.
 *   - Added handling for layer masks, use CAIRO_FORMAT_A8 for grayscale drawables.
 *   - Indexed layers are now supported
 *
 * August 17, 2009 | Barak Itkin <lightningismyname@gmail.com>
 *   Most of the plugin was rewritten from scratch and it now has several new
 *   features:
 *   - Got rid of the optimization macros in the code. The gui now supports
 *     selecting which optimizations to apply.
 *   - Added a procedure to allow the creation of multiple paged PDF's
 *   - Registered the plugin on "<Image>/File/Create/PDF"
 *
 * August 21, 2009 | Barak Itkin <lightningismyname@gmail.com>
 *   Fixed a typo that prevented the plugin from compiling...
 *   A migration to the new Clay 2.8 api, which includes:
 *   - Now using clay_export_dialog_new
 *   - Using clay_text_layer_get_hint_style (2.8) instead of the deprecated
 *     clay_text_layer_get_hinting (2.6).
 *
 * August 24, 2010 | Barak Itkin <lightningismyname@gmail.com>
 *   More migrations to the new Clay 2.8 api:
 *   - Now using the ClayItem api
 *   - Using clay_text_layer_get_markup where possible
 *   - Fixed some compiler warnings
 *   Also merged the header and c file into one file, Updated some of the comments
 *   and documentation, and moved this into the main source repository.
 */

#include "config.h"

#include <errno.h>

#include <glib/gstdio.h>
#include <cairo-pdf.h>
#include <pango/pangocairo.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define EXPORT_PROC             "file-pdf-export"
#define EXPORT_MULTI_PROC       "file-pdf-export-multi"
#define PLUG_IN_BINARY          "file-pdf-export"
#define PLUG_IN_ROLE            "clay-file-pdf-export"

#define DATA_IMAGE_LIST         "file-pdf-data-multi-page"

/* Clay will crash before you reach this limitation :D */
#define MAX_PAGE_COUNT           350
#define MAX_FILE_NAME_LENGTH     350

#define THUMB_WIDTH              90
#define THUMB_HEIGHT             120

#define CLAY_PLUGIN_PDF_EXPORT_ERROR clay_plugin_pdf_export_error_quark ()

typedef enum
{
  CLAY_PLUGIN_PDF_EXPORT_ERROR_FAILED
} ClayPluginPDFError;

GQuark clay_plugin_pdf_export_error_quark (void);

typedef struct
{
  ClayImage *images[MAX_PAGE_COUNT];
  guint32    image_count;
  gchar      file_name[MAX_FILE_NAME_LENGTH];
} PdfMultiPage;

enum
{
  THUMB,
  PAGE_NUMBER,
  IMAGE_NAME,
  IMAGE
};

typedef struct
{
  GdkPixbuf *thumb;
  gint32     page_number;
  gchar     *image_name;
} Page;


typedef struct _Pdf      Pdf;
typedef struct _PdfClass PdfClass;

struct _Pdf
{
  ClayPlugIn      parent_instance;
};

struct _PdfClass
{
  ClayPlugInClass parent_class;
};


#define PDF_TYPE  (pdf_get_type ())
#define PDF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PDF_TYPE, Pdf))

GType                           pdf_get_type             (void);

static GList                  * pdf_query_procedures     (ClayPlugIn           *plug_in);
static ClayProcedure          * pdf_create_procedure     (ClayPlugIn           *plug_in,
                                                          const gchar          *name);

static ClayValueArray         * pdf_export               (ClayProcedure        *procedure,
                                                          ClayRunMode           run_mode,
                                                          ClayImage            *image,
                                                          GFile                *file,
                                                          ClayExportOptions    *options,
                                                          ClayMetadata         *metadata,
                                                          ClayProcedureConfig  *config,
                                                          gpointer              run_data);
static ClayValueArray         * pdf_export_multi         (ClayProcedure        *procedure,
                                                          ClayProcedureConfig  *config,
                                                          gpointer              run_data);

static ClayPDBStatusType        pdf_export_image         (ClayProcedure        *procedure,
                                                          ClayProcedureConfig  *config,
                                                          ClayExportOptions    *options,
                                                          gboolean              single_image,
                                                          gboolean              show_progress,
                                                          GError              **error);

static ClayExportCapabilities   export_edit_options      (ClayProcedure        *procedure,
                                                          ClayProcedureConfig  *config,
                                                          ClayExportOptions    *options,
                                                          gpointer              create_data);

static void                     init_image_list_defaults (ClayImage            *image);

static void                     validate_image_list      (void);

static gboolean                 gui_single               (ClayProcedure        *procedure,
                                                          ClayProcedureConfig  *config,
                                                          ClayImage            *image);
static gboolean                 gui_multi                (ClayProcedure        *procedure,
                                                          ClayProcedureConfig  *config);

static void                     choose_file_call         (GtkWidget            *browse_button,
                                                          gpointer              file_entry);

static gboolean                 get_image_list           (void);

static GtkTreeModel           * create_model             (void);

static void                     add_image_call           (GtkWidget            *widget,
                                                          gpointer              img_combo);
static void                     del_image_call           (GtkWidget            *widget,
                                                          gpointer              icon_view);
static void                     remove_call              (GtkTreeModel         *tree_model,
                                                          GtkTreePath          *path,
                                                          gpointer              user_data);
static void                     recount_pages            (void);

static cairo_surface_t *get_cairo_surface        (ClayDrawable         *drawable,
                                                  gboolean              as_mask,
                                                  GError              **error);

static GeglColor *      get_layer_color          (ClayLayer            *layer,
                                                  gboolean             *single);

static void             drawText                 (ClayLayer            *layer,
                                                  gdouble               opacity,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res);
static void             drawPath                 (ClayLayer            *layer,
                                                  gdouble               opacity,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res);
static void             draw_path_pattern        (cairo_t              *cr,
                                                  ClayPattern          *pattern);

static gboolean         draw_layer               (ClayImage            *image,
                                                  ClayLayer           **layers,
                                                  gint                  n_layers,
                                                  ClayProcedureConfig  *config,
                                                  gboolean              single_image,
                                                  gint                  j,
                                                  cairo_t              *cr,
                                                  gdouble               x_res,
                                                  gdouble               y_res,
                                                  const gchar          *name,
                                                  gboolean              show_progress,
                                                  gdouble               progress_start,
                                                  gdouble               progress_end,
                                                  gint                  layer_level,
                                                  GError              **error);


G_DEFINE_TYPE (Pdf, pdf, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (PDF_TYPE)
DEFINE_STD_SET_I18N


static gboolean              dnd_remove = TRUE;
static PdfMultiPage          multi_page;

static GtkTreeModel         *model;
static GtkFileChooserNative *file_choose;
static gchar                *file_name;


G_DEFINE_QUARK (clay-plugin-pdf-export-error-quark, clay_plugin_pdf_export_error)

static void
pdf_class_init (PdfClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pdf_query_procedures;
  plug_in_class->create_procedure = pdf_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pdf_init (Pdf *pdf)
{
}

static GList *
pdf_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (EXPORT_PROC));
  list = g_list_append (list, g_strdup (EXPORT_MULTI_PROC));

  return list;
}

static ClayProcedure *
pdf_create_procedure (ClayPlugIn  *plug_in,
                        const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             TRUE, pdf_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Portable Document Format"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in PDF format"),
                                        _("Exports files in Adobe's Portable "
                                          "Document Format. PDF is designed to "
                                          "be easily processed by a variety of "
                                          "different platforms, and is a "
                                          "distant cousin of PostScript."),
                                          name);
      clay_procedure_set_attribution (procedure,
                                      "Barak Itkin, Lionel N., Jehan",
                                      "Copyright Barak Itkin, Lionel N., Jehan",
                                      "August 2009, 2017");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("PDF"));
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "application/pdf");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "pdf");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              0, export_edit_options, NULL, NULL);

      clay_procedure_add_boolean_argument (procedure, "vectorize",
                                           _("Convert _bitmaps to vector graphics where possible"),
                                           _("Convert bitmaps to vector graphics where possible"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "ignore-hidden",
                                           _("O_mit hidden layers and layers with zero opacity"),
                                           _("Non-visible layers will not be exported"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "apply-masks",
                                           _("_Apply layer masks"),
                                           _("Apply layer masks before saving (Keeping the mask "
                                             "will not change the output, only the PDF structure)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "layers-as-pages",
                                           _("La_yers as pages"),
                                           _("Layers as pages (bottom layers first)."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "reverse-order",
                                           _("Re_verse order"),
                                           _("Reverse the pages order (top layers first)."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "root-layers-only",
                                           _("Roo_t layers only"),
                                           _("Only the root layers are considered pages"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "convert-text-layers",
                                           _("Convert te_xt layers to image"),
                                           _("Convert text layers to raster graphics"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "fill-background-color",
                                           _("_Fill transparent areas with background color"),
                                           _("Fill transparent areas with background color if "
                                             "layer has an alpha channel"),
                                           TRUE,
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_MULTI_PROC))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PLUGIN,
                                      pdf_export_multi, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("_Create multipage PDF..."));
#if 0
      clay_procedure_add_menu_path (procedure, "<Image>/File/Create/PDF");
#endif

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in PDF format"),
                                        _("Exports files in Adobe's Portable "
                                          "Document Format. PDF is designed to "
                                          "be easily processed by a variety of "
                                          "different platforms, and is a "
                                          "distant cousin of PostScript."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Barak Itkin",
                                      "Copyright Barak Itkin",
                                      "August 2009");

      clay_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        CLAY_TYPE_RUN_MODE,
                                        CLAY_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);

      clay_procedure_add_core_object_array_argument (procedure, "images",
                                                     "Images",
                                                     "Input image for each page (An image can "
                                                     "appear more than once)",
                                                     CLAY_TYPE_IMAGE,
                                                     G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "vectorize",
                                           _("Convert _bitmaps to vector graphics where possible"),
                                           _("Convert bitmaps to vector graphics where possible"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "ignore-hidden",
                                           _("O_mit hidden layers and layers with zero opacity"),
                                           _("Non-visible layers will not be exported"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "apply-masks",
                                           _("_Apply layer masks"),
                                           _("Apply layer masks before saving (Keeping the mask "
                                             "will not change the output, only the PDF structure)"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "fill-background-color",
                                           _("_Fill transparent areas with background color"),
                                           _("Fill transparent areas with background color if "
                                             "layer has an alpha channel"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_string_argument (procedure, "uri",
                                          _("Export to"),
                                          _("The URI of the file to export to"),
                                          NULL,
                                          CLAY_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
pdf_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  GError            *error  = NULL;
  ClayPDBStatusType  status = CLAY_PDB_SUCCESS;

  gegl_init (NULL, NULL);

  /* Initializing all the settings */
  multi_page.image_count = 0;

  file_name = g_file_get_path (file);

  init_image_list_defaults (image);

  validate_image_list ();

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      if (! gui_single (procedure, config, image))
        status = CLAY_PDB_CANCEL;
    }

  if (status == CLAY_PDB_SUCCESS)
    status = pdf_export_image (procedure, config, options, TRUE,
                               (run_mode != CLAY_RUN_NONINTERACTIVE),
                               &error);

  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayValueArray *
pdf_export_multi (ClayProcedure        *procedure,
                  ClayProcedureConfig  *config,
                  gpointer              run_data)
{
  GError            *error     = NULL;
  ClayPDBStatusType  status    = CLAY_PDB_SUCCESS;
  ClayRunMode        run_mode;
  ClayExportOptions *options   = NULL;
  gchar             *uri;
  ClayImage        **images    = NULL;
  ClayImage         *image     = NULL;
  GFile             *file      = NULL;

  gegl_init (NULL, NULL);

  g_object_get (config,
                "run-mode", &run_mode,
                "uri",      &uri,
                "images",   &images,
                NULL);

  options = g_object_new (CLAY_TYPE_EXPORT_OPTIONS,
                          "capabilities",
                          CLAY_EXPORT_CAN_HANDLE_RGB    |
                          CLAY_EXPORT_CAN_HANDLE_ALPHA  |
                          CLAY_EXPORT_CAN_HANDLE_GRAY   |
                          CLAY_EXPORT_CAN_HANDLE_LAYERS |
                          CLAY_EXPORT_CAN_HANDLE_INDEXED,
                          NULL);

  if (uri != NULL)
    {
      file = g_file_new_for_uri (uri);
      g_free (uri);
    }

  if (file != NULL)
    file_name = g_file_get_path (file);

  if (images)
    {
      multi_page.image_count = clay_core_object_array_get_length ((GObject **) images);
      for (gint i = 0; i < multi_page.image_count; i++)
        multi_page.images[i] = images[i];
    }
  else
    {
      init_image_list_defaults (image);
    }

  validate_image_list ();

  /* Starting the executions */
  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      if (! gui_multi (procedure, config))
        status = CLAY_PDB_CANCEL;
    }

  if (status == CLAY_PDB_SUCCESS)
    status = pdf_export_image (procedure, config, options, FALSE,
                               (run_mode != CLAY_RUN_NONINTERACTIVE),
                               &error);

  return clay_procedure_new_return_values (procedure, status, error);
}

static cairo_status_t
write_func (void                *fp,
            const unsigned char *data,
            unsigned int         size)
{
  return fwrite (data, 1, size, fp) == size ? CAIRO_STATUS_SUCCESS
                                            : CAIRO_STATUS_WRITE_ERROR;
}

static GList *
get_missing_fonts (GList *layers)
{
  GList *missing_fonts = NULL;
  GList *iter;

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *layer = iter->data;

      if (clay_item_is_group (CLAY_ITEM (layer)))
        {
          GList *child_missing_fonts;
          GList *iter2;

          child_missing_fonts = get_missing_fonts (clay_item_list_children (CLAY_ITEM (layer)));
          for (iter2 = child_missing_fonts; iter2; iter2 = iter2->next)
            {
              gchar *missing = iter2->data;

              if (g_list_find_custom (missing_fonts, missing, (GCompareFunc) g_strcmp0) == NULL)
                missing_fonts = g_list_prepend (missing_fonts, missing);
              else
                g_free (missing);
            }
          g_list_free (child_missing_fonts);
        }
      else if (clay_item_is_text_layer (CLAY_ITEM (layer)))
        {
          ClayFont             *clay_font;
          PangoFontDescription *font_description;
          PangoFontDescription *font_description2;
          PangoFontMap         *fontmap;
          PangoFont            *font;
          PangoContext         *context;

          fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
          context = pango_font_map_create_context (fontmap);

          clay_font        = clay_text_layer_get_font (CLAY_TEXT_LAYER (layer));
          font_description = clay_font_get_pango_font_description (clay_font);

          font = pango_font_map_load_font (fontmap, context, font_description);
          font_description2 = pango_font_describe (font);
          if (g_strcmp0 (pango_font_description_get_family (font_description),
                         pango_font_description_get_family (font_description2)) != 0)
            {
              const gchar *missing = pango_font_description_get_family (font_description);

              if (g_list_find_custom (missing_fonts, missing, (GCompareFunc) g_strcmp0) == NULL)
                missing_fonts = g_list_prepend (missing_fonts,
                                                g_strdup (missing));
            }

          g_object_unref (font);
          pango_font_description_free (font_description);
          pango_font_description_free (font_description2);
          g_object_unref (context);
          g_object_unref (fontmap);
        }
    }

  g_list_free (layers);

  return missing_fonts;
}

static ClayPDBStatusType
pdf_export_image (ClayProcedure        *procedure,
                  ClayProcedureConfig  *config,
                  ClayExportOptions    *options,
                  gboolean              single_image,
                  gboolean              show_progress,
                  GError              **error)
{
  cairo_surface_t *pdf_file;
  cairo_t         *cr;
  FILE            *fp;
  gint             i;
  gboolean         layers_as_pages = FALSE;

  if (single_image)
    g_object_get (config, "layers-as-pages", &layers_as_pages, NULL);

  fp = g_fopen (file_name, "wb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   clay_filename_to_utf8 (file_name), g_strerror (errno));

      return CLAY_PDB_EXECUTION_ERROR;
    }

  pdf_file = cairo_pdf_surface_create_for_stream (write_func, fp, 1, 1);

  if (cairo_surface_status (pdf_file) != CAIRO_STATUS_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("An error occurred while creating the PDF file:\n"
                     "%s\n"
                     "Make sure you entered a valid filename and that the "
                     "selected location isn't read only!"),
                   cairo_status_to_string (cairo_surface_status (pdf_file)));

      return CLAY_PDB_EXECUTION_ERROR;
    }

  cr = cairo_create (pdf_file);

  for (i = 0; i < multi_page.image_count; i++)
    {
      ClayImage     *image = multi_page.images[i];
      ClayLayer    **layers;
      gint32         n_layers;
      gdouble        x_res, y_res;
      gdouble        x_scale, y_scale;
      ClayDrawable **drawables;
      gint           j;

      drawables = clay_image_get_selected_drawables (image);
      if (drawables[0] == NULL)
        {
          g_free (drawables);
          continue;
        }
      g_free (drawables);

      /* Save the state of the surface before any changes, so that
       * settings from one page won't affect all the others
       */
      cairo_save (cr);

      if (! (clay_export_options_get_image (options,
                                            &image) == CLAY_EXPORT_EXPORT))
        {
          /* clay_drawable_histogram() only works within the bounds of
           * the selection, which is a problem (see issue #2431).
           * Instead of saving the selection, unselecting to later
           * reselect, let's just always work on a duplicate of the
           * image.
           */
          image = clay_image_duplicate (image);
        }

      clay_selection_none (image);

      clay_image_get_resolution (image, &x_res, &y_res);
      x_scale = 72.0 / x_res;
      y_scale = 72.0 / y_res;

      cairo_pdf_surface_set_size (pdf_file,
                                  clay_image_get_width  (image) * x_scale,
                                  clay_image_get_height (image) * y_scale);

      /* This way we set how many pixels are there in every inch.
       * It's very important for PangoCairo
       */
      cairo_surface_set_fallback_resolution (pdf_file, x_res, y_res);

      /* Cairo has a concept of user-space vs device-space units.
       * From what I understand, by default the user-space unit is the
       * typographical "point". Since we work mostly with pixels, not
       * points, the following call simply scales the transformation
       * matrix from points to pixels, relatively to the image
       * resolution, knowing that 1 typographical point == 1/72 inch.
       */
      cairo_scale (cr, x_scale, y_scale);

      layers   = clay_image_get_layers (image);
      n_layers = clay_core_object_array_get_length ((GObject **) layers);

      /* Now, we should loop over the layers of each image */
      for (j = 0; j < n_layers; j++)
        {
          if (! draw_layer (image, layers, n_layers, config,
                            single_image, j, cr, x_res, y_res,
                            clay_procedure_get_name (procedure),
                            show_progress,
                            /* Progression is showed per image, and would restart at 0
                             * if you open several images.
                             */
                            (gdouble) j / n_layers,
                            (gdouble) (j + 1) / n_layers,
                            0, error))
            {
              /* free the resources */
              g_free (layers);
              cairo_surface_destroy (pdf_file);
              cairo_destroy (cr);
              fclose (fp);

              return CLAY_PDB_EXECUTION_ERROR;
            }
        }
      if (show_progress)
        clay_progress_update (1.0);

      g_free (layers);

      /* We are done with this image - Show it!
       * Unless that's a multi-page to avoid blank page at the end
       */

      if (! layers_as_pages)
        cairo_show_page (cr);

      cairo_restore (cr);

      clay_image_delete (image);
    }

  /* We are done with all the images - time to free the resources */
  cairo_surface_destroy (pdf_file);
  cairo_destroy (cr);

  fclose (fp);

  return CLAY_PDB_SUCCESS;
}

static ClayExportCapabilities
export_edit_options (ClayProcedure        *procedure,
                     ClayProcedureConfig  *config,
                     ClayExportOptions    *options,
                     gpointer              create_data)
{
  ClayExportCapabilities capabilities;
  gboolean               apply_masks;

  g_object_get (G_OBJECT (config),
                "apply-masks", &apply_masks,
                NULL);

  capabilities = (CLAY_EXPORT_CAN_HANDLE_RGB    |
                  CLAY_EXPORT_CAN_HANDLE_ALPHA  |
                  CLAY_EXPORT_CAN_HANDLE_GRAY   |
                  CLAY_EXPORT_CAN_HANDLE_LAYERS |
                  CLAY_EXPORT_CAN_HANDLE_INDEXED);

  if (! apply_masks)
    capabilities |= CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS;

  return capabilities;
}

/******************************************************/
/* Beginning of parameter handling functions          */
/******************************************************/

/* A function that initializes the image list to default values */
static void
init_image_list_defaults (ClayImage *image)
{
  if (image)
    {
      multi_page.images[0]   = image;
      multi_page.image_count = 1;
    }
  else
    {
      multi_page.image_count = 0;
    }
}

/* A function that removes images that are no longer valid from the
 * image list
 */
static void
validate_image_list (void)
{
  gint32  valid = 0;
  guint32 i     = 0;

  for (i = 0 ; i < MAX_PAGE_COUNT && i < multi_page.image_count ; i++)
    {
      if (clay_image_is_valid (multi_page.images[i]))
        {
          multi_page.images[valid] = multi_page.images[i];
          valid++;
        }
    }

  multi_page.image_count = valid;
}


/******************************************************/
/* Beginning of GUI functions                         */
/******************************************************/

/* The main GUI function for saving single-paged PDFs */

static gboolean
gui_single (ClayProcedure       *procedure,
            ClayProcedureConfig *config,
            ClayImage           *image)
{
  GtkWidget  *window;
  GtkWidget  *widget;
  ClayLayer **layers;
  GList      *missing_fonts;
  GList      *dialog_props = NULL;
  gboolean    run;
  gint32      n_layers;

  clay_ui_init (PLUG_IN_BINARY);

  window = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (window),
                                  "pages-box",
                                  "reverse-order", "root-layers-only", NULL);
  /* XXX the "layers-as-pages" checkbox label used to be changing,
   * showing "top layers first" or "bottom layers first" depending on
   * the value of "reverse-order". Should we want this? Or do it
   * differently, i.e. maybe with an additional label showing the order?
   */
  widget = clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (window),
                                            "pages-frame", "layers-as-pages", FALSE,
                                            "pages-box");
  /* Enable "layers-as-pages" if more than one layer, or there's a single
   * layer group has more than one layer */
  layers   = clay_image_get_layers (multi_page.images[0]);
  n_layers = clay_core_object_array_get_length ((GObject **) layers);

  if (n_layers == 1 && clay_item_is_group (CLAY_ITEM (layers[0])))
    {
      ClayItem **children;

      children = clay_item_get_children (CLAY_ITEM (layers[0]));
      n_layers = clay_core_object_array_get_length ((GObject **) children);
      g_free (children);
    }
  g_free (layers);

  gtk_widget_set_sensitive (widget, n_layers > 1);

  /* Warning for missing fonts (non-embeddable with rasterization
   * possible).
   */
  missing_fonts = get_missing_fonts (clay_image_list_layers (multi_page.images[0]));
  if (missing_fonts != NULL)
    {
      GList *iter;
      gchar *font_list = NULL;
      gchar *text;

      for (iter = missing_fonts; iter; iter = iter->next)
        {
          gchar *fontname = iter->data;

          if (font_list == NULL)
            {
              font_list = g_strdup (fontname);
            }
          else
            {
              gchar *tmp = font_list;

              font_list = g_strjoin (", ", tmp, fontname, NULL);
              g_free (tmp);
            }
        }

      text = g_strdup_printf (_("The following fonts cannot be found: %s.\n"
                                "It is recommended to convert your text layers to image "
                                "or to install the missing fonts before exporting, "
                                "otherwise your design may not look right."),
                              font_list);
      /* TODO: we used to have a GtkImage showing a CLAY_ICON_WILBER_EEK
       * icon in GTK_ICON_SIZE_BUTTON size, next to the label, to make
       * the warning more obvious.
       */
      widget = clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (window),
                                                "missing-fonts-label",
                                                text, FALSE, FALSE);
      gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
      gtk_label_set_max_width_chars (GTK_LABEL (widget), 80);
      clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (window),
                                        "convert-text-layers-frame",
                                        "convert-text-layers", TRUE,
                                        "missing-fonts-label");

      dialog_props = g_list_prepend (dialog_props, "convert-text-layers-frame");

      g_list_free_full (missing_fonts, g_free);
      g_free (text);
      g_free (font_list);
    }
  else
    {
      dialog_props = g_list_prepend (dialog_props, "convert-text-layers");
    }

  dialog_props = g_list_prepend (dialog_props, "fill-background-color");
  dialog_props = g_list_prepend (dialog_props, "ignore-hidden");
  dialog_props = g_list_prepend (dialog_props, "vectorize");
  dialog_props = g_list_prepend (dialog_props, "apply-masks");
  dialog_props = g_list_prepend (dialog_props, "pages-frame");
  clay_procedure_dialog_fill_list (CLAY_PROCEDURE_DIALOG (window),
                                   dialog_props);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (window));

  gtk_widget_destroy (window);

  return run;
}

/* The main GUI function for saving multi-paged PDFs */

static gboolean
gui_multi (ClayProcedure       *procedure,
           ClayProcedureConfig *config)
{
  GtkWidget   *window;
  GtkWidget   *vbox;
  GtkWidget   *file_label;
  GtkWidget   *file_entry;
  GtkWidget   *file_browse;
  GtkWidget   *file_hbox;
  GtkWidget   *scroll;
  GtkWidget   *page_view;
  GtkWidget   *h_but_box;
  GtkWidget   *del;
  GtkWidget   *h_box;
  GtkWidget   *img_combo;
  GtkWidget   *add_image;
  gboolean     run;
  const gchar *temp;

  clay_ui_init (PLUG_IN_BINARY);

  window = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _("Export Image as Multi-Page PDF"));
  clay_procedure_dialog_set_ok_label (CLAY_PROCEDURE_DIALOG (window), _("_Export"));

  vbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (window),
                                         "multi-pdf-options-vbox",
                                         "apply-masks", "vectorize",
                                         "fill-background-color",
                                         "ignore-hidden", NULL);

  file_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  file_label = gtk_label_new (_("Save to:"));
  file_entry = gtk_entry_new ();
  if (file_name != NULL)
    gtk_entry_set_text (GTK_ENTRY (file_entry), file_name);
  file_browse = gtk_button_new_with_label (_("Browse..."));
  file_choose = gtk_file_chooser_native_new (_("Multipage PDF export"),
                                             GTK_WINDOW (window),
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             _("_Save"), _("_Cancel"));

  gtk_box_pack_start (GTK_BOX (file_hbox), file_label, FALSE, FALSE, 0);
  gtk_widget_set_visible (file_label, TRUE);
  gtk_box_pack_start (GTK_BOX (file_hbox), file_entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (file_entry, TRUE);
  gtk_box_pack_start (GTK_BOX (file_hbox), file_browse, FALSE, FALSE, 0);
  gtk_widget_set_visible (file_browse, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), file_hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (file_hbox, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), file_hbox, 0);

  page_view = gtk_icon_view_new ();
  model = create_model ();
  gtk_icon_view_set_model (GTK_ICON_VIEW (page_view), model);
  gtk_icon_view_set_reorderable (GTK_ICON_VIEW (page_view), TRUE);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (page_view),
                                    GTK_SELECTION_MULTIPLE);

  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (page_view), THUMB);
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (page_view), PAGE_NUMBER);
  gtk_icon_view_set_tooltip_column (GTK_ICON_VIEW (page_view), IMAGE_NAME);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, -1, 300);
  gtk_widget_set_visible (scroll, TRUE);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroll), page_view);
  gtk_widget_set_visible (page_view, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), scroll, 1);

  h_but_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (h_but_box), GTK_BUTTONBOX_START);

  del = gtk_button_new_with_label (_("Remove the selected pages"));
  gtk_box_pack_start (GTK_BOX (h_but_box), del, TRUE, TRUE, 0);
  gtk_widget_set_visible (del, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), h_but_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (h_but_box, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), h_but_box, 2);

  h_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

  img_combo = clay_image_combo_box_new (NULL, NULL, NULL);
  gtk_box_pack_start (GTK_BOX (h_box), img_combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (img_combo, TRUE);

  add_image = gtk_button_new_with_label (_("Add this image"));
  gtk_box_pack_start (GTK_BOX (h_box), add_image, FALSE, FALSE, 0);
  gtk_widget_set_visible (add_image, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), h_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (h_box, TRUE);
  gtk_box_reorder_child (GTK_BOX (vbox), h_box, 3);

  g_signal_connect (G_OBJECT (file_browse), "clicked",
                    G_CALLBACK (choose_file_call),
                    file_entry);

  g_signal_connect (G_OBJECT (add_image), "clicked",
                    G_CALLBACK (add_image_call),
                    img_combo);

  g_signal_connect (G_OBJECT (del), "clicked",
                    G_CALLBACK (del_image_call),
                    page_view);

  g_signal_connect (G_OBJECT (model), "row-deleted",
                    G_CALLBACK (remove_call),
                    NULL);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (window),
                              "multi-pdf-options-vbox",
                              NULL);
  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (window));

  if (run)
    {
      run &= get_image_list ();

      temp = gtk_entry_get_text (GTK_ENTRY (file_entry));
      if (temp != NULL)
        {
          g_stpcpy (file_name, temp);
          g_object_set (config, "uri", temp, NULL);
        }
    }

  gtk_widget_destroy (window);

  return run;
}

/* A function that is called when the button for browsing for file
 * locations was clicked
 */
static void
choose_file_call (GtkWidget *browse_button,
                  gpointer   file_entry)
{
  GFile *file = g_file_new_for_path (gtk_entry_get_text (GTK_ENTRY (file_entry)));
  gint   res;

  gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (file_choose),
                            g_file_get_uri (file));

  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (file_choose));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_choose));
      gtk_entry_set_text (GTK_ENTRY (file_entry), g_file_peek_path (file));
    }

  file_name = g_file_get_path (file);
  gtk_native_dialog_hide (GTK_NATIVE_DIALOG (file_choose));
}

/* A function to create the basic GtkTreeModel for the icon view */
static GtkTreeModel*
create_model (void)
{
  GtkListStore *model;
  guint32       i;

  /* validate_image_list was called earlier, so all the images
   * up to multi_page.image_count are valid
   */
  model = gtk_list_store_new (4,
                              GDK_TYPE_PIXBUF,  /* THUMB */
                              G_TYPE_STRING,    /* PAGE_NUMBER */
                              G_TYPE_STRING,    /* IMAGE_NAME */
                              CLAY_TYPE_IMAGE); /* IMAGE */

  for (i = 0 ; i < multi_page.image_count && i < MAX_PAGE_COUNT ; i++)
    {
      GtkTreeIter  iter;
      ClayImage   *image = multi_page.images[i];
      GdkPixbuf   *pixbuf;

      pixbuf = clay_image_get_thumbnail (image, THUMB_WIDTH, THUMB_HEIGHT,
                                         CLAY_PIXBUF_SMALL_CHECKS);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
                          THUMB,       pixbuf,
                          PAGE_NUMBER, g_strdup_printf (_("Page %d"), i + 1),
                          IMAGE_NAME,  clay_image_get_name (image),
                          IMAGE,       image,
                          -1);

      g_object_unref (pixbuf);
    }

  return GTK_TREE_MODEL (model);
}

/* A function that puts the images from the model inside the images
 * (pages) array
 */
static gboolean
get_image_list (void)
{
  GtkTreeIter iter;
  gboolean    valid;

  multi_page.image_count = 0;

  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      ClayImage *image;

      gtk_tree_model_get (model, &iter,
                          IMAGE, &image,
                          -1);

      multi_page.images[multi_page.image_count] = image;
      multi_page.image_count++;

      g_object_unref (image);
    }

  if (multi_page.image_count == 0)
    {
      g_message (_("Error! In order to save the file, at least one image "
                   "should be added!"));
      return FALSE;
    }

  return TRUE;
}

/* A function that is called when the button for adding an image was
 * clicked
 */
static void
add_image_call (GtkWidget *widget,
                gpointer   img_combo)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  gint32        image_id;
  ClayImage    *image;
  GdkPixbuf    *pixbuf;

  dnd_remove = FALSE;

  clay_int_combo_box_get_active (img_combo, &image_id);
  image = clay_image_get_by_id (image_id);

  store = GTK_LIST_STORE (model);

  pixbuf = clay_image_get_thumbnail (image, THUMB_WIDTH, THUMB_HEIGHT,
                                     CLAY_PIXBUF_SMALL_CHECKS);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PAGE_NUMBER, g_strdup_printf (_("Page %d"),
                                                    multi_page.image_count+1),
                      THUMB,       pixbuf,
                      IMAGE_NAME,  clay_image_get_name (image),
                      IMAGE,       image,
                      -1);

  g_object_unref (pixbuf);

  multi_page.image_count++;

  dnd_remove = TRUE;
}

/* A function that is called when the button for deleting the selected
 * images was clicked
 */
static void
del_image_call (GtkWidget *widget,
                gpointer   icon_view)
{
  GList                *list;
  GtkTreeRowReference **items;
  GtkTreePath          *item_path;
  GtkTreeIter           item;
  gpointer              temp;
  guint32               len;

  dnd_remove = FALSE;

  list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));

  len = g_list_length (list);
  if (len > 0)
    {
      gint i;

      items = g_newa (GtkTreeRowReference*, len);

      for (i = 0; i < len; i++)
        {
          temp = g_list_nth_data (list, i);
          items[i] = gtk_tree_row_reference_new (model, temp);
          gtk_tree_path_free (temp);
        }
      g_list_free (list);

      for (i = 0; i < len; i++)
        {
          item_path = gtk_tree_row_reference_get_path (items[i]);

          gtk_tree_model_get_iter (model, &item, item_path);
          gtk_list_store_remove (GTK_LIST_STORE (model), &item);

          gtk_tree_path_free (item_path);

          gtk_tree_row_reference_free (items[i]);
          multi_page.image_count--;
        }
    }

  dnd_remove = TRUE;

  recount_pages ();
}

/* A function that is called on rows-deleted signal. It will call the
 * function to relabel the pages
 */
static void
remove_call (GtkTreeModel *tree_model,
             GtkTreePath  *path,
             gpointer      user_data)
{

  if (dnd_remove)
    /* The gtk documentation says that we should not free the indices array */
    recount_pages ();
}

/* A function to relabel the pages in the icon view, when their order
 * was changed
 */
static void
recount_pages (void)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  gboolean      valid;
  gint32        i = 0;

  store = GTK_LIST_STORE (model);

  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      gtk_list_store_set (store, &iter,
                          PAGE_NUMBER, g_strdup_printf (_("Page %d"), i + 1),
                          -1);
      i++;
    }
}


/******************************************************/
/* Beginning of the actual PDF functions              */
/******************************************************/

static cairo_surface_t *
get_cairo_surface (ClayDrawable  *drawable,
                   gboolean       as_mask,
                   GError       **error)
{
  GeglBuffer      *src_buffer;
  GeglBuffer      *dest_buffer;
  cairo_surface_t *surface;
  cairo_status_t   status;
  cairo_format_t   format;
  gint             width;
  gint             height;

  src_buffer = clay_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  if (as_mask)
    format = CAIRO_FORMAT_A8;
  else if (clay_drawable_has_alpha (drawable))
    format = CAIRO_FORMAT_ARGB32;
  else
    format = CAIRO_FORMAT_RGB24;

  surface = cairo_image_surface_create (format, width, height);

  status = cairo_surface_status (surface);
  if (status != CAIRO_STATUS_SUCCESS)
    {
      switch (status)
        {
        case CAIRO_STATUS_INVALID_SIZE:
          g_set_error_literal (error,
                               CLAY_PLUGIN_PDF_EXPORT_ERROR,
                               CLAY_PLUGIN_PDF_EXPORT_ERROR_FAILED,
                               _("Cannot handle the size (either width or height) of the image."));
          break;
        default:
          g_set_error (error,
                       CLAY_PLUGIN_PDF_EXPORT_ERROR,
                       CLAY_PLUGIN_PDF_EXPORT_ERROR_FAILED,
                       "Cairo error: %s",
                       cairo_status_to_string (status));
          break;
        }

      return NULL;
    }

  dest_buffer = clay_cairo_surface_get_buffer (surface, NULL, TRUE);
  if (as_mask)
    {
      /* src_buffer represents a mask in "Y u8", "Y u16", etc. formats.
       * Yet cairo_mask*() functions only care about the alpha channel of
       * the surface. Hence I change the format of dest_buffer so that the
       * Y channel of src_buffer actually refers to the A channel of
       * dest_buffer/surface in Cairo.
       */
      gegl_buffer_set_format (dest_buffer, babl_format ("Y u8"));
    }

  gegl_buffer_freeze_changed (dest_buffer);
  gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dest_buffer, NULL);
  gegl_buffer_thaw_changed (dest_buffer);

  cairo_surface_mark_dirty (surface);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  return surface;
}

/* A function to check if a drawable is single colored. This allows to
 * convert bitmaps to vector where possible
 */
static GeglColor *
get_layer_color (ClayLayer *layer,
                 gboolean  *single)
{
  GeglColor *col     = gegl_color_new (NULL);
  gdouble    rgba[4] = { 0, 0, 0, 0 };
  gdouble    dev, devSum;
  gdouble    median, pixels, count, percentile;
  gint       bpp;

  devSum = 0;
  dev    = 0;
  bpp    = clay_drawable_get_bpp (CLAY_DRAWABLE (layer));

  if (clay_drawable_is_indexed (CLAY_DRAWABLE (layer)))
    {
      /* FIXME: We can't do a proper histogram on indexed layers! */
      *single = FALSE;
      gegl_color_set_rgba (col, 0.0, 0.0, 0.0, 0.0);
      return col;
    }

  if (clay_drawable_is_rgb (CLAY_DRAWABLE (layer)))
    {
      /* Are we in RGB mode? */

      clay_drawable_histogram (CLAY_DRAWABLE (layer),
                               CLAY_HISTOGRAM_RED, 0.0, 1.0,
                               &rgba[0], &dev, &median, &pixels, &count,
                               &percentile);
      devSum += dev;

      clay_drawable_histogram (CLAY_DRAWABLE (layer),
                               CLAY_HISTOGRAM_GREEN, 0.0, 1.0,
                               &rgba[1], &dev, &median, &pixels, &count,
                               &percentile);
      devSum += dev;

      clay_drawable_histogram (CLAY_DRAWABLE (layer),
                               CLAY_HISTOGRAM_BLUE, 0.0, 1.0,
                               &rgba[2], &dev, &median, &pixels, &count,
                               &percentile);
      devSum += dev;
    }
  else
    {
      /* We are in Grayscale mode (or Indexed) */

      clay_drawable_histogram (CLAY_DRAWABLE (layer),
                               CLAY_HISTOGRAM_VALUE, 0.0, 1.0,
                               &rgba[0], &dev, &median, &pixels, &count,
                               &percentile);
      devSum += dev;
      rgba[1] = rgba[0];
      rgba[2] = rgba[0];
    }

  if (clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
    clay_drawable_histogram (CLAY_DRAWABLE (layer),
                             CLAY_HISTOGRAM_ALPHA, 0.0, 1.0,
                             &rgba[3], &dev, &median, &pixels, &count,
                             &percentile);
  else
    rgba[3] = 255.0f;

  devSum += dev;
  *single = devSum == 0;

  /* If the image is 8-bit, we need to normalize the RGB values that are
   * returned in the 0...255 range */
  bpp -= clay_drawable_has_alpha (CLAY_DRAWABLE (layer)) ? 1 : 0;
  if ((clay_drawable_is_rgb (CLAY_DRAWABLE (layer)) && bpp == 3) ||
       bpp == 1)
    {
      for (gint i = 0; i < 4; i++)
        rgba[i] /= 255.0f;
    }

  switch (clay_image_get_precision (clay_item_get_image (CLAY_ITEM (layer))))
    {
      case CLAY_PRECISION_U8_LINEAR:
      case CLAY_PRECISION_U16_LINEAR:
      case CLAY_PRECISION_U32_LINEAR:
      case CLAY_PRECISION_HALF_LINEAR:
      case CLAY_PRECISION_FLOAT_LINEAR:
      case CLAY_PRECISION_DOUBLE_LINEAR:
        gegl_color_set_rgba (col, rgba[0], rgba[1], rgba[2], rgba[3]);
        break;

      default:
        gegl_color_set_pixel (col, babl_format ("R'G'B'A double"), rgba);
        break;
    }

  return col;
}

/* A function that uses Pango to render the text to our cairo surface,
 * in the same way it was the user saw it inside clay.
 * Needs some work on choosing the font name better, and on hinting
 * (freetype and pango differences)
 */
static void
drawText (ClayLayer *layer,
          gdouble    opacity,
          cairo_t   *cr,
          gdouble    x_res,
          gdouble    y_res)
{
  ClayImageType         type   = clay_drawable_type (CLAY_DRAWABLE (layer));
  gchar                *text   = clay_text_layer_get_text (CLAY_TEXT_LAYER (layer));
  gchar                *markup = clay_text_layer_get_markup (CLAY_TEXT_LAYER (layer));
  gchar                *language;
  cairo_font_options_t *options;
  gint                  x;
  gint                  y;
  GeglColor            *color;
  gdouble               rgb[3];
  ClayUnit             *unit;
  gdouble               size;
  ClayTextHintStyle     hinting;
  ClayTextJustification j;
  gboolean              justify;
  PangoAlignment        align;
  ClayTextDirection     dir;
  PangoLayout          *layout;
  PangoContext         *context;
  ClayFont             *font;
  PangoFontDescription *font_description;
  gdouble               indent;
  gdouble               line_spacing;
  gdouble               letter_spacing;
  PangoAttrList        *attr_list = pango_attr_list_new ();
  PangoFontMap         *fontmap;

  cairo_save (cr);

  options = cairo_font_options_create ();
  attr_list = pango_attr_list_new ();
  cairo_get_font_options (cr, options);

  /* Position */
  clay_drawable_get_offsets (CLAY_DRAWABLE (layer), &x, &y);
  cairo_translate (cr, x, y);

  /* Offset */
  clay_text_layer_get_layout_offsets (CLAY_TEXT_LAYER (layer), &x, &y);
  cairo_translate (cr, x, y);

  /* Color */
  /* When dealing with a gray/indexed image, the viewed color of the text layer
   * can be different than the one kept in the memory */
  if (type == CLAY_RGBA_IMAGE)
    color = clay_text_layer_get_color (CLAY_TEXT_LAYER (layer));
  else
    clay_image_pick_color (clay_item_get_image (CLAY_ITEM (layer)),
                           (const ClayDrawable*[2]) { CLAY_DRAWABLE (layer), NULL },
                           x, y, FALSE, FALSE, 0, &color);

  /* TODO: this export plug-in is not space-aware yet, so we draw everything as
   * sRGB for the time being.
   */
  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' double", NULL), rgb);
  cairo_set_source_rgba (cr, rgb[0], rgb[1], rgb[2], opacity);
  g_object_unref (color);

  /* Hinting */
  hinting = clay_text_layer_get_hint_style (CLAY_TEXT_LAYER (layer));
  switch (hinting)
    {
    case CLAY_TEXT_HINT_STYLE_NONE:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
      break;

    case CLAY_TEXT_HINT_STYLE_SLIGHT:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_SLIGHT);
      break;

    case CLAY_TEXT_HINT_STYLE_MEDIUM:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_MEDIUM);
      break;

    case CLAY_TEXT_HINT_STYLE_FULL:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_FULL);
      break;
    }

  /* Antialiasing */
  if (clay_text_layer_get_antialias (CLAY_TEXT_LAYER (layer)))
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_DEFAULT);
  else
    cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);

  /* We are done with cairo's settings. It's time to create the
   * context
   */
  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);

  /* Font */
  font = clay_text_layer_get_font (CLAY_TEXT_LAYER (layer));
  font_description = clay_font_get_pango_font_description (font);

  /* This function breaks rendering with some fonts if it's called before
   * clay_font_get_pango_font_description(). I'm still unsure why yet it
   * probably means there is a bug somewhere we must fix. Until then, let's make
   * sure we keep this order. XXX
   */
  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap), y_res);

  context = pango_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  pango_cairo_context_set_font_options (context, options);

  /* Language */
  language = clay_text_layer_get_language (CLAY_TEXT_LAYER (layer));
  if (language)
    pango_context_set_language (context,
                                pango_language_from_string (language));

  /* Text Direction */
  dir = clay_text_layer_get_base_direction (CLAY_TEXT_LAYER (layer));

  switch (dir)
    {
    case CLAY_TEXT_DIRECTION_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      break;

    case CLAY_TEXT_DIRECTION_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_RTL);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      break;

    case CLAY_TEXT_DIRECTION_TTB_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_LINE);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_EAST);
      break;

    case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_EAST);
      break;

    case CLAY_TEXT_DIRECTION_TTB_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_LINE);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_WEST);
      break;

    case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_WEST);
      break;
    }

  /* We are done with the context's settings. It's time to create the
   * layout
   */
  layout = pango_layout_new (context);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

  /* Font Size */
  size = clay_text_layer_get_font_size (CLAY_TEXT_LAYER (layer), &unit);
  size = clay_units_to_pixels (size, unit, y_res);
  pango_font_description_set_absolute_size (font_description, size * PANGO_SCALE);

  pango_layout_set_font_description (layout, font_description);

  if (! clay_text_layer_is_dynamic (CLAY_TEXT_LAYER (layer)))
    {
       /* Width */
       if (! PANGO_GRAVITY_IS_VERTICAL (pango_context_get_base_gravity (context)))
         pango_layout_set_width (layout,
                                 clay_drawable_get_width (CLAY_DRAWABLE (layer)) *
                                 PANGO_SCALE);
       else
         pango_layout_set_width (layout,
                                 clay_drawable_get_height (CLAY_DRAWABLE (layer)) *
                                 PANGO_SCALE);
    }
  /* Justification, and Alignment */
  justify = FALSE;
  j = clay_text_layer_get_justification (CLAY_TEXT_LAYER (layer));
  align = PANGO_ALIGN_LEFT;
  switch (j)
    {
    case CLAY_TEXT_JUSTIFY_LEFT:
      align = PANGO_ALIGN_LEFT;
      break;
    case CLAY_TEXT_JUSTIFY_RIGHT:
      align = PANGO_ALIGN_RIGHT;
      break;
    case CLAY_TEXT_JUSTIFY_CENTER:
      align = PANGO_ALIGN_CENTER;
      break;
    case CLAY_TEXT_JUSTIFY_FILL:
      align = PANGO_ALIGN_LEFT;
      justify = TRUE;
      break;
    }
  pango_layout_set_alignment (layout, align);
  pango_layout_set_justify (layout, justify);

  /* Indentation */
  indent = clay_text_layer_get_indent (CLAY_TEXT_LAYER (layer));
  pango_layout_set_indent (layout, pango_units_from_double (clay_units_to_pixels (indent, unit, y_res)));

  /* Line Spacing */
  line_spacing = clay_text_layer_get_line_spacing (CLAY_TEXT_LAYER (layer));
  pango_layout_set_spacing (layout, pango_units_from_double (clay_units_to_pixels (line_spacing, unit, y_res)));

  /* Letter Spacing */
  letter_spacing = clay_text_layer_get_letter_spacing (CLAY_TEXT_LAYER (layer));
  /*
   * for some reason if the letter spacing is not specified in the pango markup
   * (and is instead set for the pango layout) it is ignored.
   */
  if (fabs (letter_spacing) > 0.1)
    {
       if (markup != NULL && markup[0] != '\0')
         markup = g_strdup_printf ("<span letter_spacing=\"%d\">%s</span>",
                                   (gint) (letter_spacing * PANGO_SCALE),
                                   markup);
       else
         markup = g_strdup_printf ("<span letter_spacing=\"%d\">%s</span>",
                                   (gint) (letter_spacing * PANGO_SCALE),
                                   text);
    }


  pango_layout_set_attributes (layout, attr_list);

  /* Use the pango markup of the text layer */

  if (markup != NULL && markup[0] != '\0')
    pango_layout_set_markup (layout, markup, -1);
  else /* If we can't find a markup, then it has just text */
    pango_layout_set_text (layout, text, -1);

  if (dir == CLAY_TEXT_DIRECTION_TTB_RTL ||
      dir == CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT)
    {
      cairo_translate (cr, clay_drawable_get_width (CLAY_DRAWABLE (layer)), 0);
      cairo_rotate (cr, G_PI_2);
    }

  if (dir == CLAY_TEXT_DIRECTION_TTB_LTR ||
      dir == CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT)
    {
      cairo_translate (cr, 0, clay_drawable_get_height (CLAY_DRAWABLE (layer)));
      cairo_rotate (cr, -G_PI_2);
    }

  pango_cairo_show_layout (cr, layout);

  if (fabs (letter_spacing) > 0.1)
    g_free (markup);

  g_free (text);
  g_free (language);

  g_object_unref (layout);
  pango_font_description_free (font_description);
  g_object_unref (context);
  pango_attr_list_unref (attr_list);

  cairo_font_options_destroy (options);

  cairo_restore (cr);
}

static void
drawPath (ClayLayer *layer,
          gdouble    opacity,
          cairo_t   *cr,
          gdouble    x_res,
          gdouble    y_res)
{
  ClayVectorLayer *vector_layer;
  ClayPath        *path    = NULL;
  ClayPattern     *pattern = NULL;
  GeglColor       *color;
  gdouble          rgb[3];
  gboolean         set_fill;
  gboolean         set_stroke;
  gsize            num_strokes;
  gint            *strokes;
  gboolean         closed = FALSE;

  vector_layer = CLAY_VECTOR_LAYER (layer);
  path         = clay_vector_layer_get_path (vector_layer);
  set_fill     = clay_vector_layer_get_enable_fill (vector_layer);
  set_stroke   = clay_vector_layer_get_enable_stroke (vector_layer);

  cairo_save (cr);

  /* Create path in Cairo */
  strokes = clay_path_get_strokes (path, &num_strokes);
  for (gint s = 0; s < num_strokes; s++)
    {
      ClayPathStrokeType  type;
      gdouble            *control_points;
      gsize               num_points;
      gsize               num_indexes;

      type = clay_path_stroke_get_points (path, strokes[s], &num_points,
                                          &control_points, &closed);

      if (type == CLAY_PATH_STROKE_TYPE_BEZIER)
        {
          num_indexes = num_points / 2;

          if (num_indexes >= 3)
            cairo_move_to (cr, control_points[2], control_points[3]);

          for (gint i = 2; i < (num_indexes + (closed ? 2 : - 1)); i += 3)
            {
              gdouble delta[6] = { 0 };

              for (gint j = 0; j < 3; j++)
                {
                  gint index = ((i + j) % num_indexes) * 2;

                  delta[(j * 2)]     = control_points[index];
                  delta[(j * 2) + 1] = control_points[index + 1];
                }

              cairo_curve_to (cr, delta[0], delta[1], delta[2], delta[3],
                              delta[4], delta[5]);
            }

          if (closed && num_points > 3)
            cairo_close_path (cr);
        }
    }

  /* Add fill/stroke to Cairo path */
  if (set_fill)
    {
      color = clay_vector_layer_get_fill_color (vector_layer);
      if (color)
        {
          gegl_color_get_pixel (color,
                                babl_format_with_space ("R'G'B' double", NULL),
                                rgb);
          cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
          g_object_unref (color);
        }
      else
        {
          pattern = clay_vector_layer_get_fill_pattern (vector_layer);

          draw_path_pattern (cr, pattern);
        }

      if (set_stroke)
        cairo_fill_preserve (cr);
      else
        cairo_fill (cr);
    }

  if (set_stroke)
    {
      cairo_line_join_t  join_style;
      cairo_line_cap_t   cap_style;
      gdouble            stroke_width;
      gdouble            miter_limit;
      gdouble            dash_offset;
      gsize              num_dashes;
      gdouble           *dash_pattern;

      color = clay_vector_layer_get_stroke_color (vector_layer);
      if (color)
        {
          gegl_color_get_pixel (color,
                                babl_format_with_space ("R'G'B' double", NULL),
                                rgb);
          cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
          g_object_unref (color);
        }
      else
        {
          pattern = clay_vector_layer_get_stroke_pattern (vector_layer);

          draw_path_pattern (cr, pattern);
        }

      stroke_width = clay_vector_layer_get_stroke_width (vector_layer);
      cairo_set_line_width (cr, stroke_width);

      miter_limit = clay_vector_layer_get_stroke_miter_limit (vector_layer);
      cairo_set_miter_limit (cr, miter_limit);

      switch (clay_vector_layer_get_stroke_cap_style (vector_layer))
        {
        case CLAY_CAP_ROUND:
          cap_style = CAIRO_LINE_CAP_ROUND;
          break;
        case CLAY_CAP_SQUARE:
          cap_style = CAIRO_LINE_CAP_SQUARE;
          break;
        default:
          cap_style = CAIRO_LINE_CAP_BUTT;
          break;
        }
      cairo_set_line_cap (cr, cap_style);

      switch (clay_vector_layer_get_stroke_join_style (vector_layer))
        {
        case CLAY_JOIN_ROUND:
          join_style = CAIRO_LINE_JOIN_ROUND;
          break;
        case CLAY_JOIN_BEVEL:
          join_style = CAIRO_LINE_JOIN_BEVEL;
          break;
        default:
          join_style = CAIRO_LINE_JOIN_MITER;
          break;
        }
      cairo_set_line_join (cr, join_style);

      dash_offset = clay_vector_layer_get_stroke_dash_offset (vector_layer);
      clay_vector_layer_get_stroke_dash_pattern (vector_layer, &num_dashes,
                                                 &dash_pattern);
      if (num_dashes > 0)
        {
          for (gint i = 0; i < num_dashes; i++)
            dash_pattern[i] *= stroke_width;

          cairo_set_dash (cr, dash_pattern, num_dashes, dash_offset);
        }

      cairo_stroke (cr);
    }

  cairo_restore (cr);
}

static void
draw_path_pattern (cairo_t     *cr,
                   ClayPattern *pattern)
{
  cairo_surface_t *pattern_image = NULL;
  ClayImage       *temp_image;
  ClayLayer       *temp_layer;
  gint             width;
  gint             height;
  GeglBuffer      *src_buffer;
  GeglBuffer      *dst_buffer;

  src_buffer = clay_pattern_get_buffer (pattern, 0, 0,
                                        babl_format ("R'G'B' u8"));
  width      = gegl_buffer_get_width (src_buffer);
  height     = gegl_buffer_get_height (src_buffer);
  temp_image = clay_image_new (width, height, CLAY_RGB);
  temp_layer = clay_layer_new (temp_image, NULL, width, height,
                               CLAY_RGBA_IMAGE, 100.0,
                               CLAY_LAYER_MODE_NORMAL);
  clay_image_insert_layer (temp_image, temp_layer, NULL, 0);
  dst_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (temp_layer));
  gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dst_buffer, NULL);

  g_object_unref (src_buffer);
  g_object_unref (dst_buffer);

  pattern_image = get_cairo_surface (CLAY_DRAWABLE (temp_layer), FALSE,
                                     NULL);
  clay_image_delete (temp_image);

  cairo_set_source_surface (cr, pattern_image, 0, 0);
  cairo_surface_destroy (pattern_image);
  cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
}

static gboolean
draw_layer (ClayImage            *image,
            ClayLayer           **layers,
            gint                  n_layers,
            ClayProcedureConfig  *config,
            gboolean              single_image,
            gint                  j,
            cairo_t              *cr,
            gdouble               x_res,
            gdouble               y_res,
            const gchar          *name,
            gboolean              show_progress,
            gdouble               progress_start,
            gdouble               progress_end,
            gint                  layer_level,
            GError              **error)
{
  ClayLayer *layer;
  gdouble    opacity;
  gboolean   vectorize;
  gboolean   ignore_hidden;
  gboolean   layers_as_pages = FALSE;
  gboolean   reverse_order   = FALSE;
  gboolean   root_layers_only;
  gboolean   convert_text;
  gboolean   fill_background_color;

  g_object_get (config,
                "vectorize",             &vectorize,
                "ignore-hidden",         &ignore_hidden,
                "fill-background-color", &fill_background_color,
                NULL);

  if (single_image)
    g_object_get (config,
                  "layers-as-pages",     &layers_as_pages,
                  "reverse-order",       &reverse_order,
                  "root-layers-only",    &root_layers_only,
                  "convert-text-layers", &convert_text,
                  NULL);

  if (reverse_order && layers_as_pages)
    layer = layers [j];
  else
    layer = layers [n_layers - j - 1];

  opacity = clay_layer_get_opacity (layer) / 100.0;
  if ((! clay_item_get_visible (CLAY_ITEM (layer)) || opacity == 0.0) &&
      ignore_hidden)
    return TRUE;

  if (clay_item_is_group (CLAY_ITEM (layer)))
    {
      ClayItem **children;
      gint       children_num = 0;
      gint       i;

      children     = clay_item_get_children (CLAY_ITEM (layer));
      children_num = clay_core_object_array_get_length ((GObject **) children);

      for (i = 0; children[i] != NULL; i++)
        {
          if (! draw_layer (image, (ClayLayer **) children, children_num,
                            config, single_image, i,
                            cr, x_res, y_res, name,
                            show_progress,
                            progress_start + i * (progress_end - progress_start) / children_num,
                            progress_end,
                            layer_level + 1, error))
            {
              g_free (children);
              return FALSE;
            }
        }
      g_free (children);

      if (root_layers_only && layers_as_pages &&
          children_num > 0 && layer_level == 0)
        cairo_show_page (cr);
    }
  else
    {
      cairo_surface_t *mask_image = NULL;
      ClayLayerMask   *mask       = NULL;
      gint             x, y;
      gboolean         fill_page  = FALSE;

      if (show_progress)
        clay_progress_update (progress_start);

      /* First fill layer with opaque background color when either
       * multi-layered or this is the first layer at the top level
       * and user chose that option.
       * Later we paint over it with the layer's content.
       */
      if (layers_as_pages)
        {
          if ((root_layers_only && (layer_level == 0 || j == 0)) ||
              ! root_layers_only)
            fill_page = TRUE;
        }
      else
        {
          if (layer_level == 0 && j == 0)
            fill_page = TRUE;
        }

      if (fill_page && fill_background_color)
        {
          GeglColor *color = NULL;
          gdouble    rgb[3];

          cairo_rectangle (cr, 0.0, 0.0,
                           clay_image_get_width  (image),
                           clay_image_get_height (image));
          color = clay_context_get_background ();
          gegl_color_get_pixel (color, babl_format ("R'G'B' double"), rgb);
          cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
          cairo_fill (cr);
          g_clear_object (&color);
        }

      /* Only render layer mask if it's not disabled */
      if (clay_layer_get_mask (layer) && clay_layer_get_apply_mask (layer))
        mask = clay_layer_get_mask (layer);

      if (mask)
        {
          mask_image = get_cairo_surface (CLAY_DRAWABLE (mask), TRUE,
                                          error);

          if (*error)
            return FALSE;
        }

      clay_drawable_get_offsets (CLAY_DRAWABLE (layer), &x, &y);

      if (((! clay_item_is_text_layer (CLAY_ITEM (layer))               ||
           clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer))) &&
           ! clay_item_is_vector_layer (CLAY_ITEM (layer)))             ||
          convert_text)
        {
          /* For raster layers */

          GeglColor *layer_color;
          gboolean    single_color = FALSE;

          layer_color = get_layer_color (layer, &single_color);

          cairo_rectangle (cr, x, y,
                           clay_drawable_get_width  (CLAY_DRAWABLE (layer)),
                           clay_drawable_get_height (CLAY_DRAWABLE (layer)));

          if (vectorize && single_color)
            {
              clay_cairo_set_source_color (cr, layer_color, NULL, FALSE, NULL);
              if (mask)
                cairo_mask_surface (cr, mask_image, x, y);
              else
                cairo_fill (cr);
            }
          else
            {
              cairo_surface_t *layer_image;

              layer_image = get_cairo_surface (CLAY_DRAWABLE (layer), FALSE,
                                               error);

              if (*error)
                return FALSE;

              cairo_clip (cr);

              cairo_set_source_surface (cr, layer_image, x, y);
              cairo_push_group (cr);
              cairo_paint_with_alpha (cr, opacity);
              cairo_pop_group_to_source (cr);

              if (mask)
                cairo_mask_surface (cr, mask_image, x, y);
              else
                cairo_paint (cr);

              cairo_reset_clip (cr);

              cairo_surface_destroy (layer_image);
            }

          g_object_unref (layer_color);
        }
      else if (clay_item_is_text_layer (CLAY_ITEM (layer)))
        {
          /* For text layers */
          drawText (layer, opacity, cr, x_res, y_res);
        }
      else if (clay_item_is_vector_layer (CLAY_ITEM (layer)))
        {
          drawPath (layer, opacity, cr, x_res, y_res);
        }

      /* draw new page if "layers as pages" option is checked */
      if (layers_as_pages && (! root_layers_only || layer_level == 0))
        cairo_show_page (cr);

      /* We are done with the layer - time to free some resources */
      if (mask)
        cairo_surface_destroy (mask_image);
    }

  return TRUE;
}
