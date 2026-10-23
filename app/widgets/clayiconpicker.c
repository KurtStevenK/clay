/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayiconpicker.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
 *               2012 Daniel Sabo
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

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claylist.h"
#include "core/claycontext.h"
#include "core/claytemplate.h"
#include "core/clayviewable.h"

#include "claycontainerpopup.h"
#include "clayiconpicker.h"
#include "clayview.h"
#include "clayviewablebutton.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"

enum
{
  PROP_0,
  PROP_CLAY,
  PROP_ICON_NAME,
  PROP_ICON_PIXBUF
};


typedef struct _ClayIconPickerPrivate ClayIconPickerPrivate;

struct _ClayIconPickerPrivate
{
  Clay          *clay;

  gchar         *icon_name;
  GdkPixbuf     *icon_pixbuf;

  ClayViewable  *preview;

  ClayContainer *icon_name_container;
  ClayContext   *icon_name_context;
  ClayObject    *null_template_object;

  GtkWidget     *right_click_menu;
  GtkWidget     *menu_item_file_icon;
  GtkWidget     *menu_item_name_icon;
  GtkWidget     *menu_item_copy;
  GtkWidget     *menu_item_paste;
};

#define GET_PRIVATE(picker) \
        ((ClayIconPickerPrivate *) clay_icon_picker_get_instance_private ((ClayIconPicker *) (picker)))


