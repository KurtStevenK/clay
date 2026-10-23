/* xyz.kainzmayer.ClayDesign
 *
 * file-pdf-load.c - PDF file loader
 *
 * Copyright (C) 2005 Nathan Summers
 *
 * Some code in render_page_to_surface() borrowed from
 * poppler.git/glib/poppler-page.cc.
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

#include <string.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#undef GTK_DISABLE_SINGLE_INCLUDES
#include <poppler.h>
#define GTK_DISABLE_SINGLE_INCLUDES

#include "libclay/stdplugins-intl.h"


#define LOAD_PROC       "file-pdf-load"
#define LOAD_THUMB_PROC "file-pdf-load-thumb"
#define PLUG_IN_BINARY  "file-pdf-load"
#define PLUG_IN_ROLE    "clay-file-pdf-load"

#define THUMBNAIL_SIZE  128

#define CLAY_PLUGIN_PDF_LOAD_ERROR clay_plugin_pdf_load_error_quark ()
static GQuark
clay_plugin_pdf_load_error_quark (void)
{
  return g_quark_from_static_string ("clay-plugin-pdf-load-error-quark");
}

typedef struct
{
  gsize  n_pages;
  gint  *pages;
} PdfSelectedPages;


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

GType                   pdf_get_type         (void);

static GList          * pdf_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * pdf_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static gboolean         pdf_extract          (ClayProcedure        *procedure,
                                              ClayRunMode           run_mode,
                                              GFile                *file,
                                              ClayMetadata         *metadata,
                                              ClayProcedureConfig  *config,
                                              ClayVectorLoadData   *extracted_dimensions,
                                              gpointer             *data_for_run,
                                              GDestroyNotify       *data_for_run_destroy,
                                              gpointer              extract_data,
                                              GError              **error);
static ClayValueArray * pdf_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              gint                   width,
                                              gint                   height,
                                              ClayVectorLoadData     extracted_data,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               data_from_extract,
                                              gpointer               run_data);
static ClayValueArray * pdf_load_thumb       (ClayProcedure         *procedure,
                                              GFile                 *file,
                                              gint                   size,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);

static ClayImage       * load_image          (PopplerDocument       *doc,
                                              GFile                 *file,
                                              ClayVectorLoadData     extracted_data,
                                              ClayRunMode            run_mode,
                                              ClayPageSelectorTarget target,
                                              gint                   width,
                                              gint                   height,
                                              gdouble                resolution,
                                              gboolean               antialias,
                                              gboolean               white_background,
                                              gboolean               reverse_order,
                                              PdfSelectedPages      *pages);

static ClayPDBStatusType load_dialog         (PopplerDocument       *doc,
                                              PdfSelectedPages      *pages,
                                              ClayVectorLoadData     extracted_data,
                                              ClayProcedure         *procedure,
                                              ClayProcedureConfig   *config);

static PopplerDocument * open_document       (GFile                 *file,
                                              const gchar           *PDF_password,
                                              ClayRunMode            run_mode,
                                              gdouble               *width,
                                              gdouble               *height,
                                              GError               **error);

static cairo_surface_t * get_thumb_surface   (PopplerDocument       *doc,
                                              gint                   page,
                                              gint                   preferred_size,
                                              gboolean               white_background);

static GdkPixbuf       * get_thumb_pixbuf    (PopplerDocument       *doc,
                                              gint                   page,
                                              gint                   preferred_size,
                                              gboolean               white_background);

static ClayLayer       * layer_from_surface  (ClayImage             *image,
                                              const gchar           *layer_name,
                                              gint                   position,
                                              cairo_surface_t       *surface,
                                              gdouble                progress_start,
                                              gdouble                progress_scale);


G_DEFINE_TYPE (Pdf, pdf, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (PDF_TYPE)
DEFINE_STD_SET_I18N


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

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static ClayProcedure *
pdf_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_vector_load_procedure_new (plug_in, name,
                                                  CLAY_PDB_PROC_TYPE_PLUGIN,
                                                  pdf_extract, NULL, NULL,
                                                  pdf_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Portable Document Format"));

      clay_procedure_set_documentation (procedure,
                                        "Load file in PDF format",
                                        "Loads files in Adobe's Portable "
                                        "Document Format. PDF is designed to "
                                        "be easily processed by a variety "
                                        "of different platforms, and is a "
                                        "distant cousin of PostScript.",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Nathan Summers, Lionel N.",
                                      "Nathan Summers, Lionel N.",
                                      "2005, 2017");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("PDF"));
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "application/pdf");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "pdf");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0, string,%PDF-");

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);

      clay_procedure_add_string_argument (procedure, "password",
                                          _("PDF password"),
                                          _("The password to decrypt the encrypted PDF file"),
                                          NULL,
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "reverse-order",
                                           _("Load in re_verse order"),
                                           _("Load PDF pages in reverse order"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      /* FIXME: this should be a clay_procedure_add_enum_argument () of type
       * CLAY_TYPE_PAGE_SELECTOR_TARGET but it won't work right now (see FIXME
       * comment in libclay/claygpparams-body.c:116).

      clay_procedure_add_enum_argument (procedure, "target",
                                        _("Open pages as"),
                                        _("Number of pages to load (0 for all)"),
                                        CLAY_TYPE_PAGE_SELECTOR_TARGET,
                                        CLAY_PAGE_SELECTOR_TARGET_LAYERS,
                                        G_PARAM_READWRITE);

       */
      clay_procedure_add_int_aux_argument (procedure, "target",
                                           _("Open pages as"),
                                           _("Number of pages to load (0 for all)"),
                                           CLAY_PAGE_SELECTOR_TARGET_LAYERS, CLAY_PAGE_SELECTOR_TARGET_IMAGES,
                                           CLAY_PAGE_SELECTOR_TARGET_LAYERS,
                                           G_PARAM_READWRITE);

      /* FIXME: shouldn't the whole selector be considered as one argument
       * containing properties "target" and "pages" as a single object?
       *
       * Or actually should we store pages at all? While it makes sense to store
       * some settings generally, not sure that the list of page makes sense
       * from one PDF document loaded to another (different) one.
       */
      clay_procedure_add_int32_array_argument (procedure, "pages",
                                               _("Pages"),
                                               _("The pages to load in the expected order"),
                                               G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "antialias",
                                           _("Use _Anti-aliasing"),
                                           _("Render texts with anti-aliasing"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "white-background",
                                           _("_Fill transparent areas with white"),
                                           _("Render all pages as opaque by filling the background in white"),
                                           TRUE,
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                pdf_load_thumb, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Loads a preview from a PDF file.",
                                        "Loads a small preview of the first "
                                        "page of the PDF format file. Uses "
                                        "the embedded thumbnail if present.",
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Nathan Summers",
                                      "Nathan Summers",
                                      "2005");
    }

  return procedure;
}

