/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayexportproceduredialog.c
 * Copyright (C) 2020 Jehan
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

#include "libclay-intl.h"


struct _ClayExportProcedureDialog
{
  ClayProcedureDialog  parent_instance;

  GList               *additional_metadata;
  ClayImage           *image;

  GThread             *metadata_thread;
  gboolean             metadata_thread_running;
  GMutex               metadata_thread_mutex;
};


static void   clay_export_procedure_dialog_finalize  (GObject             *object);

static void   clay_export_procedure_dialog_fill_end  (ClayProcedureDialog *dialog,
                                                      ClayProcedure       *procedure,
                                                      ClayProcedureConfig *config);
static void   clay_export_procedure_dialog_fill_list (ClayProcedureDialog *dialog,
                                                      ClayProcedure       *procedure,
                                                      ClayProcedureConfig *config,
                                                      GList               *properties);

static void   clay_export_procedure_dialog_notify_comment
                                                     (ClayProcedureConfig *config,
                                                      GParamSpec          *pspec,
                                                      GtkWidget           *widget);

static gpointer clay_export_procedure_dialog_edit_metadata_thread   (gpointer                 data);
static gboolean clay_export_procedure_dialog_activate_edit_metadata (GtkLinkButton           *link,
                                                                     ClayExportProcedureDialog *dialog);


G_DEFINE_TYPE (ClayExportProcedureDialog, clay_export_procedure_dialog, CLAY_TYPE_PROCEDURE_DIALOG)

#define parent_class clay_export_procedure_dialog_parent_class

static void
clay_export_procedure_dialog_class_init (ClayExportProcedureDialogClass *klass)
{
  GObjectClass             *object_class;
  ClayProcedureDialogClass *proc_dialog_class;

  object_class      = G_OBJECT_CLASS (klass);
  proc_dialog_class = CLAY_PROCEDURE_DIALOG_CLASS (klass);

  object_class->finalize       = clay_export_procedure_dialog_finalize;
  proc_dialog_class->fill_end  = clay_export_procedure_dialog_fill_end;
  proc_dialog_class->fill_list = clay_export_procedure_dialog_fill_list;
}

static void
clay_export_procedure_dialog_init (ClayExportProcedureDialog *dialog)
{
  dialog->additional_metadata     = NULL;
  dialog->image                   = NULL;
  dialog->metadata_thread         = NULL;
  dialog->metadata_thread_running = FALSE;
  g_mutex_init (&dialog->metadata_thread_mutex);
}