static void    clay_icon_picker_constructed     (GObject        *object);
static void    clay_icon_picker_finalize        (GObject        *object);
static void    clay_icon_picker_set_property    (GObject        *object,
                                                 guint           property_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void    clay_icon_picker_get_property    (GObject        *object,
                                                 guint           property_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);

static void    clay_icon_picker_icon_changed    (ClayContext    *context,
                                                 ClayTemplate   *template,
                                                 ClayIconPicker *picker);
static void    clay_icon_picker_clicked         (GtkWidget      *widget,
                                                 GdkEventButton *event,
                                                 gpointer        data);

static void    clay_icon_picker_menu_from_file  (GtkWidget      *widget,
                                                 GdkEventButton *event,
                                                 gpointer        data);
static void    clay_icon_picker_menu_from_name  (GtkWidget      *widget,
                                                 GdkEventButton *event,
                                                 gpointer        data);
static void    clay_icon_picker_menu_paste      (GtkWidget      *widget,
                                                 GdkEventButton *event,
                                                 gpointer        data);
static void    clay_icon_picker_menu_copy       (GtkWidget      *widget,
                                                 GdkEventButton *event,
                                                 gpointer        data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayIconPicker, clay_icon_picker, GTK_TYPE_BOX)

#define parent_class clay_icon_picker_parent_class


static void
clay_icon_picker_class_init (ClayIconPickerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_icon_picker_constructed;
  object_class->finalize     = clay_icon_picker_finalize;
  object_class->set_property = clay_icon_picker_set_property;
  object_class->get_property = clay_icon_picker_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name", NULL, NULL,
                                                        "clay-toilet-paper",
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ICON_PIXBUF,
                                   g_param_spec_object ("icon-pixbuf", NULL, NULL,
                                                        GDK_TYPE_PIXBUF,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_icon_picker_init (ClayIconPicker *picker)
{
  ClayIconPickerPrivate *private = GET_PRIVATE (picker);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (picker),
                                  GTK_ORIENTATION_HORIZONTAL);

  private->preview = g_object_new (CLAY_TYPE_VIEWABLE,
                                   "icon-name",   private->icon_name,
                                   "icon-pixbuf", private->icon_pixbuf,
                                   NULL);
}

static void
clay_icon_picker_constructed (GObject *object)
{
  ClayIconPicker         *picker  = CLAY_ICON_PICKER (object);
  ClayIconPickerPrivate  *private = GET_PRIVATE (object);
  GtkWidget              *button;
  GtkWidget              *viewable_view;
  GList                  *icon_list;
  GList                  *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (private->clay));

  /* Set up the icon picker */
  private->icon_name_container = clay_list_new (CLAY_TYPE_TEMPLATE, FALSE);
  private->icon_name_context = clay_context_new (private->clay, "foo", NULL);

  g_signal_connect (private->icon_name_context, "template-changed",
                    G_CALLBACK (clay_icon_picker_icon_changed),
                    picker);

  icon_list = gtk_icon_theme_list_icons (gtk_icon_theme_get_default (), NULL);

  icon_list = g_list_sort (icon_list, (GCompareFunc) g_strcmp0);
  icon_list = g_list_reverse (icon_list);

  for (list = icon_list; list; list = g_list_next (list))
    {
      ClayObject *object = g_object_new (CLAY_TYPE_TEMPLATE,
                                         "name",      list->data,
                                         "icon-name", list->data,
                                         NULL);

      clay_container_add (private->icon_name_container, object);
      g_object_unref (object);

      if (private->icon_name && strcmp (list->data, private->icon_name) == 0)
        clay_context_set_template (private->icon_name_context,
                                   CLAY_TEMPLATE (object));
    }

  /* An extra template object, use to make all icons clickable when a
   * pixbuf icon is set.
   */
  private->null_template_object = g_object_new (CLAY_TYPE_TEMPLATE,
                                                "name",      "",
                                                "icon-name", "",
                                                NULL);

  if (private->icon_pixbuf)
    {
      clay_context_set_template (private->icon_name_context,
                                 CLAY_TEMPLATE (private->null_template_object));
    }

  g_list_free_full (icon_list, (GDestroyNotify) g_free);


  /* Set up preview button */
  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (picker), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (clay_icon_picker_clicked),
                    object);


  viewable_view = clay_view_new (private->icon_name_context,
                                 private->preview,
                                 CLAY_VIEW_SIZE_SMALL,
                                 0,
                                 FALSE);
  gtk_container_add (GTK_CONTAINER (button), GTK_WIDGET (viewable_view));
  gtk_widget_set_visible (viewable_view, TRUE);

  /* Set up button menu */
  private->right_click_menu = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (private->right_click_menu), button, NULL);

  private->menu_item_file_icon =
    gtk_menu_item_new_with_label (_("From File..."));
  gtk_menu_shell_append (GTK_MENU_SHELL (private->right_click_menu),
                         GTK_WIDGET (private->menu_item_file_icon));

  g_signal_connect (private->menu_item_file_icon, "button-press-event",
                    G_CALLBACK (clay_icon_picker_menu_from_file),
                    object);

  private->menu_item_name_icon =
    gtk_menu_item_new_with_label (_("From Named Icons..."));
  gtk_menu_shell_append (GTK_MENU_SHELL (private->right_click_menu),
                         GTK_WIDGET (private->menu_item_name_icon));

  g_signal_connect (private->menu_item_name_icon, "button-press-event",
                    G_CALLBACK (clay_icon_picker_menu_from_name),
                    object);

  private->menu_item_copy =
    gtk_menu_item_new_with_label (_("Copy Icon to Clipboard"));
  gtk_menu_shell_append (GTK_MENU_SHELL (private->right_click_menu),
                         GTK_WIDGET (private->menu_item_copy));

  g_signal_connect (private->menu_item_copy, "button-press-event",
                    G_CALLBACK (clay_icon_picker_menu_copy),
                    object);

  private->menu_item_paste =
    gtk_menu_item_new_with_label (_("Paste Icon from Clipboard"));
  gtk_menu_shell_append (GTK_MENU_SHELL (private->right_click_menu),
                         GTK_WIDGET (private->menu_item_paste));

  g_signal_connect (private->menu_item_paste, "button-press-event",
                    G_CALLBACK (clay_icon_picker_menu_paste),
                    object);

  gtk_widget_show_all (GTK_WIDGET (private->right_click_menu));
}