static gboolean
pdf_extract (ClayProcedure        *procedure,
             ClayRunMode           run_mode,
             GFile                *file,
             ClayMetadata         *metadata,
             ClayProcedureConfig  *config,
             ClayVectorLoadData   *extracted_dimensions,
             gpointer             *data_for_run,
             GDestroyNotify       *data_for_run_destroy,
             gpointer              extract_data,
             GError              **error)
{
  PopplerDocument     *doc      = NULL;
  gchar               *password = NULL;
  gdouble              width    = 0.0;
  gdouble              height   = 0.0;

  if (run_mode == CLAY_RUN_INTERACTIVE)
    clay_ui_init (PLUG_IN_BINARY);

  g_object_get (config, "password", &password, NULL);
  doc = open_document (file, password, run_mode,
                       &width, &height, error);
  g_free (password);
  if (doc == NULL)
    return FALSE;

  extracted_dimensions->width         = width;
  extracted_dimensions->height        = height;
  extracted_dimensions->width_unit    = clay_unit_point ();
  extracted_dimensions->height_unit   = clay_unit_point ();
  extracted_dimensions->exact_width   = TRUE;
  extracted_dimensions->exact_height  = TRUE;
  extracted_dimensions->correct_ratio = TRUE;

  *data_for_run = doc;
  *data_for_run_destroy = g_object_unref;

  return TRUE;
}

