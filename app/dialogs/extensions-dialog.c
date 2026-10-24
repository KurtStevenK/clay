/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * extension-dialog.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include <cairo-gobject.h>
#include <gegl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "dialogs-types.h"

#include "core/clay.h"
#include "core/clayextensionmanager.h"
#include "core/clayextension.h"

#include "pdb/clayprocedure.h"
#include "file-data/file-data-gex.h"

#include "widgets/clayextensiondetails.h"
#include "widgets/clayextensionlist.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayprefsbox.h"

#include "extensions-dialog.h"

#include "clay-intl.h"

#define CLAY_EXTENSION_LIST_STACK_CHILD    "extension-list"
#define CLAY_EXTENSION_DETAILS_STACK_CHILD "extension-details"

typedef struct
{
  Clay              *clay;
  ClayExtensionList *store_list;
  GtkWidget         *search_entry;
  gchar             *category;
} ExtensionsDialogData;

static void extensions_dialog_response            (GtkWidget            *widget,
                                                   gint                  response_id,
                                                   GtkWidget            *dialog);
static void extensions_dialog_search_changed      (GtkSearchEntry       *entry,
                                                   gpointer              user_data);
static void extensions_dialog_category_toggled    (GtkToggleButton      *button,
                                                   gpointer              user_data);
static void extensions_dialog_install_file        (GtkButton            *button,
                                                   gpointer              user_data);
static void extensions_dialog_refresh_store       (GtkButton            *button,
                                                   gpointer              user_data);
static void extensions_dialog_install_requested   (ClayExtensionList    *list,
                                                   const gchar          *entry_id,
                                                   gpointer              user_data);
static void extensions_dialog_extension_activated (ClayExtensionList    *list,
                                                   ClayExtension        *extension,
                                                   GtkStack             *stack);
static void extensions_dialog_store_activated     (ClayExtensionList    *list,
                                                   ClayStoreEntry       *entry,
                                                   GtkStack             *stack);
static void extensions_dialog_back_button_clicked (GtkButton            *button,
                                                   GtkStack             *stack);

static ExtensionsDialogData *
extensions_dialog_data_new (Clay *clay)
{
  ExtensionsDialogData *data = g_new0 (ExtensionsDialogData, 1);

  data->clay     = clay;
  data->category = g_strdup ("all");
  return data;
}

static void
extensions_dialog_data_free (gpointer ptr)
{
  ExtensionsDialogData *data = ptr;

  g_free (data->category);
  g_free (data);
}

static GtkWidget *
extensions_dialog_add_category_chip (GtkWidget            *box,
                                     GtkWidget           **group,
                                     const gchar          *id,
                                     const gchar          *label,
                                     gboolean              active,
                                     ExtensionsDialogData *data)
{
  GtkWidget *button;

  if (*group)
    button = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (*group),
                                                          label);
  else
    button = gtk_radio_button_new_with_label (NULL, label);

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
                               "clay-store-chip");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
  g_object_set_data_full (G_OBJECT (button), "category", g_strdup (id), g_free);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (extensions_dialog_category_toggled), data);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
  if (! *group)
    *group = button;
  return button;
}

static void
extensions_dialog_ensure_contrast (void)
{
  static gboolean done = FALSE;
  GtkCssProvider *css;

  if (done)
    return;
  done = TRUE;

  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css,
    "button.clay-store-chip {"
    "  color: #f5f5f5;"
    "  background-color: #2a2a2a;"
    "  border: 1px solid #9a9a9a;"
    "  padding: 4px 10px;"
    "  font-weight: 600;"
    "}"
    "button.clay-store-chip:checked {"
    "  color: #ffffff;"
    "  background-color: #1473e6;"
    "  border-color: #ffffff;"
    "}"
    "button.clay-store-power {"
    "  min-width: 76px;"
    "  color: #ffffff;"
    "  background-color: #4a4a4a;"
    "  border: 2px solid #c8c8c8;"
    "  font-weight: 700;"
    "}"
    "button.clay-store-power:checked {"
    "  color: #ffffff;"
    "  background-color: #128a3e;"
    "  border-color: #b6f0c8;"
    "}"
    "button.clay-store-get {"
    "  color: #ffffff;"
    "  background-color: #1473e6;"
    "  border: 1px solid #ffffff;"
    "  font-weight: 700;"
    "}",
    -1, NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);
}

