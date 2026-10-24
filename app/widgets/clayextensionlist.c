/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayextensionlist.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clayextension.h"
#include "core/clayextensionmanager.h"

#include "clayextensionlist.h"

#include "clay-intl.h"

enum
{
  EXTENSION_ACTIVATED,
  STORE_ENTRY_ACTIVATED,
  INSTALL_REQUESTED,
  LAST_SIGNAL
};

typedef enum
{
  CLAY_EXT_LIST_USER,
  CLAY_EXT_LIST_SYSTEM,
  CLAY_EXT_LIST_SEARCH,
} ClayExtensionListContents;

struct _ClayExtensionListPrivate
{
  ClayExtensionManager      *manager;

  ClayExtensionListContents  contents;
  gchar                     *search_terms;
  gchar                     *category;
};

static void clay_extension_list_set            (ClayExtensionList *list,
                                                const GList       *extensions,
                                                gboolean           is_system);

static void clay_extension_list_ext_installed  (ClayExtensionManager *manager,
                                                ClayExtension        *extension,
                                                gboolean              is_system_ext,
                                                ClayExtensionList    *list);
static void clay_extension_list_ext_removed    (ClayExtensionManager *manager,
                                                gchar                *extension_id,
                                                ClayExtensionList    *list);
static void clay_extension_list_store_updated  (ClayExtensionManager *manager,
                                                ClayExtensionList    *list);

static void clay_extension_list_delete_clicked (GtkButton            *delbutton,
                                                ClayExtensionList    *list);
static void clay_extension_switch_active       (GObject              *onoff,
                                                GParamSpec           *spec,
                                                gpointer              extension);
static void clay_extension_row_activated       (GtkListBox           *box,
                                                GtkListBoxRow        *row,
                                                gpointer              user_data);
static void clay_extension_list_add_store_row  (ClayExtensionList    *list,
                                                ClayStoreEntry       *entry);
static void clay_extension_list_get_clicked    (GtkButton            *button,
                                                ClayExtensionList    *list);
static void clay_extension_store_power_toggled (GtkToggleButton      *button,
                                                gpointer              extension);

G_DEFINE_TYPE_WITH_PRIVATE (ClayExtensionList, clay_extension_list,
                            GTK_TYPE_LIST_BOX)

#define parent_class clay_extension_list_parent_class

static guint signals[LAST_SIGNAL] = { 0, };

static void
clay_extension_list_class_init (ClayExtensionListClass *klass)
{
  signals[EXTENSION_ACTIVATED] =
    g_signal_new ("extension-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayExtensionListClass, extension_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_OBJECT);

  signals[STORE_ENTRY_ACTIVATED] =
    g_signal_new ("store-entry-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayExtensionListClass, store_entry_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  signals[INSTALL_REQUESTED] =
    g_signal_new ("install-requested",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayExtensionListClass, install_requested),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
clay_extension_list_init (ClayExtensionList *list)
{
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list),
                                   GTK_SELECTION_SINGLE);
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list),
                                             FALSE);
  list->p = clay_extension_list_get_instance_private (list);
  g_signal_connect (list, "row-activated",
                    G_CALLBACK (clay_extension_row_activated), NULL);
}

static void
clay_extension_list_disable (GtkWidget *row,
                             gchar     *extension_id)
{
  ClayExtension *extension;
  GtkWidget     *outframe = gtk_bin_get_child (GTK_BIN (row));

  if (! outframe)
    return;

  extension = g_object_get_data (G_OBJECT (outframe), "extension");
  if (! extension)
    return;

  if (g_strcmp0 (clay_object_get_name (extension), extension_id) == 0)
    gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (outframe)), FALSE);
}

GtkWidget *
clay_extension_list_new (ClayExtensionManager *manager)
{
  ClayExtensionList *list;

  g_return_val_if_fail (CLAY_IS_EXTENSION_MANAGER (manager), NULL);

  list = g_object_new (CLAY_TYPE_EXTENSION_LIST, NULL);
  list->p->manager  = manager;

  g_signal_connect (manager, "extension-installed",
                    G_CALLBACK (clay_extension_list_ext_installed),
                    list);
  g_signal_connect (manager, "extension-removed",
                    G_CALLBACK (clay_extension_list_ext_removed),
                    list);
  g_signal_connect (manager, "store-updated",
                    G_CALLBACK (clay_extension_list_store_updated),
                    list);

  return GTK_WIDGET (list);
}

void
clay_extension_list_show_system (ClayExtensionList *list)
{
  list->p->contents = CLAY_EXT_LIST_SYSTEM;
  clay_extension_list_set (list,
                           clay_extension_manager_get_system_extensions (list->p->manager),
                           TRUE);
}

