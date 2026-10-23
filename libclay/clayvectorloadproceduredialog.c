/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayvectorloadproceduredialog.c
 * Copyright (C) 2024 Jehan
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"
#include "clayui.h"
#include "clayvectorloadproceduredialog.h"
#include "clayresolutionentry-private.h"

#include "libclay-intl.h"


struct _ClayVectorLoadProcedureDialog
{
  ClayProcedureDialog  parent_instance;

  GFile               *file;
  ClayVectorLoadData  *extracted_data;
};


static void clay_vector_load_procedure_dialog_fill_start       (ClayProcedureDialog           *dialog,
                                                                ClayProcedure                 *procedure,
                                                                ClayProcedureConfig           *config);
static void clay_vector_load_procedure_dialog_fill_list        (ClayProcedureDialog           *dialog,
                                                                ClayProcedure                 *procedure,
                                                                ClayProcedureConfig           *config,
                                                                GList                         *properties);

static void clay_vector_load_procedure_dialog_preview_allocate (GtkWidget                     *gtk_image,
                                                                GtkAllocation                 *allocation,
                                                                ClayVectorLoadProcedureDialog *dialog);


G_DEFINE_TYPE (ClayVectorLoadProcedureDialog, clay_vector_load_procedure_dialog, CLAY_TYPE_PROCEDURE_DIALOG)

#define parent_class clay_vector_load_procedure_dialog_parent_class

static void
clay_vector_load_procedure_dialog_class_init (ClayVectorLoadProcedureDialogClass *klass)
{
  ClayProcedureDialogClass *proc_dialog_class = CLAY_PROCEDURE_DIALOG_CLASS (klass);

  proc_dialog_class->fill_start = clay_vector_load_procedure_dialog_fill_start;
  proc_dialog_class->fill_list  = clay_vector_load_procedure_dialog_fill_list;
}

static void
clay_vector_load_procedure_dialog_init (ClayVectorLoadProcedureDialog *dialog)
{
  dialog->file = NULL;
}