static ClayValueArray *
pdf_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          gint                   width,
          gint                   height,
          ClayVectorLoadData     extracted_data,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               data_from_extract,
          gpointer               run_data)
{
  ClayValueArray      *return_vals;
  ClayPDBStatusType    status   = CLAY_PDB_SUCCESS;
  ClayImage           *image    = NULL;
  PopplerDocument     *doc      = POPPLER_DOCUMENT (data_from_extract);
  PdfSelectedPages     pages    = { 0, NULL };
  GError              *error    = NULL;

  gegl_init (NULL, NULL);

  if (doc == NULL)
    {
      status = CLAY_PDB_EXECUTION_ERROR;
    }
  else if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      status = load_dialog (doc, &pages, extracted_data, procedure, config);
    }
  else if (run_mode == CLAY_RUN_NONINTERACTIVE)
    {
      PopplerPage *test_page = poppler_document_get_page (doc, 0);

      if (test_page)
        {
          ClayArray    *pages_array;
          const gint32 *page_numbers;
          gint          i;
          gint          doc_n_pages;

          g_object_get (config, "pages", &pages_array, NULL);
          page_numbers = clay_int32_array_get_values (pages_array, &pages.n_pages);

          doc_n_pages = poppler_document_get_n_pages (doc);
          /* The number of imported pages may be bigger than
           * the number of pages from the original document.
           * Indeed it is possible to duplicate some pages
           * by setting the same number several times in the
           * "pages" argument.
           * Not ceiling this value is *not* an error.
           */
          if (pages.n_pages <= 0)
            {
              pages.n_pages = doc_n_pages;
              pages.pages = g_new (gint, pages.n_pages);
              for (i = 0; i < pages.n_pages; i++)
                pages.pages[i] = i;
            }
          else
            {
              pages.pages = g_new (gint, pages.n_pages);

              for (i = 0; i < pages.n_pages; i++)
                {
                  if (page_numbers[i] >= doc_n_pages)
                    {
                      status = CLAY_PDB_EXECUTION_ERROR;
                      g_set_error (&error, CLAY_PLUGIN_PDF_LOAD_ERROR, 0,
                                   /* TRANSLATORS: first argument is file name,
                                    * second is out-of-range page number,
                                    * third is number of pages.
                                    * Specify order as in English if needed.
                                    */
                                   ngettext ("PDF document '%1$s' has %3$d page. Page %2$d is out of range.",
                                             "PDF document '%1$s' has %3$d pages. Page %2$d is out of range.",
                                             doc_n_pages),
                                   clay_file_get_utf8_name (file),
                                   page_numbers[i],
                                   doc_n_pages);
                      break;
                    }
                  else
                    {
                      pages.pages[i] = page_numbers[i];
                    }
                }
            }

          g_object_unref (test_page);
        }
    }

  if (status == CLAY_PDB_SUCCESS)
    {
      ClayPageSelectorTarget target;
      gboolean               reverse_order;
      gdouble                resolution;
      gboolean               antialias;
      gboolean               white_background;

      g_object_get (config,
                    "target",           &target,
                    "reverse-order",    &reverse_order,
                    "width",            &width,
                    "height",           &height,
                    "pixel-density",    &resolution,
                    "antialias",        &antialias,
                    "white-background", &white_background,
                    NULL);
      image = load_image (doc,
                          file, extracted_data,
                          run_mode,
                          target,
                          width, height,
                          resolution,
                          antialias,
                          white_background,
                          reverse_order,
                          &pages);

      if (image == NULL)
        status = CLAY_PDB_EXECUTION_ERROR;
    }

  g_free (pages.pages);

  return_vals = clay_procedure_new_return_values (procedure, status, error);
  if (status == CLAY_PDB_SUCCESS)
    CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayValueArray *
