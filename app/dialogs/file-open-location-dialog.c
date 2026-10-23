/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayprogress.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/claycontainerentry.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayprogressbox.h"
#include "widgets/claywidgets-utils.h"

#include "file-open-location-dialog.h"

#include "clay-intl.h"


static void      file_open_location_response   (GtkDialog          *dialog,
                                                gint                response_id,
                                                Clay               *clay);

static gboolean  file_open_location_completion (GtkEntryCompletion *completion,
                                                const gchar        *key,
                                                GtkTreeIter        *iter,
                                                gpointer            data);


/*  public functions  */

GtkWidget *
file_open_location_dialog_new (Clay *clay)
{
  ClayContext        *context;
  GtkWidget          *dialog;
  GtkWidget          *hbox;
  GtkWidget          *vbox;
  GtkWidget          *image;
  GtkWidget          *label;
  GtkWidget          *entry;
  GtkEntryCompletion *completion;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  dialog = clay_dialog_new (_("Open Location"),
                            "clay-file-open-location",
                            NULL, 0,
                            clay_standard_help_func,
                            CLAY_HELP_FILE_OPEN_LOCATION,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Open"),   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  clay_dialog_set_alternative_button_order (GTK_DIALOG(dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_location_response),
                    clay);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_WEB, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (_("Enter location (URI):"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  /* we don't want the context to affect the entry, so create
   * a scratch one instead of using e.g. the user context
   */
  context = clay_context_new (clay, "file-open-location-dialog", NULL);
  entry = clay_container_entry_new (clay->documents, context,
                                    CLAY_VIEW_SIZE_SMALL, 0);
  g_object_unref (context);

  completion = gtk_entry_get_completion (GTK_ENTRY (entry));
  gtk_entry_completion_set_match_func (completion,
                                       file_open_location_completion,
                                       NULL, NULL);

  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_widget_set_size_request (entry, 400, -1);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (entry, TRUE);

  g_object_set_data (G_OBJECT (dialog), "location-entry", entry);

  return dialog;
}


/*  private functions  */

static void
file_open_location_response (GtkDialog *dialog,
                             gint       response_id,
                             Clay      *clay)
{
  GtkWidget   *entry;
  GtkWidget   *box;
  const gchar *text = NULL;

  box = g_object_get_data (G_OBJECT (dialog), "progress-box");

  if (response_id != GTK_RESPONSE_OK)
    {
      if (box && CLAY_PROGRESS_BOX (box)->active)
        clay_progress_cancel (CLAY_PROGRESS (box));
      else
        gtk_widget_destroy (GTK_WIDGET (dialog));

      return;
    }

  entry = g_object_get_data (G_OBJECT (dialog), "location-entry");
  text = gtk_entry_get_text (GTK_ENTRY (entry));

  if (text && strlen (text))
    {
      ClayImage         *image;
      gchar             *filename;
      GFile             *file;
      ClayPDBStatusType  status;
      GError            *error = NULL;

      filename = g_filename_from_uri (text, NULL, NULL);

      if (filename)
        {
          file = g_file_new_for_uri (text);
          g_free (filename);
        }
      else
        {
          file = file_utils_filename_to_file (clay, text, &error);
        }

      if (!box)
        {
          box = clay_progress_box_new ();
          gtk_container_set_border_width (GTK_CONTAINER (box), 12);
          gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                            box, FALSE, FALSE, 0);

          g_object_set_data (G_OBJECT (dialog), "progress-box", box);
        }

      if (file)
        {
          gtk_widget_set_visible (box, TRUE);

          gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
          gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, FALSE);

          image = file_open_with_proc_and_display (clay,
                                                   clay_get_user_context (clay),
                                                   CLAY_PROGRESS (box),
                                                   file, FALSE, FALSE, NULL,
                                                   G_OBJECT (clay_widget_get_monitor (entry)),
                                                   &status, &error);

          gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, TRUE);
          gtk_editable_set_editable (GTK_EDITABLE (entry), TRUE);

          if (image == NULL && status != CLAY_PDB_CANCEL)
            {
              clay_message (clay, G_OBJECT (box), CLAY_MESSAGE_ERROR,
                            _("Opening '%s' failed:\n\n%s"),
                            clay_file_get_utf8_name (file), error->message);
              g_clear_error (&error);
            }

          g_object_unref (file);

          if (image != NULL)
            {
              gtk_widget_destroy (GTK_WIDGET (dialog));
              return;
            }
        }
      else
        {
          clay_message (clay, G_OBJECT (box), CLAY_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        text,
                        /* error should never be NULL, also issue #3093 */
                        error ? error->message : _("Invalid URI"));
          g_clear_error (&error);
        }
    }
}

static gboolean
file_open_location_completion (GtkEntryCompletion *completion,
                               const gchar        *key,
                               GtkTreeIter        *iter,
                               gpointer            data)
{
  GtkTreeModel *model = gtk_entry_completion_get_model (completion);
  gchar        *name;
  gchar        *normalized;
  gchar        *case_normalized;
  gboolean      match;

  gtk_tree_model_get (model, iter,
                      1, &name,
                      -1);

  if (! name)
    return FALSE;

  normalized = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
  case_normalized = g_utf8_casefold (normalized, -1);

  match = (strncmp (key, case_normalized, strlen (key)) == 0);

  if (! match)
    {
      const gchar *colon = strchr (case_normalized, ':');

      if (colon && strlen (colon) > 2 && colon[1] == '/' && colon[2] == '/')
        match = (strncmp (key, colon + 3, strlen (key)) == 0);
    }

  g_free (normalized);
  g_free (case_normalized);
  g_free (name);

  return match;
}