static void
clay_icon_picker_finalize (GObject *object)
{
  ClayIconPickerPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->icon_name, g_free);

  g_clear_object (&private->icon_name_container);
  g_clear_object (&private->icon_name_context);
  g_clear_object (&private->icon_pixbuf);
  g_clear_object (&private->preview);
  g_clear_object (&private->null_template_object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_icon_picker_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayIconPickerPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CLAY:
      private->clay = g_value_get_object (value); /* don't ref */
      break;

    case PROP_ICON_NAME:
      clay_icon_picker_set_icon_name (CLAY_ICON_PICKER (object),
                                      g_value_get_string (value));
      break;

    case PROP_ICON_PIXBUF:
      clay_icon_picker_set_icon_pixbuf (CLAY_ICON_PICKER (object),
                                        g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_icon_picker_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayIconPickerPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, private->clay);
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, private->icon_name);
      break;

    case PROP_ICON_PIXBUF:
      g_value_set_object (value, private->icon_pixbuf);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
clay_icon_picker_new (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_new (CLAY_TYPE_ICON_PICKER,
                       "clay", clay,
                       NULL);
}

const gchar *
clay_icon_picker_get_icon_name (ClayIconPicker *picker)
{
  g_return_val_if_fail (CLAY_IS_ICON_PICKER (picker), NULL);

  return GET_PRIVATE (picker)->icon_name;
}

void
clay_icon_picker_set_icon_name (ClayIconPicker *picker,
                                const gchar    *icon_name)
{
  ClayIconPickerPrivate *private;

  g_return_if_fail (CLAY_IS_ICON_PICKER (picker));
  g_return_if_fail (icon_name != NULL);

  private = GET_PRIVATE (picker);

  g_free (private->icon_name);
  private->icon_name = g_strdup (icon_name);

  if (private->icon_name_container)
    {
      ClayObject *object;

      object = clay_container_get_child_by_name (private->icon_name_container,
                                                 icon_name);

      if (object)
        clay_context_set_template (private->icon_name_context,
                                   CLAY_TEMPLATE (object));
    }

  g_object_set (private->preview,
                "icon-name", private->icon_name,
                NULL);

  g_object_notify (G_OBJECT (picker), "icon-name");
}

GdkPixbuf  *
clay_icon_picker_get_icon_pixbuf (ClayIconPicker *picker)
{
  g_return_val_if_fail (CLAY_IS_ICON_PICKER (picker), NULL);

  return GET_PRIVATE (picker)->icon_pixbuf;
}

void
clay_icon_picker_set_icon_pixbuf (ClayIconPicker *picker,
                                  GdkPixbuf      *value)
{
  ClayIconPickerPrivate *private;

  g_return_if_fail (CLAY_IS_ICON_PICKER (picker));
  g_return_if_fail (value == NULL || GDK_IS_PIXBUF (value));

  private = GET_PRIVATE (picker);

  if (private->icon_pixbuf)
    g_object_unref (private->icon_pixbuf);

  private->icon_pixbuf = value;

  if (private->icon_pixbuf)
    {
      g_object_ref (private->icon_pixbuf);

      clay_context_set_template (private->icon_name_context,
                                 CLAY_TEMPLATE (private->null_template_object));
    }
  else
    {
      ClayObject *object;

      object = clay_container_get_child_by_name (private->icon_name_container,
                                                 private->icon_name);

      if (object)
        clay_context_set_template (private->icon_name_context,
                                   CLAY_TEMPLATE (object));
    }

  g_object_set (private->preview,
                "icon-pixbuf", private->icon_pixbuf,
                NULL);

  g_object_notify (G_OBJECT (picker), "icon-pixbuf");
}


/*  private functions  */

static void
clay_icon_picker_icon_changed (ClayContext    *context,
                               ClayTemplate   *template,
                               ClayIconPicker *picker)
{
  ClayIconPickerPrivate *private = GET_PRIVATE (picker);

  if (CLAY_OBJECT (template) != private->null_template_object)
    {
      clay_icon_picker_set_icon_pixbuf (picker, NULL);
      clay_icon_picker_set_icon_name (picker, clay_object_get_name (template));
    }
}