GtkWidget *
extensions_dialog_new (Clay *clay)
{
  GtkWidget            *dialog;
  GtkWidget            *stack;
  GtkWidget            *stacked;
  GtkWidget            *vbox;
  GtkWidget            *hbox;
  GtkWidget            *list;
  GtkWidget            *widget;
  GtkTreeIter           top_iter;
  ExtensionsDialogData *data;

  extensions_dialog_ensure_contrast ();
  data = extensions_dialog_data_new (clay);

  dialog = clay_dialog_new (C_("Clay extensions", "Clay App Store"), "clay-extensions",
                            NULL, 0, NULL,
                            CLAY_HELP_EXTENSIONS_DIALOG,
                            _("_OK"), GTK_RESPONSE_OK,
                            NULL);
  g_object_set_data_full (G_OBJECT (dialog), "dialog-data", data,
                          extensions_dialog_data_free);

  widget = gtk_window_get_titlebar (GTK_WINDOW (dialog));
  if (widget)
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (widget),
                                          FALSE);

  gtk_window_set_default_size (GTK_WINDOW (dialog), 780, 560);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (extensions_dialog_response),
                    dialog);

  stack = gtk_stack_new ();
  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 clay_widget_animation_enabled ()        ?
                                   GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT :
                                   GTK_STACK_TRANSITION_TYPE_NONE);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      stack, TRUE, TRUE, 0);
  gtk_widget_set_visible (stack, TRUE);

  stacked = clay_prefs_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (stacked), 12);
  gtk_stack_add_named (GTK_STACK (stack), stacked,
                       CLAY_EXTENSION_LIST_STACK_CHILD);
  gtk_widget_set_visible (stacked, TRUE);

  /* Discover / App Store — primary tab */
  vbox = clay_prefs_box_add_page (CLAY_PREFS_BOX (stacked),
                                  "system-software-install",
                                  _("Discover"),
                                  _("Clay App Store"),
                                  CLAY_HELP_EXTENSIONS_INSTALL,
                                  NULL,
                                  &top_iter);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);
  gtk_widget_set_visible (hbox, TRUE);

  widget = gtk_search_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (widget),
                                  _("Search plugins, AI tools, effects (e.g. Inhaltssensitives Füllen, Beautify, LUTs)…"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
  gtk_widget_set_visible (widget, TRUE);
  data->search_entry = widget;

  widget = gtk_button_new_with_label (_("Install from File…"));
  g_signal_connect (widget, "clicked",
                    G_CALLBACK (extensions_dialog_install_file), data);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_visible (widget, TRUE);

  widget = gtk_button_new_with_label (_("Refresh Store"));
  g_signal_connect (widget, "clicked",
                    G_CALLBACK (extensions_dialog_refresh_store), data);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_visible (widget, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_set_visible (hbox, TRUE);

  {
    GtkWidget *group = NULL;

    extensions_dialog_add_category_chip (hbox, &group, "all", _("All"), TRUE, data);
    extensions_dialog_add_category_chip (hbox, &group, "featured", _("Featured"), FALSE, data);
    extensions_dialog_add_category_chip (hbox, &group, "ai-inpaint", _("AI & Inpainting"), FALSE, data);
    extensions_dialog_add_category_chip (hbox, &group, "beautify", _("Beautify & Retouch"), FALSE, data);
    extensions_dialog_add_category_chip (hbox, &group, "vfx", _("Filters & VFX"), FALSE, data);
    extensions_dialog_add_category_chip (hbox, &group, "assets", _("Brushes & Assets"), FALSE, data);
    extensions_dialog_add_category_chip (hbox, &group, "workflow", _("Workflow & Batch"), FALSE, data);
  }

  list = clay_extension_list_new (clay->extension_manager);
  data->store_list = CLAY_EXTENSION_LIST (list);
  g_signal_connect (list, "extension-activated",
                    G_CALLBACK (extensions_dialog_extension_activated),
                    stack);
  g_signal_connect (list, "store-entry-activated",
                    G_CALLBACK (extensions_dialog_store_activated),
                    stack);
  g_signal_connect (list, "install-requested",
                    G_CALLBACK (extensions_dialog_install_requested),
                    data);
  g_signal_connect (data->search_entry, "search-changed",
                    G_CALLBACK (extensions_dialog_search_changed),
                    data);
  clay_extension_list_show_store (CLAY_EXTENSION_LIST (list), NULL, "all");
  gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 1);
  gtk_widget_set_visible (list, TRUE);

  /* Installed user DLCs */
  vbox = clay_prefs_box_add_page (CLAY_PREFS_BOX (stacked),
                                  "system-software-install",
                                  _("Installed DLCs"),
                                  _("Installed DLCs"),
                                  CLAY_HELP_EXTENSIONS_INSTALLED,
                                  NULL,
                                  &top_iter);

  list = clay_extension_list_new (clay->extension_manager);
  g_signal_connect (list, "extension-activated",
                    G_CALLBACK (extensions_dialog_extension_activated),
                    stack);
  clay_extension_list_show_user (CLAY_EXTENSION_LIST (list));
  gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 1);
  gtk_widget_set_visible (list, TRUE);

  /* System extensions */
  vbox = clay_prefs_box_add_page (CLAY_PREFS_BOX (stacked),
                                  "system-software-install",
                                  _("System Extensions"),
                                  _("System Extensions"),
                                  CLAY_HELP_EXTENSIONS_SYSTEM,
                                  NULL,
                                  &top_iter);

  list = clay_extension_list_new (clay->extension_manager);
  g_signal_connect (list, "extension-activated",
                    G_CALLBACK (extensions_dialog_extension_activated),
                    stack);
  clay_extension_list_show_system (CLAY_EXTENSION_LIST (list));
  gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 1);
  gtk_widget_set_visible (list, TRUE);

  stacked = clay_extension_details_new ();
  gtk_stack_add_named (GTK_STACK (stack), stacked,
                       CLAY_EXTENSION_DETAILS_STACK_CHILD);
  gtk_widget_set_visible (stacked, TRUE);

  gtk_stack_set_visible_child_name (GTK_STACK (stack),
                                    CLAY_EXTENSION_LIST_STACK_CHILD);
  return dialog;
}