pdf_load_thumb (ClayProcedure       *procedure,
                GFile               *file,
                gint                 size,
                ClayProcedureConfig *config,
                gpointer             run_data)
{
  ClayValueArray  *return_vals;
  gdouble          width  = 0;
  gdouble          height = 0;
  gdouble          scale;
  ClayImage       *image     = NULL;
  gint             num_pages = 0;
  PopplerDocument *doc       = NULL;
  cairo_surface_t *surface   = NULL;
  GError          *error     = NULL;

  gegl_init (NULL, NULL);

  doc = open_document (file, NULL, CLAY_RUN_NONINTERACTIVE,
                       NULL, NULL, &error);

  if (doc)
    {
      PopplerPage *page = poppler_document_get_page (doc, 0);

      if (page)
        {
          poppler_page_get_size (page, &width, &height);

          g_object_unref (page);
        }

      num_pages = poppler_document_get_n_pages (doc);

      surface = get_thumb_surface (doc, 0, size, TRUE);

      g_object_unref (doc);
    }

  if (surface)
    {
      image = clay_image_new (cairo_image_surface_get_width (surface),
                              cairo_image_surface_get_height (surface),
                              CLAY_RGB);

      clay_image_undo_disable (image);

      layer_from_surface (image, "thumbnail", 0, surface, 0.0, 1.0);
      cairo_surface_destroy (surface);

      clay_image_undo_enable (image);
      clay_image_clean_all (image);
    }

  /* Thumbnail resolution: 100.0. */
  scale = 100.0 / clay_unit_get_factor (clay_unit_point ());

  width  *= scale;
  height *= scale;

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);
  CLAY_VALUES_SET_INT   (return_vals, 2, width);
  CLAY_VALUES_SET_INT   (return_vals, 3, height);
  CLAY_VALUES_SET_ENUM  (return_vals, 4, CLAY_RGB_IMAGE);
  CLAY_VALUES_SET_INT   (return_vals, 5, num_pages);

  return return_vals;
}

static PopplerDocument *
open_document (GFile        *file,
               const gchar  *PDF_password,
               ClayRunMode   run_mode,
               gdouble      *width,
               gdouble      *height,
               GError      **load_error)
{
  PopplerDocument *doc;
  GError          *error = NULL;

  doc = poppler_document_new_from_gfile (file, PDF_password, NULL, &error);

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      GtkWidget *label;

      label = gtk_label_new (_("PDF is password protected, please input the password:"));
      while (error                          &&
             error->domain == POPPLER_ERROR &&
             error->code == POPPLER_ERROR_ENCRYPTED)
        {
          GtkWidget *vbox;
          GtkWidget *dialog;
          GtkWidget *entry;
          gint       run;

          dialog = clay_dialog_new (_("Encrypted PDF"), PLUG_IN_ROLE,
                                    NULL, 0,
                                    NULL, NULL,
                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_OK"), GTK_RESPONSE_OK,
                                    NULL);
          clay_window_set_transient (GTK_WINDOW (dialog));
          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
          gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
          gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                              vbox, TRUE, TRUE, 0);
          entry = gtk_entry_new ();
          gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
          gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
          gtk_container_add (GTK_CONTAINER (vbox), label);
          gtk_container_add (GTK_CONTAINER (vbox), entry);

          gtk_widget_show_all (dialog);

          run = clay_dialog_run (CLAY_DIALOG (dialog));
          if (run == GTK_RESPONSE_OK)
            {
              g_clear_error (&error);
              doc = poppler_document_new_from_gfile (file,
                                                     gtk_entry_get_text (GTK_ENTRY (entry)),
                                                     NULL, &error);
            }
          label = gtk_label_new (_("Wrong password! Please input the right one:"));
          gtk_widget_destroy (dialog);
          if (run == GTK_RESPONSE_CANCEL || run == GTK_RESPONSE_DELETE_EVENT)
            {
              break;
            }
        }

      gtk_widget_destroy (label);
    }

  /* We can't g_mapped_file_unref(mapped_file) as apparently doc has
   * references to data in there. No big deal, this is just a
   * short-lived plug-in.
   */
  if (! doc)
    {
      g_set_error (load_error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not load '%s': %s"),
                   clay_file_get_utf8_name (file),
                   error->message);
      g_error_free (error);
      return NULL;
    }

  if (width && height)
    {
      gint n_pages;

      n_pages = poppler_document_get_n_pages (doc);
      for (gint i = 0; i < n_pages; i++)
        {
          PopplerPage *page;

          page = poppler_document_get_page (doc, i);
          poppler_page_get_size (page, width, height);
          g_object_unref (page);

          /* Only check the first page for the whole document size. */
          break;
        }
    }

  return doc;
}

