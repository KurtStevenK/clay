/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayfileentry.c
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "claywidgetstypes.h"

#include "claydialog.h"

#undef CLAY_DISABLE_DEPRECATED
#include "clayfileentry.h"

#include "clayhelpui.h"
#include "clayicons.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: clayfileentry
 * @title: ClayFileEntry
 * @short_description: Widget for entering a filename.
 * @see_also: #ClayPathEditor
 *
 * This widget is used to enter filenames or directories.
 *
 * There is a #GtkEntry for entering the filename manually and a "..."
 * button which will pop up a #GtkFileChooserDialog.
 *
 * You can restrict the #ClayFileEntry to directories. In this
 * case the filename listbox of the #GtkFileChooser dialog will be
 * set to directory mode.
 *
 * If you specify @check_valid as %TRUE in _clay_file_entry_new() the
 * entered filename will be checked for validity and a pixmap will be
 * shown which indicates if the file exists or not.
 *
 * Whenever the user changes the filename, the "filename_changed"
 * signal will be emitted.
 **/


enum
{
  FILENAME_CHANGED,
  LAST_SIGNAL
};

struct _ClayFileEntry
{
  GtkBox                parent_instance;

  GtkWidget            *file_exists;
  GtkWidget            *entry;
  GtkWidget            *browse_button;

  GtkFileChooserNative *file_dialog;

  gchar                *title;
  gboolean              dir_only;
  gboolean              check_valid;
};


static void   clay_file_entry_dispose              (GObject       *object);

static void   clay_file_entry_entry_changed        (GtkWidget     *widget,
                                                    GtkWidget     *button);
static void   clay_file_entry_entry_activate       (GtkWidget     *widget,
                                                    ClayFileEntry *entry);
static gint   clay_file_entry_entry_focus_out      (GtkWidget     *widget,
                                                    GdkEvent      *event,
                                                    ClayFileEntry *entry);
static void   clay_file_entry_file_manager_clicked (GtkWidget     *widget,
                                                    ClayFileEntry *entry);
static void   clay_file_entry_browse_clicked       (GtkWidget     *widget,
                                                    ClayFileEntry *entry);
static void   clay_file_entry_check_filename       (ClayFileEntry *entry);


G_DEFINE_TYPE (ClayFileEntry, _clay_file_entry, GTK_TYPE_BOX)

#define parent_class _clay_file_entry_parent_class

static guint clay_file_entry_signals[LAST_SIGNAL] = { 0 };


static void
_clay_file_entry_class_init (ClayFileEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * ClayFileEntry::filename-changed:
   *
   * This signal is emitted whenever the user changes the filename.
   **/
  clay_file_entry_signals[FILENAME_CHANGED] =
    g_signal_new ("filename-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose   = clay_file_entry_dispose;
}

static void
_clay_file_entry_init (ClayFileEntry *entry)
{
  GtkWidget *image;
  GtkWidget *button;

  entry->title       = NULL;
  entry->file_dialog = NULL;
  entry->check_valid = FALSE;
  entry->file_exists = NULL;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (entry),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (entry), 4);
  gtk_box_set_homogeneous (GTK_BOX (entry), FALSE);

  button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (entry), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  gtk_widget_set_sensitive (button, FALSE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_FILE_MANAGER,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_visible (image, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_file_entry_file_manager_clicked),
                    entry);

  clay_help_set_help_data (button,
                           _("Show file location in the file manager"),
                           NULL);

  entry->browse_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (entry), entry->browse_button, FALSE, FALSE, 0);
  gtk_widget_set_visible (entry->browse_button, TRUE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_DOCUMENT_OPEN,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (entry->browse_button), image);
  gtk_widget_set_visible (image, TRUE);

  g_signal_connect (entry->browse_button, "clicked",
                    G_CALLBACK (clay_file_entry_browse_clicked),
                    entry);

  entry->entry = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (entry), entry->entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (entry->entry, TRUE);

  g_signal_connect (entry->entry, "changed",
                    G_CALLBACK (clay_file_entry_entry_changed),
                    button);
  g_signal_connect (entry->entry, "activate",
                    G_CALLBACK (clay_file_entry_entry_activate),
                    entry);
  g_signal_connect (entry->entry, "focus-out-event",
                    G_CALLBACK (clay_file_entry_entry_focus_out),
                    entry);
}