void
clay_extension_list_show_user (ClayExtensionList *list)
{
  list->p->contents = CLAY_EXT_LIST_USER;
  clay_extension_list_set (list,
                           clay_extension_manager_get_user_extensions (list->p->manager),
                           FALSE);
}

void
clay_extension_list_show_search (ClayExtensionList *list,
                                 const gchar       *search_terms)
{
  clay_extension_list_show_store (list, search_terms, "all");
}

void
clay_extension_list_show_store (ClayExtensionList *list,
                                const gchar       *search_terms,
                                const gchar       *category)
{
  GList *matches;
  GList *iter;

  list->p->contents = CLAY_EXT_LIST_SEARCH;
  g_free (list->p->search_terms);
  g_free (list->p->category);
  list->p->search_terms = g_strdup (search_terms);
  list->p->category     = g_strdup (category);

  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  matches = clay_extension_manager_search_store (list->p->manager,
                                                 search_terms, category);
  for (iter = matches; iter; iter = iter->next)
    clay_extension_list_add_store_row (list, iter->data);
  g_list_free (matches);

  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_list_box_row_set_activatable,
                         GUINT_TO_POINTER (TRUE));
}

static void
clay_extension_list_store_updated (ClayExtensionManager *manager,
                                   ClayExtensionList    *list)
{
  if (list->p->contents == CLAY_EXT_LIST_SEARCH)
    clay_extension_list_show_store (list, list->p->search_terms, list->p->category);
}

static void
clay_extension_list_set (ClayExtensionList *list,
                         const GList       *extensions,
                         gboolean           is_system)
{
  GList *iter = (GList *) extensions;

  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  for (; iter; iter = iter->next)
    {
      ClayExtension *extension = iter->data;

      clay_extension_list_ext_installed (list->p->manager, extension,
                                         is_system, list);
    }
  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_list_box_row_set_activatable,
                         GUINT_TO_POINTER (TRUE));
}

static GtkWidget *
clay_extension_list_make_icon (const gchar *icon_name)
{
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (icon_name && *icon_name ?
                                        icon_name : "application-x-addon",
                                        GTK_ICON_SIZE_DND);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 42);
  gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_visible (image, TRUE);
  return image;
}

static void
clay_extension_list_add_store_row (ClayExtensionList *list,
                                   ClayStoreEntry    *entry)
{
  GtkWidget     *card;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *title_box;
  GtkWidget     *label;
  GtkWidget     *button;
  ClayExtension *installed;
  gchar         *markup;
  const gchar   *summary;

  installed = clay_extension_manager_lookup (list->p->manager, entry->id);

  card = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (card), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (list), card);
  g_object_set_data (G_OBJECT (card), "store-entry", entry);
  g_object_set_data (G_OBJECT (card), "extension", installed);
  gtk_widget_set_visible (card, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_container_add (GTK_CONTAINER (card), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  gtk_box_pack_start (GTK_BOX (hbox),
                      clay_extension_list_make_icon (entry->icon_name),
                      FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  title_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start (GTK_BOX (vbox), title_box, FALSE, FALSE, 0);
  gtk_widget_set_visible (title_box, TRUE);

  markup = g_markup_printf_escaped ("<b>%s</b>",
                                    entry->name ? entry->name : entry->id);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (title_box), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);
  g_free (markup);

  if (entry->version)
    {
      label = gtk_label_new (entry->version);
      gtk_style_context_add_class (gtk_widget_get_style_context (label),
                                   "dim-label");
      gtk_box_pack_start (GTK_BOX (title_box), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);
    }

  if (entry->featured)
    {
      label = gtk_label_new (_("Featured"));
      gtk_box_pack_start (GTK_BOX (title_box), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);
    }

  if (entry->category)
    {
      label = gtk_label_new (entry->category);
      gtk_box_pack_end (GTK_BOX (title_box), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);
    }

  summary = entry->summary_de && *entry->summary_de ? entry->summary_de : entry->summary;
  if (summary)
    {
      label = gtk_label_new (summary);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_max_width_chars (GTK_LABEL (label), 60);
      gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
      gtk_widget_set_visible (label, TRUE);
    }

  if (entry->rating > 0)
    {
      gchar *stars = g_strdup_printf ("★ %.1f", entry->rating);

      label = gtk_label_new (stars);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);
      g_free (stars);
    }

  if (installed)
    {
      GtkWidget *onoff = gtk_toggle_button_new_with_label (_("Aus"));
      gboolean   running;

      running = clay_extension_manager_is_running (list->p->manager, installed);
      gtk_widget_set_valign (onoff, GTK_ALIGN_CENTER);
      gtk_style_context_add_class (gtk_widget_get_style_context (onoff),
                                   "clay-store-power");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (onoff), running);
      gtk_button_set_label (GTK_BUTTON (onoff), running ? _("An") : _("Aus"));
      gtk_widget_set_sensitive (onoff,
                                clay_extension_manager_can_run (list->p->manager,
                                                                installed));
      g_signal_connect (onoff, "toggled",
                        G_CALLBACK (clay_extension_store_power_toggled),
                        installed);
      gtk_box_pack_end (GTK_BOX (hbox), onoff, FALSE, FALSE, 0);
      gtk_widget_set_visible (onoff, TRUE);
    }
  else
    {
      button = gtk_button_new_with_label (_("Installieren"));
      gtk_style_context_add_class (gtk_widget_get_style_context (button),
                                   "clay-store-get");
      gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
      g_object_set_data_full (G_OBJECT (button), "entry-id",
                              g_strdup (entry->id), g_free);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (clay_extension_list_get_clicked),
                        list);
      gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_set_visible (button, TRUE);
    }
}