static ClayLayer *
layer_from_surface (ClayImage       *image,
                    const gchar     *layer_name,
                    gint             position,
                    cairo_surface_t *surface,
                    gdouble          progress_start,
                    gdouble          progress_scale)
{
  ClayLayer *layer;

  layer = clay_layer_new_from_surface (image, layer_name, surface,
                                       progress_start,
                                       progress_start + progress_scale);
  clay_image_insert_layer (image, layer, NULL, position);

  return layer;
}

static cairo_surface_t *
render_page_to_surface (PopplerPage *page,
                        int          width,
                        int          height,
                        double       scale,
                        gboolean     antialias,
                        gboolean     white_background)
{
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  cairo_save (cr);
  cairo_translate (cr, 0.0, 0.0);

  if (scale != 1.0)
    cairo_scale (cr, scale, scale);

  if (! antialias)
    {
      cairo_font_options_t *options = cairo_font_options_create ();

      cairo_get_font_options (cr, options);
      cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);
      cairo_set_font_options (cr, options);
      cairo_font_options_destroy (options);

      cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    }

  poppler_page_render (page, cr);
  cairo_restore (cr);

  if (white_background)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_paint (cr);
    }

  cairo_destroy (cr);

  return surface;
}

#if 0

/* This is currently unused, but we'll have it here in case the military
   wants it. */

static GdkPixbuf *
render_page_to_pixbuf (PopplerPage *page,
                       int          width,
                       int          height,
                       double       scale)
{
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;

  surface = render_page_to_surface (page, width, height, scale);
  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                        cairo_image_surface_get_width (surface),
                                        cairo_image_surface_get_height (surface));
  cairo_surface_destroy (surface);

  return pixbuf;
}

#endif