static void
clay_file_entry_dispose (GObject *object)
{
  ClayFileEntry *entry = CLAY_FILE_ENTRY (object);

  g_clear_pointer (&entry->file_dialog, g_object_unref);

  g_clear_pointer (&entry->title, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/**
 * _clay_file_entry_new:
 * @title:       The title of the #ClayFileEntry dialog.
 * @filename:    The initial filename.
 * @dir_only:    %TRUE if the file entry should accept directories only.
 * @check_valid: %TRUE if the widget should check if the entered file
 *               really exists.
 *
 * You should use #GtkFileChooserButton instead.
 *
 * Returns: A pointer to the new #ClayFileEntry widget.
 **/
GtkWidget *
_clay_file_entry_new (const gchar *title,
                      const gchar *filename,
                      gboolean     dir_only,
                      gboolean     check_valid)
{
  ClayFileEntry *entry;

  entry = g_object_new (CLAY_TYPE_FILE_ENTRY, NULL);

  entry->title       = g_strdup (title);
  entry->dir_only    = dir_only;
  entry->check_valid = check_valid;

  clay_help_set_help_data (entry->browse_button,
                           entry->dir_only ?
                           _("Open a file selector to browse your folders") :
                           _("Open a file selector to browse your files"),
                           NULL);

  if (check_valid)
    {
      entry->file_exists = gtk_image_new_from_icon_name ("gtk-no",
                                                         GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (entry), entry->file_exists, FALSE, FALSE, 0);
      gtk_widget_set_visible (entry->file_exists, TRUE);

      clay_help_set_help_data (entry->file_exists,
                               entry->dir_only ?
                               _("Indicates whether or not the folder exists") :
                               _("Indicates whether or not the file exists"),
                               NULL);
    }

  _clay_file_entry_set_filename (entry, filename);

  return GTK_WIDGET (entry);
}

/**
 * _clay_file_entry_get_filename:
 * @entry: The file entry you want to know the filename from.
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The file or directory the user has entered.
 **/
gchar *
_clay_file_entry_get_filename (ClayFileEntry *entry)
{
  gchar *utf8;
  gchar *filename;

  g_return_val_if_fail (CLAY_IS_FILE_ENTRY (entry), NULL);

  utf8 = gtk_editable_get_chars (GTK_EDITABLE (entry->entry), 0, -1);

  filename = g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL);

  g_free (utf8);

  return filename;
}

/**
 * _clay_file_entry_set_filename:
 * @entry:    The file entry you want to set the filename for.
 * @filename: The new filename.
 *
 * If you specified @check_valid as %TRUE in _clay_file_entry_new()
 * the #ClayFileEntry will immediately check the validity of the file
 * name.
 **/
void
_clay_file_entry_set_filename (ClayFileEntry *entry,
                               const gchar   *filename)
{
  gchar *utf8;

  g_return_if_fail (CLAY_IS_FILE_ENTRY (entry));

  if (filename)
    utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
  else
    utf8 = g_strdup ("");

  gtk_entry_set_text (GTK_ENTRY (entry->entry), utf8);
  g_free (utf8);

  /*  update everything
   */
  clay_file_entry_entry_activate (entry->entry, entry);
}

/**
 * _clay_file_entry_get_entry:
 * @entry: The #ClayFileEntry.
 *
 * Returns: (transfer none): the #GtkEntry internally used by the
 *          widget. The object belongs to @entry and should not be
 *          freed.
 **/
GtkWidget *
_clay_file_entry_get_entry (ClayFileEntry *entry)
{
  return entry->entry;
}

/* Private Functions */

static void
clay_file_entry_entry_changed (GtkWidget *widget,
                               GtkWidget *button)
{
  const gchar *text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (text && strlen (text))
    gtk_widget_set_sensitive (button, TRUE);
  else
    gtk_widget_set_sensitive (button, FALSE);
}

static void
clay_file_entry_entry_activate (GtkWidget     *widget,
                                ClayFileEntry *entry)
{
  gchar *utf8;
  gchar *filename;
  gint   len;

  /*  filenames still need more sanity checking
   *  (erase double G_DIR_SEPARATORS, ...)
   */
  utf8 = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  utf8 = g_strstrip (utf8);

  while (((len = strlen (utf8)) > 1) &&
         (utf8[len - 1] == G_DIR_SEPARATOR))
    utf8[len - 1] = '\0';

  filename = g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL);

  g_signal_handlers_block_by_func (entry->entry,
                                   clay_file_entry_entry_activate,
                                   entry);
  gtk_entry_set_text (GTK_ENTRY (entry->entry), utf8);
  g_signal_handlers_unblock_by_func (entry->entry,
                                     clay_file_entry_entry_activate,
                                     entry);

  if (entry->file_dialog)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (entry->file_dialog),
                                   filename);

  g_free (filename);
  g_free (utf8);

  clay_file_entry_check_filename (entry);

  gtk_editable_set_position (GTK_EDITABLE (entry->entry), -1);

  g_signal_emit (entry, clay_file_entry_signals[FILENAME_CHANGED], 0);
}