static void
clay_extension_list_get_clicked (GtkButton         *button,
                                 ClayExtensionList *list)
{
  const gchar *entry_id;

  entry_id = g_object_get_data (G_OBJECT (button), "entry-id");
  if (entry_id)
    g_signal_emit (list, signals[INSTALL_REQUESTED], 0, entry_id);
}

static void
clay_extension_list_ext_installed (ClayExtensionManager *manager,
                                   ClayExtension        *extension,
                                   gboolean              is_system_ext,
                                   ClayExtensionList    *list)
{
  GList     *rows;
  GList     *iter;

  GtkWidget *outframe;
  GtkWidget *grid;
  GtkWidget *onoff;
  GtkWidget *image;

  if (list->p->contents == CLAY_EXT_LIST_SEARCH)
    {
      clay_extension_list_show_store (list, list->p->search_terms, list->p->category);
      return;
    }

  if ((list->p->contents == CLAY_EXT_LIST_USER && is_system_ext) ||
      (list->p->contents == CLAY_EXT_LIST_SYSTEM && ! is_system_ext))
    return;

  /* Check if extension already listed (i.e. removed then reinstalled). */
  rows = gtk_container_get_children (GTK_CONTAINER (list));
  for (iter = rows; iter; iter = iter->next)
    {
      ClayExtension *row_ext;
      GtkWidget     *frame = gtk_bin_get_child (GTK_BIN (iter->data));

      if (! frame)
        continue;

      row_ext = g_object_get_data (G_OBJECT (frame), "extension");
      if (! row_ext)
        continue;

      if (extension == row_ext)
        {
          gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (frame)), TRUE);
          g_list_free (rows);
          return;
        }
    }
  g_list_free (rows);

  outframe = gtk_frame_new (clay_extension_get_name (extension));
  gtk_container_add (GTK_CONTAINER (list), outframe);
  g_object_set_data (G_OBJECT (outframe), "extension", extension);
  gtk_widget_set_visible (outframe, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_container_add (GTK_CONTAINER (outframe), grid);
  gtk_widget_set_visible (grid, TRUE);

  onoff = gtk_switch_new ();
  gtk_widget_set_vexpand (onoff, FALSE);
  gtk_widget_set_hexpand (onoff, FALSE);
  gtk_widget_set_halign (onoff, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (onoff, GTK_ALIGN_CENTER);
  gtk_switch_set_active (GTK_SWITCH (onoff),
                         clay_extension_manager_is_running (list->p->manager,
                                                            extension));
  gtk_widget_set_sensitive (onoff,
                            clay_extension_manager_can_run (list->p->manager,
                                                            extension));
  g_signal_connect (onoff, "notify::active",
                    G_CALLBACK (clay_extension_switch_active), extension);
  gtk_grid_attach (GTK_GRID (grid), onoff, 0, 0, 1, 1);
  gtk_widget_set_visible (onoff, TRUE);

  if (clay_extension_get_comment (extension))
    {
      GtkWidget     *desc = gtk_text_view_new ();
      GtkTextBuffer *buffer;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (desc));
      gtk_text_buffer_set_text (buffer,
                                clay_extension_get_comment (extension),
                                -1);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (desc), FALSE);
      gtk_widget_set_vexpand (desc, TRUE);
      gtk_widget_set_hexpand (desc, TRUE);
      gtk_grid_attach (GTK_GRID (grid), desc, 1, 0, 1, 1);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (desc),
                                   GTK_WRAP_WORD_CHAR);
      gtk_widget_set_visible (desc, TRUE);
    }

  if (! is_system_ext)
    {
      GtkWidget *delbutton;

      delbutton = gtk_button_new ();
      g_object_set_data (G_OBJECT (delbutton), "extension", extension);
      g_signal_connect (delbutton, "clicked",
                        G_CALLBACK (clay_extension_list_delete_clicked),
                        list);
      gtk_button_set_relief (GTK_BUTTON (delbutton), GTK_RELIEF_NONE);
      image = gtk_image_new_from_icon_name (CLAY_ICON_EDIT_DELETE,
                                            GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
      gtk_widget_set_vexpand (delbutton, FALSE);
      gtk_widget_set_hexpand (delbutton, FALSE);
      gtk_widget_set_halign (delbutton, GTK_ALIGN_END);
      gtk_widget_set_valign (delbutton, GTK_ALIGN_START);
      gtk_container_add (GTK_CONTAINER (delbutton), image);
      gtk_widget_set_visible (image, TRUE);
      gtk_grid_attach (GTK_GRID (grid), delbutton, 2, 0, 1, 1);
      gtk_widget_set_visible (delbutton, TRUE);
    }
}