static ClayImage *
load_image (PopplerDocument        *doc,
            GFile                  *file,
            ClayVectorLoadData      extracted_data,
            ClayRunMode             run_mode,
            ClayPageSelectorTarget  target,
            gint                    doc_width,
            gint                    doc_height,
            gdouble                 resolution,
            gboolean                antialias,
            gboolean                white_background,
            gboolean                reverse_order,
            PdfSelectedPages       *pages)
{
  ClayImage  *image        = NULL;
  ClayImage **images       = NULL;
  gint        i;
  gdouble     scale;
  gdouble     doc_progress = 0;
  gint        base_index   = 0;
  gint        sign         = 1;

  if (reverse_order && pages->n_pages > 0)
    {
      base_index = pages->n_pages - 1;
      sign = -1;
    }

  if (target == CLAY_PAGE_SELECTOR_TARGET_IMAGES)
    images = g_new0 (ClayImage *, pages->n_pages);

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  scale = (gdouble) doc_width / extracted_data.width;

  /* read the file */

  for (i = 0; i < pages->n_pages; i++)
    {
      PopplerPage *page;
      gchar       *page_label;
      gdouble      page_width;
      gdouble      page_height;

      cairo_surface_t *surface;
      gint             width;
      gint             height;
      gint             page_index;

      page_index = base_index + sign * i;

      page = poppler_document_get_page (doc, pages->pages[page_index]);

      poppler_page_get_size (page, &page_width, &page_height);
      width  = (gint) ceil (page_width  * scale);
      height = (gint) ceil (page_height * scale);

      g_object_get (G_OBJECT (page), "label", &page_label, NULL);

      if (! image)
        {
          image = clay_image_new (doc_width, doc_height, CLAY_RGB);
          clay_image_undo_disable (image);

          clay_image_set_resolution (image, resolution, resolution);
        }
      doc_width  = MAX (doc_width, width);
      doc_height = MAX (doc_height, height);

      surface = render_page_to_surface (page, width, height, scale,
                                        antialias, white_background);

      layer_from_surface (image, page_label, 0, surface,
                          doc_progress, 1.0 / pages->n_pages);

      g_free (page_label);
      cairo_surface_destroy (surface);

      doc_progress = (double) (i + 1) / pages->n_pages;
      clay_progress_update (doc_progress);

      if (target == CLAY_PAGE_SELECTOR_TARGET_IMAGES)
        {
          images[i] = image;

          clay_image_undo_enable (image);
          clay_image_clean_all (image);

          image = 0;
        }
    }
  clay_progress_update (1.0);

  if (image)
    {
      /* If PDF contains both portrait and landscape pages,
       * then we need to resize the image to contain both */
      clay_image_resize (image, doc_width, doc_height, 0, 0);

      clay_image_undo_enable (image);
      clay_image_clean_all (image);
    }

  if (target == CLAY_PAGE_SELECTOR_TARGET_IMAGES)
    {
      if (run_mode != CLAY_RUN_NONINTERACTIVE)
        {
          /* Display images in reverse order.  The last will be
           * displayed by Clay itself
           */
          for (i = pages->n_pages - 1; i > 0; i--)
            clay_display_new (images[i]);
        }

      image = images[0];

      g_free (images);
    }

  return image;
}

static cairo_surface_t *
get_thumb_surface (PopplerDocument *doc,
                   gint             page_num,
                   gint             preferred_size,
                   gboolean         white_background)
{
  PopplerPage     *page;
  cairo_surface_t *surface;

  page = poppler_document_get_page (doc, page_num);

  if (! page)
    return NULL;

  surface = poppler_page_get_thumbnail (page);

  if (! surface)
    {
      gdouble width;
      gdouble height;
      gdouble scale;

      poppler_page_get_size (page, &width, &height);

      scale = (gdouble) preferred_size / MAX (width, height);

      width  *= scale;
      height *= scale;

      surface = render_page_to_surface (page, width, height, scale, TRUE, white_background);
    }

  g_object_unref (page);

  return surface;
}

static GdkPixbuf *
get_thumb_pixbuf (PopplerDocument *doc,
                  gint             page_num,
                  gint             preferred_size,
                  gboolean         white_background)
{
  cairo_surface_t *surface;
  GdkPixbuf       *pixbuf;

  surface = get_thumb_surface (doc, page_num, preferred_size, white_background);
  pixbuf  = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                         cairo_image_surface_get_width (surface),
                                         cairo_image_surface_get_height (surface));
  cairo_surface_destroy (surface);

  return pixbuf;
}

typedef struct
{
  PopplerDocument  *document;
  ClayPageSelector *selector;
  gboolean          white_background;

  GMutex            mutex;
  GCond             render_thumb;
  gboolean          stop_thumbnailing;
  gboolean          render_thumbnails;
} ThreadData;

typedef struct
{
  ClayPageSelector *selector;
  gint              page_no;
  GdkPixbuf        *pixbuf;
} IdleData;