static gboolean
clay_file_entry_entry_focus_out (GtkWidget     *widget,
                                 GdkEvent      *event,
                                 ClayFileEntry *entry)
{
  clay_file_entry_entry_activate (widget, entry);

  return FALSE;
}

/*  local callback of clay_file_entry_browse_clicked()  */
static void
clay_file_entry_chooser_response (GtkNativeDialog *dialog,
                                  gint             response_id,
                                  ClayFileEntry   *entry)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      _clay_file_entry_set_filename (entry, filename);
      g_free (filename);
    }

  gtk_native_dialog_hide (dialog);
}

static void
clay_file_entry_file_manager_clicked (GtkWidget     *widget,
                                      ClayFileEntry *entry)
{
  gchar  *utf8;
  GFile  *file;
  GError *error = NULL;

  utf8 = gtk_editable_get_chars (GTK_EDITABLE (entry->entry), 0, -1);
  file = g_file_parse_name (utf8);
  g_free (utf8);

  if (! clay_file_show_in_file_manager (file, &error))
    {
      g_message (_("Can't show file in file manager: %s"),
                 error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

static void
clay_file_entry_browse_clicked (GtkWidget     *widget,
                                ClayFileEntry *entry)
{
  gchar *utf8;
  gchar *filename;

  utf8 = gtk_editable_get_chars (GTK_EDITABLE (entry->entry), 0, -1);
  filename = g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL);
  g_free (utf8);

  if (! entry->file_dialog)
    {
      const gchar *title = entry->title;

      if (! title)
        {
          if (entry->dir_only)
            title = _("Select Folder");
          else
            title = _("Select File");
        }

      entry->file_dialog =
        gtk_file_chooser_native_new (title, NULL,
                                     entry->dir_only ?
                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER :
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     _("_OK"), _("_Cancel"));

      g_signal_connect (entry->file_dialog, "response",
                        G_CALLBACK (clay_file_entry_chooser_response),
                        entry);
    }

  if (entry->dir_only)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (entry->file_dialog),
                                         filename);
  else
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (entry->file_dialog),
                                   filename);

  g_free (filename);

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (entry->file_dialog));
}

static void
clay_file_entry_check_filename (ClayFileEntry *entry)
{
  gchar    *utf8;
  gchar    *filename;
  gboolean  exists;

  if (! entry->check_valid || ! entry->file_exists)
    return;

  utf8 = gtk_editable_get_chars (GTK_EDITABLE (entry->entry), 0, -1);
  filename = g_filename_from_utf8 (utf8, -1, NULL, NULL, NULL);
  g_free (utf8);

  if (entry->dir_only)
    exists = g_file_test (filename, G_FILE_TEST_IS_DIR);
  else
    exists = g_file_test (filename, G_FILE_TEST_IS_REGULAR);

  g_free (filename);

  gtk_image_set_from_icon_name (GTK_IMAGE (entry->file_exists),
                                exists ? "gtk-yes" : "gtk-no",
                                GTK_ICON_SIZE_BUTTON);
}