static void
clay_extension_list_ext_removed (ClayExtensionManager *manager,
                                 gchar                *extension_id,
                                 ClayExtensionList    *list)
{
  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) clay_extension_list_disable,
                         extension_id);
}

static void
clay_extension_list_delete_clicked (GtkButton         *delbutton,
                                    ClayExtensionList *list)
{
  ClayExtensionManager *manager = list->p->manager;
  ClayExtension        *extension;
  GError               *error = NULL;

  extension = g_object_get_data (G_OBJECT (delbutton), "extension");
  g_return_if_fail (extension);

  if (clay_extension_manager_is_removed (manager, extension))
    clay_extension_manager_undo_remove (manager, extension, &error);
  else
    clay_extension_manager_remove (manager, extension, &error);

  if (error)
    {
      g_warning ("%s: %s\n", G_STRFUNC, error->message);
      g_error_free (error);
    }
}

static void
clay_extension_store_power_toggled (GtkToggleButton *button,
                                    gpointer         extension)
{
  ClayExtension *ext = extension;
  gboolean       active = gtk_toggle_button_get_active (button);

  gtk_button_set_label (GTK_BUTTON (button), active ? _("An") : _("Aus"));

  if (active)
    {
      GError *error = NULL;

      clay_extension_run (ext, &error);
      if (error)
        {
          g_signal_handlers_block_by_func (button,
                                           G_CALLBACK (clay_extension_store_power_toggled),
                                           extension);
          gtk_toggle_button_set_active (button, FALSE);
          gtk_button_set_label (GTK_BUTTON (button), _("Aus"));
          g_signal_handlers_unblock_by_func (button,
                                             G_CALLBACK (clay_extension_store_power_toggled),
                                             extension);
          g_printerr ("Extension '%s' failed to run: %s\n",
                      clay_object_get_name (ext),
                      error->message);
          g_error_free (error);
        }
    }
  else
    {
      clay_extension_stop (ext);
    }
}

static void
clay_extension_switch_active (GObject    *onoff,
                              GParamSpec *pspec,
                              gpointer    extension)
{
  ClayExtension *ext = (ClayExtension *) extension;

  if (gtk_switch_get_active (GTK_SWITCH (onoff)))
    {
      GError *error = NULL;

      clay_extension_run (ext, &error);
      if (error)
        {
          g_signal_handlers_block_by_func (onoff,
                                           G_CALLBACK (clay_extension_switch_active),
                                           extension);
          gtk_switch_set_active (GTK_SWITCH (onoff), FALSE);
          g_signal_handlers_unblock_by_func (onoff,
                                             G_CALLBACK (clay_extension_switch_active),
                                             extension);
          g_printerr ("Extension '%s' failed to run: %s\n",
                      clay_object_get_name (ext),
                      error->message);
          g_error_free (error);
        }
    }
  else
    clay_extension_stop (ext);
}

static void
clay_extension_row_activated (GtkListBox    *box,
                              GtkListBoxRow *row,
                              gpointer       user_data)
{
  GtkWidget     *frame = gtk_bin_get_child (GTK_BIN (row));
  ClayExtension *extension;
  ClayStoreEntry *entry;

  if (! frame)
    return;

  entry = g_object_get_data (G_OBJECT (frame), "store-entry");
  if (entry)
    {
      g_signal_emit (box, signals[STORE_ENTRY_ACTIVATED], 0, entry);
      return;
    }

  extension = g_object_get_data (G_OBJECT (frame), "extension");
  if (extension)
    g_signal_emit (box, signals[EXTENSION_ACTIVATED], 0, extension);
}
