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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"

#include "widgets/clayhelp-ids.h"

#include "tips-dialog.h"
#include "tips-parser.h"

#include "clay-intl.h"

enum
{
  RESPONSE_PREVIOUS = 1,
  RESPONSE_NEXT     = 2
};

static void     tips_dialog_set_tip  (ClayTip       *tip);
static void     tips_dialog_response (GtkWidget     *dialog,
                                      gint           response);
static void     tips_dialog_destroy  (GtkWidget     *widget,
                                      ClayGuiConfig *config);
static gboolean more_button_clicked  (GtkWidget     *button,
                                      Clay          *clay);


static GtkWidget *tips_dialog = NULL;
static GtkWidget *tip_label   = NULL;
static GtkWidget *more_button = NULL;
static GList     *tips        = NULL;
static GList     *current_tip = NULL;


GtkWidget *
tips_dialog_create (Clay *clay)
{
  ClayGuiConfig *config;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *button;
  GtkWidget     *image;
  gint           tips_count;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (!tips)
    {
      GError *error = NULL;
      GFile  *file;

      file = clay_data_directory_file ("tips", "clay-tips.xml", NULL);

      tips = clay_tips_from_file (file, &error);

      if (! tips)
        {
          ClayTip *tip;

          if (! error)
            {
              tip = clay_tip_new (_("The Clay tips file is empty!"), NULL);
            }
          else if (error->code == G_FILE_ERROR_NOENT)
            {
              tip = clay_tip_new (_("The Clay tips file appears to be "
                                    "missing!"),
                                  _("There should be a file called '%s'. "
                                    "Please check your installation."),
                                  clay_file_get_utf8_name (file));
            }
          else
            {
              tip = clay_tip_new (_("The Clay tips file could not be parsed!"),
                                  "%s", error->message);
            }

          tips = g_list_prepend (tips, tip);
        }
      else if (error)
        {
          g_printerr ("Error while parsing '%s': %s\n",
                      clay_file_get_utf8_name (file), error->message);
        }

      g_clear_error (&error);
      g_object_unref (file);
    }

  tips_count = g_list_length (tips);

  config = CLAY_GUI_CONFIG (clay->config);

  if (config->last_tip_shown >= tips_count || config->last_tip_shown < 0)
    config->last_tip_shown = 0;

  current_tip = g_list_nth (tips, config->last_tip_shown);

  if (tips_dialog)
    return tips_dialog;

  tips_dialog = clay_dialog_new (_("Clay Tip of the Day"),
                                 "clay-tip-of-the-day",
                                 NULL, 0, clay_standard_help_func,
                                 CLAY_HELP_TIPS_DIALOG,
                                 NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (tips_dialog),
                                  _("_Previous Tip"), RESPONSE_PREVIOUS);
  image = gtk_image_new_from_icon_name (CLAY_ICON_GO_PREVIOUS,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_widget_set_visible (image, TRUE);

  button = gtk_dialog_add_button (GTK_DIALOG (tips_dialog),
                                  _("_Next Tip"), RESPONSE_NEXT);
  image = gtk_image_new_from_icon_name (CLAY_ICON_GO_NEXT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_widget_set_visible (image, TRUE);
  gtk_dialog_set_default_response (GTK_DIALOG (tips_dialog), RESPONSE_NEXT);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (tips_dialog),
                                     RESPONSE_NEXT, tips_count > 1);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (tips_dialog),
                                     RESPONSE_PREVIOUS, tips_count > 1);

  g_signal_connect (tips_dialog, "response",
                    G_CALLBACK (tips_dialog_response),
                    NULL);
  g_signal_connect (tips_dialog, "destroy",
                    G_CALLBACK (tips_dialog_destroy),
                    config);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (tips_dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_DIALOG_INFORMATION,
                                        GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  tip_label = gtk_label_new (NULL);
  gtk_label_set_max_width_chars (GTK_LABEL (tip_label), 70);
  gtk_label_set_selectable (GTK_LABEL (tip_label), TRUE);
  gtk_label_set_justify (GTK_LABEL (tip_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (tip_label), TRUE);
  gtk_label_set_yalign (GTK_LABEL (tip_label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), tip_label, TRUE, TRUE, 0);
  gtk_widget_set_visible (tip_label, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  more_button = gtk_link_button_new_with_label ("https://docs.gimp.org/",
  /*  a link to the related section in the user manual  */
                                                _("Learn more"));
  gtk_widget_set_visible (more_button, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), more_button, FALSE, FALSE, 0);

  g_signal_connect (more_button, "activate-link",
                    G_CALLBACK (more_button_clicked),
                    clay);

  tips_dialog_set_tip (current_tip->data);

  return tips_dialog;
}

static void
tips_dialog_destroy (GtkWidget     *widget,
                     ClayGuiConfig *config)
{
  /* the last-shown-tip is saved in sessionrc */
  config->last_tip_shown = g_list_position (tips, current_tip);

  tips_dialog = NULL;
  current_tip = NULL;

  clay_tips_free (tips);
  tips = NULL;
}

static void
tips_dialog_response (GtkWidget *dialog,
                      gint       response)
{
  switch (response)
    {
    case RESPONSE_PREVIOUS:
      current_tip = current_tip->prev ? current_tip->prev : g_list_last (tips);
      tips_dialog_set_tip (current_tip->data);
      break;

    case RESPONSE_NEXT:
      current_tip = current_tip->next ? current_tip->next : tips;
      tips_dialog_set_tip (current_tip->data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
tips_dialog_set_tip (ClayTip *tip)
{
  g_return_if_fail (tip != NULL);

  gtk_label_set_markup (GTK_LABEL (tip_label), tip->text);

  /*  set the URI to unset the "visited" state  */
  gtk_link_button_set_uri (GTK_LINK_BUTTON (more_button),
                           "https://docs.gimp.org/");

  gtk_widget_set_sensitive (more_button, tip->help_id != NULL);
}

static gboolean
more_button_clicked (GtkWidget *button,
                     Clay      *clay)
{
  ClayTip *tip = current_tip->data;

  if (tip->help_id)
    clay_help (clay, NULL, NULL, tip->help_id);

  /* Do not run the link set at construction. */
  return TRUE;
}