static void
clay_export_procedure_dialog_finalize (GObject *object)
{
  ClayExportProcedureDialog *dialog = CLAY_EXPORT_PROCEDURE_DIALOG (object);

  g_list_free_full (dialog->additional_metadata, g_free);
  g_mutex_lock (&dialog->metadata_thread_mutex);
  if (dialog->metadata_thread_running)
    {
      g_mutex_unlock (&dialog->metadata_thread_mutex);
      g_thread_join (dialog->metadata_thread);
      g_mutex_lock (&dialog->metadata_thread_mutex);
    }
  g_mutex_unlock (&dialog->metadata_thread_mutex);
  g_mutex_clear (&dialog->metadata_thread_mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_export_procedure_dialog_fill_end (ClayProcedureDialog *dialog,
                                       ClayProcedure       *procedure,
                                       ClayProcedureConfig *config)
{
  ClayExportProcedureDialog *export_dialog;
  ClayExportProcedure       *export_procedure;
  GtkWidget                 *content_area;
  ClayExportCapabilities     capabilities = 0;

  CLAY_PROCEDURE_DIALOG_CLASS (parent_class)->fill_end (dialog, procedure, config);

  export_dialog    = CLAY_EXPORT_PROCEDURE_DIALOG (dialog);
  export_procedure = CLAY_EXPORT_PROCEDURE (procedure);
  content_area     = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  if (clay_export_procedure_get_support_exif      (export_procedure) ||
      clay_export_procedure_get_support_iptc      (export_procedure) ||
      clay_export_procedure_get_support_xmp       (export_procedure) ||
      clay_export_procedure_get_support_profile   (export_procedure) ||
      clay_export_procedure_get_support_thumbnail (export_procedure) ||
      g_list_length (export_dialog->additional_metadata) > 0         ||
      clay_export_procedure_get_support_comment   (export_procedure))
    {
      GtkWidget      *frame;
      GtkWidget      *frame_title;
      GtkWidget      *grid;
      GtkWidget      *widget;
      GtkWidget      *label;
      GtkWidget      *link;
      PangoAttrList  *attrs;
      PangoAttribute *attr;
      gint            n_metadata;
      gint            left = 0;
      gint            top  = 0;

      frame = clay_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

      /* Metadata frame title: a label and an edit link. */
      frame_title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

      label = gtk_label_new (_("Metadata"));
      attrs = pango_attr_list_new ();
      attr  = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
      pango_attr_list_insert (attrs, attr);
      gtk_label_set_attributes (GTK_LABEL (label), attrs);
      pango_attr_list_unref (attrs);
      gtk_box_pack_start (GTK_BOX (frame_title), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);

      link = gtk_link_button_new_with_label (_("Edit Metadata"), _("(edit)"));
      gtk_link_button_set_visited (GTK_LINK_BUTTON (link), FALSE);
      g_signal_connect (link, "activate-link",
                        G_CALLBACK (clay_export_procedure_dialog_activate_edit_metadata),
                        dialog);
      gtk_box_pack_start (GTK_BOX (frame_title), link, FALSE, FALSE, 0);
      gtk_widget_set_visible (link, TRUE);

      gtk_frame_set_label_widget (GTK_FRAME (frame), frame_title);
      gtk_widget_set_visible (frame_title, TRUE);

      /* Metadata frame contents in a grid.. */
      grid = gtk_grid_new ();
      gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
      gtk_widget_set_vexpand (grid, TRUE);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_set_visible (grid, TRUE);

      /* Line for 3 metadata formats: Exif, IPTC, XMP. */
      n_metadata = clay_export_procedure_get_support_exif (export_procedure) +
                   clay_export_procedure_get_support_iptc (export_procedure) +
                   clay_export_procedure_get_support_xmp  (export_procedure);
      n_metadata = MAX (n_metadata,
                        clay_export_procedure_get_support_profile (export_procedure) +
                        clay_export_procedure_get_support_thumbnail (export_procedure));

      if (clay_export_procedure_get_support_exif (export_procedure))
        {
          widget = clay_prop_check_button_new (G_OBJECT (config),
                                               "include-exif", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_set_visible (widget, TRUE);
        }
      if (clay_export_procedure_get_support_iptc (export_procedure))
        {
          widget = clay_prop_check_button_new (G_OBJECT (config),
                                               "include-iptc", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_set_visible (widget, TRUE);
        }
      if (clay_export_procedure_get_support_xmp (export_procedure))
        {
          widget = clay_prop_check_button_new (G_OBJECT (config),
                                               "include-xmp", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, 0, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          top   = 1;
          gtk_widget_set_visible (widget, TRUE);
        }

      /* Line for specific metadata: profile, thumbnail. */
      left = 0;

      if (clay_export_procedure_get_support_profile (export_procedure))
        {
          widget = clay_prop_check_button_new (G_OBJECT (config),
                                               "include-color-profile", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          gtk_widget_set_visible (widget, TRUE);
        }
      if (clay_export_procedure_get_support_thumbnail (export_procedure))
        {
          widget = clay_prop_check_button_new (G_OBJECT (config),
                                               "include-thumbnail", NULL);
          gtk_grid_attach (GTK_GRID (grid), widget,
                           left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          gtk_widget_set_visible (widget, TRUE);
        }
      if (n_metadata > 0)
        top++;

      /* Custom metadata: n_metadata items per line. */
      left = 0;
      for (GList *iter = export_dialog->additional_metadata; iter; iter = iter->next)
        {
          widget = clay_procedure_dialog_get_widget (dialog, iter->data, G_TYPE_NONE);
          gtk_grid_attach (GTK_GRID (grid), widget, left, top, 6 / n_metadata, 1);
          left += 6 / n_metadata;
          if (left >= 6)
            {
              top++;
              left = 0;
            }

          gtk_widget_set_visible (widget, TRUE);
        }
      top++;

      /* Last line for comment field. */
      if (clay_export_procedure_get_support_comment (export_procedure))
        {
          GtkTextBuffer *buffer;
          const gchar   *tooltip;
          GtkWidget     *frame2;
          GtkWidget     *title;
          GParamSpec    *pspec;
          GtkWidget     *scrolled_window;

          pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                "clay-comment");

          frame2 = clay_frame_new (NULL);
          title  = clay_prop_check_button_new (G_OBJECT (config),
                                               "include-comment", NULL);
          gtk_frame_set_label_widget (GTK_FRAME (frame2), title);
          gtk_widget_set_visible (title, TRUE);

          buffer = clay_prop_text_buffer_new (G_OBJECT (config),
                                              "clay-comment", -1);
          widget = gtk_text_view_new_with_buffer (buffer);
          gtk_text_view_set_top_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_left_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_right_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD_CHAR);
          g_object_unref (buffer);

          tooltip = g_param_spec_get_blurb (pspec);
          if (tooltip)
            clay_help_set_help_data (widget, tooltip, NULL);

          scrolled_window = gtk_scrolled_window_new (NULL, NULL);
          gtk_widget_set_size_request (scrolled_window, -1, 100);
          gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_OUT);
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                          GTK_POLICY_EXTERNAL,
                                          GTK_POLICY_AUTOMATIC);
          gtk_container_add (GTK_CONTAINER (frame2), scrolled_window);
          gtk_widget_set_visible (scrolled_window, TRUE);
          gtk_widget_set_hexpand (widget, TRUE);
          gtk_widget_set_vexpand (widget, TRUE);
          gtk_container_add (GTK_CONTAINER (scrolled_window), widget);
          gtk_widget_set_visible (widget, TRUE);

          /* Comment field should only be editable when "Save Comment" is
           * sensitive. */
          g_signal_connect_object (config, "notify::include-comment",
                                   G_CALLBACK (clay_export_procedure_dialog_notify_comment),
                                   widget, 0);
          clay_export_procedure_dialog_notify_comment (config, NULL, widget);

          gtk_grid_attach (GTK_GRID (grid), frame2, 0, top, 6, 1);
          gtk_widget_set_visible (frame2, TRUE);
        }

      gtk_box_pack_start (GTK_BOX (content_area), frame, TRUE, TRUE, 0);
      gtk_widget_set_visible (frame, TRUE);
    }

  g_object_get (procedure, "capabilities", &capabilities, NULL);
  if (capabilities != 0                                  &&
      (capabilities & CLAY_EXPORT_CAN_HANDLE_ALPHA) == 0 &&
      clay_image_has_transparency (export_dialog->image))
    {
      const gchar *file_type;
      const gchar *title = _("Transparency will be lost");
      gchar       *details;


      file_type = clay_file_procedure_get_format_name (CLAY_FILE_PROCEDURE (procedure));
      details   = g_strdup_printf (_("This format does not support transparency: %s"),
                                   file_type);

      clay_procedure_dialog_set_warning (dialog, title, details, FALSE);

      g_free (details);
    }
}

static void
clay_export_procedure_dialog_fill_list (ClayProcedureDialog *dialog,
                                        ClayProcedure       *procedure,
                                        ClayProcedureConfig *config,
                                        GList               *properties)
{
  ClayExportProcedureDialog *export_dialog;
  ClayExportProcedure       *export_procedure;
  GList                     *properties2 = NULL;
  GList                     *iter;

  export_dialog    = CLAY_EXPORT_PROCEDURE_DIALOG (dialog);
  export_procedure = CLAY_EXPORT_PROCEDURE (procedure);

  for (iter = properties; iter; iter = iter->next)
    {
      gchar *propname = iter->data;

      if ((clay_export_procedure_get_support_exif (export_procedure) &&
           g_strcmp0 (propname, "include-exif") == 0)                 ||
          (clay_export_procedure_get_support_iptc (export_procedure) &&
           g_strcmp0 (propname, "include-iptc") == 0)                 ||
          (clay_export_procedure_get_support_xmp (export_procedure) &&
           g_strcmp0 (propname, "include-xmp") == 0)                  ||
          (clay_export_procedure_get_support_profile (export_procedure) &&
           g_strcmp0 (propname, "include-color-profile") == 0)        ||
          (clay_export_procedure_get_support_thumbnail (export_procedure) &&
           g_strcmp0 (propname, "include-thumbnail") == 0)            ||
          (clay_export_procedure_get_support_comment (export_procedure) &&
           (g_strcmp0 (propname, "include-comment") == 0 ||
            g_strcmp0 (propname, "clay-comment") == 0))            ||
          g_list_find (export_dialog->additional_metadata, propname))
        /* Ignoring the standards and custom metadata. */
        continue;

      properties2 = g_list_prepend (properties2, propname);
    }
  properties2 = g_list_reverse (properties2);
  CLAY_PROCEDURE_DIALOG_CLASS (parent_class)->fill_list (dialog, procedure, config, properties2);
  g_list_free (properties2);
}

static void
clay_export_procedure_dialog_notify_comment (ClayProcedureConfig *config,
                                             GParamSpec          *pspec,
                                             GtkWidget           *widget)
{
  gboolean sensitive;

  g_object_get (config, "include-comment", &sensitive, NULL);

  gtk_widget_set_sensitive (widget, sensitive);
}

static gpointer
clay_export_procedure_dialog_edit_metadata_thread (gpointer data)
{
  ClayExportProcedureDialog *dialog = data;
  ClayProcedure             *procedure;
  GBytes                    *dialog_handle;

  g_mutex_lock (&dialog->metadata_thread_mutex);
  dialog->metadata_thread_running = TRUE;
  g_mutex_unlock (&dialog->metadata_thread_mutex);

  procedure     = clay_pdb_lookup_procedure (clay_get_pdb (), "plug-in-metadata-editor");
  dialog_handle = clay_dialog_get_native_handle (CLAY_DIALOG (dialog));
  clay_procedure_run (procedure,
                      "run-mode",      CLAY_RUN_INTERACTIVE,
                      "image",         dialog->image,
                      "parent-handle", dialog_handle,
                      NULL);

  g_mutex_lock (&dialog->metadata_thread_mutex);
  dialog->metadata_thread_running = FALSE;
  g_mutex_unlock (&dialog->metadata_thread_mutex);

  return NULL;
}

static gboolean
clay_export_procedure_dialog_activate_edit_metadata (GtkLinkButton             *link,
                                                     ClayExportProcedureDialog *dialog)
{
  gtk_link_button_set_visited (link, TRUE);

  g_mutex_lock (&dialog->metadata_thread_mutex);

  /* Only run if not already running. */
  if (! dialog->metadata_thread_running)
    {
      if (dialog->metadata_thread)
        g_thread_join (dialog->metadata_thread);

      dialog->metadata_thread = g_thread_try_new ("Edit Metadata",
                                                  clay_export_procedure_dialog_edit_metadata_thread,
                                                  dialog, NULL);
    }

  g_mutex_unlock (&dialog->metadata_thread_mutex);

  /* Stop propagation as the URI is bogus. */
  return TRUE;
}


/* Public Functions */


/**
 * clay_export_procedure_dialog_new:
 * @procedure: the export procedure.
 * @config:    the @procedure's config object.
 * @image:     the image which you are planning to export.
 *
 * This will create a dialog for an export procedure. You should further
 * fill it, using the [class@ClayUi.ProcedureDialog] API, with the
 * arguments of @procedure as GUI settings.
 *
 * The @image can either be your source image or the one as created by
 * [method@Clay.ExportOptions.get_image]. Note though though if @image is
 * not the source image, the dialog may not be able to automatically
 * print some relevant warnings, such as the fact that the transparency
 * will be lost when @procedure did not register the capability
 * [flags@Clay.ExportCapabilities.CAN_HANDLE_ALPHA].
 *
 * On the other end, if @image is the source image, then the metadate
 * "(edit)" link will modify metadata of the source image.
 *
 * Returns: (type ClayExportProcedureDialog): the new export dialog.
 */
GtkWidget *
clay_export_procedure_dialog_new (ClayExportProcedure *procedure,
                                  ClayProcedureConfig *config,
                                  ClayImage           *image)
{
  GtkWidget   *dialog;
  gchar       *title;
  const gchar *format_name;
  const gchar *help_id;
  gboolean     use_header_bar;

  g_return_val_if_fail (CLAY_IS_EXPORT_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (CLAY_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (clay_procedure_config_get_procedure (config) ==
                        CLAY_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

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
  title = g_strdup_printf (_("Export Image as %s"), format_name);

  help_id = clay_procedure_get_help_id (CLAY_PROCEDURE (procedure));

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (CLAY_TYPE_EXPORT_PROCEDURE_DIALOG,
                         "procedure",      procedure,
                         "config",         config,
                         "title",          title,
                         "help-func",      clay_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);
  CLAY_EXPORT_PROCEDURE_DIALOG (dialog)->image = image;
  g_free (title);

  return dialog;
}

void
clay_export_procedure_dialog_add_metadata (ClayExportProcedureDialog *dialog,
                                           const gchar               *property)
{
  if (! g_list_find (dialog->additional_metadata, property))
    dialog->additional_metadata = g_list_append (dialog->additional_metadata, g_strdup (property));
}