static void
extensions_dialog_response (GtkWidget  *widget,
                            gint        response_id,
                            GtkWidget  *dialog)
{
  gtk_widget_destroy (dialog);
}

static void
extensions_dialog_search_changed (GtkSearchEntry *entry,
                                  gpointer        user_data)
{
  ExtensionsDialogData *data = user_data;

  clay_extension_list_show_store (data->store_list,
                                  gtk_entry_get_text (GTK_ENTRY (entry)),
                                  data->category);
}

static void
extensions_dialog_category_toggled (GtkToggleButton *button,
                                    gpointer         user_data)
{
  ExtensionsDialogData *data = user_data;
  const gchar          *category;

  if (! gtk_toggle_button_get_active (button))
    return;

  category = g_object_get_data (G_OBJECT (button), "category");
  g_free (data->category);
  data->category = g_strdup (category ? category : "all");

  clay_extension_list_show_store (data->store_list,
                                  gtk_entry_get_text (GTK_ENTRY (data->search_entry)),
                                  data->category);
}

static void
extensions_dialog_install_file (GtkButton *button,
                                gpointer   user_data)
{
  ExtensionsDialogData *data = user_data;
  GtkWidget            *chooser;
  GtkWidget            *toplevel;
  gint                  response;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
  chooser = gtk_file_chooser_dialog_new (_("Install Clay Extension"),
                                         GTK_WINDOW (toplevel),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                                         _("_Install"), GTK_RESPONSE_ACCEPT,
                                         NULL);

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter, _("Clay / GIMP extensions (*.gex, *.zip)"));
    gtk_file_filter_add_pattern (filter, "*.gex");
    gtk_file_filter_add_pattern (filter, "*.zip");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
  }

  response = gtk_dialog_run (GTK_DIALOG (chooser));
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
      GError *error = NULL;

      if (! file_gex_install (data->clay, file, &error))
        {
          GtkWidget *msg = gtk_message_dialog_new (GTK_WINDOW (toplevel),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "%s",
                                                   error ? error->message :
                                                   _("Could not install the extension."));
          gtk_dialog_run (GTK_DIALOG (msg));
          gtk_widget_destroy (msg);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  gtk_widget_destroy (chooser);
}