static gboolean
idle_set_thumbnail (gpointer data)
{
  IdleData *idle_data = data;

  clay_page_selector_set_page_thumbnail (idle_data->selector,
                                         idle_data->page_no,
                                         idle_data->pixbuf);
  g_object_unref (idle_data->pixbuf);
  g_free (idle_data);

  return FALSE;
}

static gpointer
thumbnail_thread (gpointer data)
{
  ThreadData *thread_data = data;
  gboolean    first_loop  = TRUE;
  gint        n_pages;
  gint        i;

  n_pages = poppler_document_get_n_pages (thread_data->document);

  while (TRUE)
    {
      gboolean white_background;
      gboolean stop_thumbnailing;
      gboolean render_thumbnails;

      g_mutex_lock (&thread_data->mutex);
      if (first_loop)
        first_loop = FALSE;
      else
        g_cond_wait (&thread_data->render_thumb, &thread_data->mutex);

      stop_thumbnailing = thread_data->stop_thumbnailing;
      g_mutex_unlock (&thread_data->mutex);

      if (stop_thumbnailing)
        break;

      g_mutex_lock (&thread_data->mutex);
      render_thumbnails = thread_data->render_thumbnails;
      white_background  = thread_data->white_background;
      thread_data->render_thumbnails = FALSE;
      g_mutex_unlock (&thread_data->mutex);

      /* This handles "spurious wakeup", i.e. cases when g_cond_wait() returned
       * even though there was no call asking us to re-render the thumbnails.
       * See docs of g_cond_wait().
       */
      if (! render_thumbnails)
        continue;

      for (i = 0; i < n_pages; i++)
        {
          IdleData *idle_data = g_new0 (IdleData, 1);
          gboolean  white_background2;

          idle_data->selector = thread_data->selector;
          idle_data->page_no  = i;

          /* FIXME get preferred size from somewhere? */
          idle_data->pixbuf = get_thumb_pixbuf (thread_data->document, i,
                                                THUMBNAIL_SIZE,
                                                white_background);

          g_idle_add (idle_set_thumbnail, idle_data);

          g_mutex_lock (&thread_data->mutex);
          white_background2 = thread_data->white_background;
          stop_thumbnailing = thread_data->stop_thumbnailing;
          g_mutex_unlock (&thread_data->mutex);

          if (stop_thumbnailing || white_background2 != white_background)
            break;
        }

      if (stop_thumbnailing)
        break;
    }

  return NULL;
}

static void
white_background_toggled (GtkToggleButton *widget,
                          ThreadData      *thread_data)
{
  g_mutex_lock (&thread_data->mutex);
  thread_data->white_background  = gtk_toggle_button_get_active (widget);
  thread_data->render_thumbnails = TRUE;
  g_cond_signal (&thread_data->render_thumb);
  g_mutex_unlock (&thread_data->mutex);
}