static void
clay_vector_load_procedure_dialog_fill_start (ClayProcedureDialog *dialog,
                                              ClayProcedure       *procedure,
                                              ClayProcedureConfig *config)
{
  ClayVectorLoadProcedureDialog *vector_dialog = CLAY_VECTOR_LOAD_PROCEDURE_DIALOG (dialog);
  GtkWidget                     *content_area;
  GtkWidget                     *top_hbox;
  GtkWidget                     *main_vbox;
  GtkWidget                     *res_entry;
  GtkWidget                     *label;
  gchar                         *text = NULL;
  gchar                         *markup = NULL;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (content_area), main_vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (main_vbox, TRUE);

  top_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), top_hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (top_hbox, TRUE);

  /* Resolution */
  res_entry = clay_prop_resolution_entry_new (G_OBJECT (config),
                                              "width", "height", "pixel-density",
                                              "physical-unit");

  gtk_box_pack_start (GTK_BOX (top_hbox), res_entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (res_entry, TRUE);

  /* Preview */

  if (vector_dialog->file)
    {
      GtkWidget *image;

      image = gtk_image_new ();
      g_signal_connect (image, "size-allocate",
                        G_CALLBACK (clay_vector_load_procedure_dialog_preview_allocate),
                        dialog);

      gtk_box_pack_start (GTK_BOX (top_hbox), image, FALSE, FALSE, 0);
      gtk_widget_set_visible (image, TRUE);
    }

  if (vector_dialog->extracted_data)
    {
      if (vector_dialog->extracted_data->exact_width && vector_dialog->extracted_data->exact_height)
        {
          /* TRANSLATORS: the %s is a vector format name, e.g. "SVG" or "PDF",
           * followed by 2D dimensions with unit, e.g. "200 inch x 400 inch"
           */
          text = g_strdup_printf (_("Source %s file size: %%.%df %s × %%.%df %s"),
                                  clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure)),
                                  clay_unit_get_digits (vector_dialog->extracted_data->width_unit),
                                  clay_unit_get_abbreviation (vector_dialog->extracted_data->width_unit),
                                  clay_unit_get_digits (vector_dialog->extracted_data->height_unit),
                                  clay_unit_get_abbreviation (vector_dialog->extracted_data->height_unit));
          markup = g_strdup_printf (text, vector_dialog->extracted_data->width, vector_dialog->extracted_data->height);
        }
      else if (vector_dialog->extracted_data->correct_ratio)
        {
          gdouble ratio_width         = 0.0;
          gint    ratio_width_digits  = 0;
          gdouble ratio_height        = 0.0;
          gint    ratio_height_digits = 0;

          if (vector_dialog->extracted_data->width_unit == vector_dialog->extracted_data->height_unit)
            {
              ratio_width  = vector_dialog->extracted_data->width;
              ratio_height = vector_dialog->extracted_data->height;
              if (vector_dialog->extracted_data->width_unit == clay_unit_pixel () ||
                  vector_dialog->extracted_data->width_unit == clay_unit_percent ())
                ratio_width_digits = ratio_height_digits = 0;
              else
                ratio_width_digits = ratio_height_digits = clay_unit_get_digits (vector_dialog->extracted_data->width_unit);
            }
          else if (vector_dialog->extracted_data->width_unit != clay_unit_pixel () && vector_dialog->extracted_data->height_unit != clay_unit_pixel () &&
                   vector_dialog->extracted_data->width_unit != clay_unit_percent () && vector_dialog->extracted_data->height_unit != clay_unit_percent ())
            {
              ratio_width = vector_dialog->extracted_data->width / clay_unit_get_factor (vector_dialog->extracted_data->width_unit);
              ratio_height = vector_dialog->extracted_data->height / clay_unit_get_factor (vector_dialog->extracted_data->height_unit);

              ratio_width_digits = ratio_height_digits = clay_unit_get_digits (clay_unit_inch ());
            }

          if (ratio_width != 0.0 && ratio_height != 0.0)
            {
              /* TRANSLATOR: the %s is a vector format name, e.g. "SVG" or "PDF". */
              text = g_strdup_printf (_("Source %s file's aspect ratio: %%.%df × %%.%df"),
                                      clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure)),
                                      ratio_width_digits, ratio_height_digits);
              markup = g_strdup_printf (text, ratio_width, ratio_height);
            }
        }
      else if (vector_dialog->extracted_data->width != 0.0 && vector_dialog->extracted_data->height != 0.0)
        {
          text = g_strdup_printf (_("Approximated source %s file size: %%.%df %s × %%.%df %s"),
                                  clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure)),
                                  clay_unit_get_digits (vector_dialog->extracted_data->width_unit),
                                  clay_unit_get_abbreviation (vector_dialog->extracted_data->width_unit),
                                  clay_unit_get_digits (vector_dialog->extracted_data->height_unit),
                                  clay_unit_get_abbreviation (vector_dialog->extracted_data->height_unit));
          markup = g_strdup_printf (text, vector_dialog->extracted_data->width, vector_dialog->extracted_data->height);
        }
    }

  if (markup == NULL)
    {
      /* TRANSLATOR: the %s is a vector format name, e.g. "SVG" or "PDF". */
      text = g_strdup_printf (_("The source %s file does not specify a size!"),
                              clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure)));
      markup = g_strdup_printf ("<i>%s</i>", text);
    }

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, TRUE, TRUE, 4);
  gtk_widget_set_visible (label, TRUE);

  g_free (text);
  g_free (markup);

  CLAY_PROCEDURE_DIALOG_CLASS (parent_class)->fill_start (dialog, procedure, config);
}

static void
clay_vector_load_procedure_dialog_fill_list (ClayProcedureDialog *dialog,
                                             ClayProcedure       *procedure,
                                             ClayProcedureConfig *config,
                                             GList               *properties)
{
  GList *properties2 = NULL;
  GList *iter;

  for (iter = properties; iter; iter = iter->next)
    {
      gchar *propname = iter->data;

      if (g_strcmp0 (propname, "width") == 0                    ||
          g_strcmp0 (propname, "height") == 0                   ||
          g_strcmp0 (propname, "keep-ratio") == 0               ||
          g_strcmp0 (propname, "prefer-native-dimensions") == 0 ||
          g_strcmp0 (propname, "pixel-density") == 0            ||
          g_strcmp0 (propname, "physical-unit") == 0)
        /* Ignoring the standards args which are being handled by fill_start(). */
        continue;

      properties2 = g_list_prepend (properties2, propname);
    }
  properties2 = g_list_reverse (properties2);
  CLAY_PROCEDURE_DIALOG_CLASS (parent_class)->fill_list (dialog, procedure, config, properties2);
  g_list_free (properties2);
}