static void
extensions_dialog_refresh_store (GtkButton *button,
                                 gpointer   user_data)
{
  ExtensionsDialogData *data = user_data;

  clay_extension_manager_refresh_store (data->clay->extension_manager);
}

static void
extensions_dialog_install_requested (ClayExtensionList *list,
                                     const gchar       *entry_id,
                                     gpointer           user_data)
{
  ExtensionsDialogData *data  = user_data;
  GError               *error = NULL;
  ClayStoreEntry       *entry;

  if (clay_extension_manager_install_from_store (data->clay->extension_manager,
                                                 entry_id, &error))
    {
      clay_extension_list_show_store (data->store_list,
                                      gtk_entry_get_text (GTK_ENTRY (data->search_entry)),
                                      data->category);
      return;
    }

  entry = clay_extension_manager_find_store_entry (data->clay->extension_manager,
                                                   entry_id);
  if (entry && entry->download_url && *entry->download_url)
    {
      GFile  *remote = g_file_new_for_uri (entry->download_url);
      gchar  *tmp_path;
      GFile  *tmp;
      gint    fd;

      fd = g_file_open_tmp ("clay-store-XXXXXX.gex", &tmp_path, NULL);
      if (fd >= 0)
        g_close (fd, NULL);
      tmp = g_file_new_for_path (tmp_path);

      if (g_file_copy (remote, tmp, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error))
        {
          g_clear_error (&error);
          if (! file_gex_install (data->clay, tmp, &error))
            g_printerr ("Store install failed: %s\n", error ? error->message : "?");
          else
            clay_extension_list_show_store (data->store_list,
                                            gtk_entry_get_text (GTK_ENTRY (data->search_entry)),
                                            data->category);
        }

      g_object_unref (remote);
      g_object_unref (tmp);
      g_free (tmp_path);
    }

  if (error)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (list));
      GtkWidget *msg = gtk_message_dialog_new (GTK_IS_WINDOW (toplevel) ? GTK_WINDOW (toplevel) : NULL,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", error->message);
      gtk_dialog_run (GTK_DIALOG (msg));
      gtk_widget_destroy (msg);
      g_error_free (error);
    }
}

static void
extensions_dialog_show_details (GtkStack *stack)
{
  GtkWidget *dialog = gtk_widget_get_toplevel (GTK_WIDGET (stack));
  GtkWidget *header_bar;
  GtkWidget *widget;

  header_bar = gtk_window_get_titlebar (GTK_WINDOW (dialog));
  widget = gtk_button_new_from_icon_name ("go-previous", GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_signal_connect (widget, "clicked",
                    G_CALLBACK (extensions_dialog_back_button_clicked),
                    stack);
  gtk_widget_set_visible (widget, TRUE);

  if (header_bar)
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), widget);
  else
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                       widget);

  gtk_stack_set_visible_child_name (stack, CLAY_EXTENSION_DETAILS_STACK_CHILD);
}

static void
extensions_dialog_extension_activated (ClayExtensionList *list,
                                       ClayExtension     *extension,
                                       GtkStack          *stack)
{
  GtkWidget *widget;

  widget = gtk_stack_get_child_by_name (stack, CLAY_EXTENSION_DETAILS_STACK_CHILD);
  clay_extension_details_set (CLAY_EXTENSION_DETAILS (widget), extension);
  extensions_dialog_show_details (stack);
}

static void
extensions_dialog_store_activated (ClayExtensionList *list,
                                   ClayStoreEntry    *entry,
                                   GtkStack          *stack)
{
  GtkWidget *widget;

  widget = gtk_stack_get_child_by_name (stack, CLAY_EXTENSION_DETAILS_STACK_CHILD);
  clay_extension_details_set_store (CLAY_EXTENSION_DETAILS (widget), entry);
  extensions_dialog_show_details (stack);
}

static void
extensions_dialog_back_button_clicked (GtkButton *button,
                                       GtkStack  *stack)
{
  gtk_stack_set_visible_child_name (stack,
                                    CLAY_EXTENSION_LIST_STACK_CHILD);
  gtk_widget_destroy (GTK_WIDGET (button));
}