static void
clay_icon_picker_menu_from_file (GtkWidget      *widget,
                                 GdkEventButton *event,
                                 gpointer        object)
{
  ClayIconPicker       *picker = CLAY_ICON_PICKER (object);
  GtkFileChooserNative *dialog;
  GtkFileFilter        *filter;

  dialog = gtk_file_chooser_native_new (_("Load Icon Image"),
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        _("_Open"), _("_Cancel"));

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Image files"));
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  if (gtk_native_dialog_run (GTK_NATIVE_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gchar     *filename;
      GdkPixbuf *icon_pixbuf = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      icon_pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

      if (icon_pixbuf)
        {
          clay_icon_picker_set_icon_pixbuf (picker, icon_pixbuf);
          g_object_unref (icon_pixbuf);
        }

      g_free (filename);
    }

  gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (dialog));
}

static void
clay_icon_picker_menu_copy (GtkWidget      *widget,
                            GdkEventButton *event,
                            gpointer        object)
{
  ClayIconPicker        *picker    = CLAY_ICON_PICKER (object);
  ClayIconPickerPrivate *private   = GET_PRIVATE (picker);
  GtkClipboard          *clipboard = NULL;

  clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (widget),
                                             GDK_SELECTION_CLIPBOARD);

  if (private->icon_pixbuf)
    {
      gtk_clipboard_set_image (clipboard, private->icon_pixbuf);
    }
}

static void
clay_icon_picker_menu_paste (GtkWidget      *widget,
                             GdkEventButton *event,
                             gpointer        object)
{
  ClayIconPicker *picker           = CLAY_ICON_PICKER (object);
  GtkClipboard   *clipboard        = NULL;
  GdkPixbuf      *clipboard_pixbuf = NULL;

  clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (widget),
                                             GDK_SELECTION_CLIPBOARD);

  clipboard_pixbuf = gtk_clipboard_wait_for_image (clipboard);

  if (clipboard_pixbuf)
    {
      clay_icon_picker_set_icon_pixbuf (picker, clipboard_pixbuf);
      g_object_unref (clipboard_pixbuf);
    }
}

static void
clay_icon_picker_clicked (GtkWidget      *widget,
                          GdkEventButton *event,
                          gpointer        object)
{
  ClayIconPicker        *picker    = CLAY_ICON_PICKER (object);
  ClayIconPickerPrivate *private   = GET_PRIVATE (picker);
  GtkClipboard          *clipboard = NULL;

  clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (widget),
                                             GDK_SELECTION_CLIPBOARD);

  if (gtk_clipboard_wait_is_image_available (clipboard))
    gtk_widget_set_sensitive (private->menu_item_paste, TRUE);
  else
    gtk_widget_set_sensitive (private->menu_item_paste, FALSE);

  if (private->icon_pixbuf)
    gtk_widget_set_sensitive (private->menu_item_copy, TRUE);
  else
    gtk_widget_set_sensitive (private->menu_item_copy, FALSE);

  gtk_menu_popup_at_widget (GTK_MENU (private->right_click_menu),
                            widget,
                            GDK_GRAVITY_EAST,
                            GDK_GRAVITY_NORTH_WEST,
                            (GdkEvent *) event);
}

static void
clay_icon_picker_menu_from_name (GtkWidget      *widget,
                                 GdkEventButton *event,
                                 gpointer        object)
{
  ClayIconPicker        *picker  = CLAY_ICON_PICKER (object);
  ClayIconPickerPrivate *private = GET_PRIVATE (picker);
  GtkWidget             *popup;

  /* FIXME: Right clicking on this popup can cause a crash */
  popup = clay_container_popup_new (private->icon_name_container,
                                    private->icon_name_context,
                                    CLAY_VIEW_TYPE_LIST,
                                    CLAY_VIEW_SIZE_SMALL,
                                    CLAY_VIEW_SIZE_SMALL,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

  clay_container_popup_set_view_type (CLAY_CONTAINER_POPUP (popup),
                                      CLAY_VIEW_TYPE_GRID);

  clay_popup_show (CLAY_POPUP (popup), GTK_WIDGET (picker));
}