static void
clay_vector_load_procedure_dialog_preview_allocate (GtkWidget                     *gtk_image,
                                                    GtkAllocation                 *allocation,
                                                    ClayVectorLoadProcedureDialog *dialog)
{
  if (dialog->file)
    {
      ClayProcedure     *procedure = NULL;
      ClayValueArray    *retval;
      ClayPDBStatusType  status;

      g_object_get (dialog, "procedure", &procedure, NULL);

      retval = clay_procedure_run (procedure,
                                   "file",       dialog->file,
                                   "width",      allocation->height,
                                   "height",     allocation->height,
                                   "keep-ratio", TRUE,
                                   NULL);

      status = g_value_get_enum (clay_value_array_index (retval, 0));

      if (status == CLAY_PDB_SUCCESS)
        {
          ClayImage *image;
          GdkPixbuf *preview;

          image = g_value_get_object (clay_value_array_index (retval, 1));
          preview = clay_image_get_thumbnail (image,
                                              clay_image_get_width (image),
                                              clay_image_get_height (image),
                                              CLAY_PIXBUF_SMALL_CHECKS);
          gtk_image_set_from_pixbuf (GTK_IMAGE (gtk_image), preview);
          clay_image_delete (image);
        }

      clay_value_array_unref (retval);
    }
}


/* Public Functions */


/**
 * clay_vector_load_procedure_dialog_new:
 * @procedure: the associated #ClayVectorLoadProcedure.
 * @config:    a #ClayProcedureConfig from which properties will be
 *             turned into widgets.
 * @extracted_data: (nullable): the extracted dimensions of the file to load.
 * @file: (nullable): a [iface@Gio.File] to load the preview from.
 *
 * Creates a new dialog for @procedure using widgets generated from
 * properties of @config.
 *
 * @file must be the same vector file which was passed to the
 * [callback@Clay.RunVectorLoadFunc] implementation for your plug-in. If you pass any
 * other file, then the preview may be wrong or not showing at all. And it is
 * considered a programming error.
 *
 * As for all #GtkWindow, the returned #ClayProcedureDialog object is
 * owned by GTK and its initial reference is stored in an internal list
 * of top-level windows. To delete the dialog, call
 * gtk_widget_destroy().
 *
 * Returns: (transfer none): the newly created #ClayVectorLoadProcedureDialog.
 */
GtkWidget *
clay_vector_load_procedure_dialog_new (ClayVectorLoadProcedure *procedure,
                                       ClayProcedureConfig     *config,
                                       ClayVectorLoadData      *extracted_data,
                                       GFile                   *file)
{
  GtkWidget   *dialog;
  gchar       *title;
  const gchar *format_name;
  const gchar *help_id;
  gboolean     use_header_bar;

  g_return_val_if_fail (CLAY_IS_VECTOR_LOAD_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (clay_procedure_config_get_procedure (config) ==
                        CLAY_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  format_name = clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure));
  if (! format_name)
    {
      g_critical ("%s: no format name set on file procedure '%s'. "
                  "Set one with clay_file_procedure_set_format_name()",
                  G_STRFUNC,
                  clay_procedure_get_name (CLAY_PROCEDURE (procedure)));
      return NULL;
    }
  /* TRANSLATORS: %s will be a format name, e.g. "PNG" or "JPEG". */
  title = g_strdup_printf (_("Load %s Image"), format_name);

  help_id = clay_procedure_get_help_id (CLAY_PROCEDURE (procedure));

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (CLAY_TYPE_VECTOR_LOAD_PROCEDURE_DIALOG,
                         "procedure",      procedure,
                         "config",         config,
                         "title",          title,
                         "help-func",      clay_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);
  CLAY_VECTOR_LOAD_PROCEDURE_DIALOG (dialog)->file           = file;
  CLAY_VECTOR_LOAD_PROCEDURE_DIALOG (dialog)->extracted_data = extracted_data;
  g_free (title);

  return dialog;
}