static ClayPDBStatusType
load_dialog (PopplerDocument     *doc,
             PdfSelectedPages    *pages,
             ClayVectorLoadData   extracted_data,
             ClayProcedure       *procedure,
             ClayProcedureConfig *config)
{
  GtkWidget  *dialog;
  GtkWidget  *vbox;
  GtkWidget  *title;
  GtkWidget  *selector;
  GtkWidget  *white_bg;

  ThreadData  thread_data;
  GThread    *thread;

  gint        i;
  gint        n_pages;
  gchar      *title_text;

  gboolean    run;

  ClayPageSelectorTarget  target;
  gboolean                white_background;

  n_pages    = poppler_document_get_n_pages (doc);
  title_text = poppler_document_get_title (doc);

  dialog = clay_vector_load_procedure_dialog_new (CLAY_VECTOR_LOAD_PROCEDURE (procedure),
                                                  CLAY_PROCEDURE_CONFIG (config),
                                                  &extracted_data, NULL);

  g_object_get (config,
                "target",           &target,
                "white-background", &white_background,
                NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  /* Title */
  if (title_text && strlen (g_strstrip (title_text)) > 0)
    {
      title = gtk_label_new (title_text);
      gtk_label_set_ellipsize (GTK_LABEL (title), PANGO_ELLIPSIZE_END);
      gtk_box_pack_start (GTK_BOX (vbox), title, FALSE, FALSE, 0);
      gtk_widget_set_visible (title, TRUE);
    }
  if (title_text)
    g_free (title_text);

  /* Page Selector */
  selector = clay_page_selector_new ();
  gtk_widget_set_size_request (selector, 380, 300);
  gtk_box_pack_start (GTK_BOX (vbox), selector, TRUE, TRUE, 0);
  gtk_widget_set_visible (selector, TRUE);

  if (n_pages <= 0)
    {
      g_message (_("Error getting number of pages from the given PDF file."));
      return CLAY_PDB_EXECUTION_ERROR;
    }

  clay_page_selector_set_n_pages (CLAY_PAGE_SELECTOR (selector), n_pages);
  clay_page_selector_set_target (CLAY_PAGE_SELECTOR (selector),
                                 target);

  for (i = 0; i < n_pages; i++)
    {
      PopplerPage *page;
      gchar       *label;

      page = poppler_document_get_page (doc, i);
      g_object_get (G_OBJECT (page), "label", &label, NULL);

      clay_page_selector_set_page_label (CLAY_PAGE_SELECTOR (selector), i,
                                         label);

      g_object_unref (page);
      g_free (label);
    }
  /* Since selecting none will be equivalent to selecting all, this is
   * only useful as a feedback for the default behavior of selecting all
   * pages. */
  clay_page_selector_select_all (CLAY_PAGE_SELECTOR (selector));

  g_signal_connect_swapped (selector, "activate",
                            G_CALLBACK (gtk_window_activate_default),
                            dialog);

  thread_data.document          = doc;
  thread_data.selector          = CLAY_PAGE_SELECTOR (selector);
  thread_data.render_thumbnails = TRUE;
  thread_data.stop_thumbnailing = FALSE;
  thread_data.white_background  = white_background;
  g_mutex_init (&thread_data.mutex);
  g_cond_init (&thread_data.render_thumb);

  thread = g_thread_new ("thumbnailer", thumbnail_thread, &thread_data);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "reverse-order",
                              NULL);

  white_bg = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                               "white-background", G_TYPE_NONE);
  g_signal_connect (white_bg, "toggled",
                    G_CALLBACK (white_background_toggled), &thread_data);
  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "antialias",
                              "white-background",
                              NULL);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  target = clay_page_selector_get_target (CLAY_PAGE_SELECTOR (selector));
  g_object_set (config,
                "target", target,
                NULL);

  pages->pages =
    clay_page_selector_get_selected_pages (CLAY_PAGE_SELECTOR (selector),
                                           (gint *) &pages->n_pages);

  /* select all if none selected */
  if (pages->n_pages == 0)
    {
      clay_page_selector_select_all (CLAY_PAGE_SELECTOR (selector));

      pages->pages =
        clay_page_selector_get_selected_pages (CLAY_PAGE_SELECTOR (selector),
                                               (gint *) &pages->n_pages);
    }

  /* cleanup */
  g_mutex_lock (&thread_data.mutex);
  thread_data.stop_thumbnailing = TRUE;
  g_cond_signal (&thread_data.render_thumb);
  g_mutex_unlock (&thread_data.mutex);
  g_thread_join (thread);

  g_mutex_clear (&thread_data.mutex);
  g_cond_clear (&thread_data.render_thumb);

  /* XXX Unsure why, I get some CRITICALs when destroying the dialog, unless I
   * unselect all first.
   */
  clay_page_selector_unselect_all (CLAY_PAGE_SELECTOR (selector));
  gtk_widget_destroy (dialog);

  return run ? CLAY_PDB_SUCCESS : CLAY_PDB_CANCEL;
}
