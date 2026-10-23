/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayerrorconsole.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * partly based on errorconsole.c
 * Copyright (C) 1998 Nick Fetchak <nuke@bayside.net>
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

#include "claydocked.h"
#include "clayerrorconsole.h"
#include "claymenufactory.h"
#include "claysessioninfo-aux.h"
#include "claytextbuffer.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static const gboolean default_highlight[] =
{
  [CLAY_MESSAGE_ERROR]   = TRUE,
  [CLAY_MESSAGE_WARNING] = TRUE,
  [CLAY_MESSAGE_INFO]    = FALSE
};


static void       clay_error_console_docked_iface_init (ClayDockedInterface *iface);

static void       clay_error_console_constructed       (GObject          *object);
static void       clay_error_console_dispose           (GObject          *object);

static void       clay_error_console_unmap             (GtkWidget        *widget);

static gboolean   clay_error_console_button_press      (GtkWidget        *widget,
                                                        GdkEventButton   *event,
                                                        ClayErrorConsole *console);

static void       clay_error_console_set_aux_info      (ClayDocked       *docked,
                                                        GList            *aux_info);
static GList    * clay_error_console_get_aux_info      (ClayDocked       *docked);


G_DEFINE_TYPE_WITH_CODE (ClayErrorConsole, clay_error_console, CLAY_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_error_console_docked_iface_init))

#define parent_class clay_error_console_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_error_console_class_init (ClayErrorConsoleClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = clay_error_console_constructed;
  object_class->dispose     = clay_error_console_dispose;

  widget_class->unmap       = clay_error_console_unmap;
}

static void
clay_error_console_init (ClayErrorConsole *console)
{
  GtkWidget *scrolled_window;

  console->text_buffer = GTK_TEXT_BUFFER (clay_text_buffer_new ());

  gtk_text_buffer_create_tag (console->text_buffer, "title",
                              "scale",  PANGO_SCALE_LARGE,
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);
  gtk_text_buffer_create_tag (console->text_buffer, "message",
                              NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (console), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_window, TRUE);

  console->text_view = gtk_text_view_new_with_buffer (console->text_buffer);
  g_object_unref (console->text_buffer);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (console->text_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (console->text_view),
                               GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), console->text_view);
  gtk_widget_set_visible (console->text_view, TRUE);

  g_signal_connect (console->text_view, "button-press-event",
                    G_CALLBACK (clay_error_console_button_press),
                    console);

  console->file_dialog = NULL;

  memcpy (console->highlight, default_highlight, sizeof (default_highlight));
}

static void
clay_error_console_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_aux_info = clay_error_console_set_aux_info;
  iface->get_aux_info = clay_error_console_get_aux_info;
}

static void
clay_error_console_constructed (GObject *object)
{
  ClayErrorConsole *console = CLAY_ERROR_CONSOLE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  console->clear_button =
    clay_editor_add_action_button (CLAY_EDITOR (console), "error-console",
                                   "error-console-clear", NULL);

  console->save_button =
    clay_editor_add_action_button (CLAY_EDITOR (console), "error-console",
                                   "error-console-save-all",
                                   "error-console-save-selection",
                                   GDK_SHIFT_MASK,
                                   NULL);
}

static void
clay_error_console_dispose (GObject *object)
{
  ClayErrorConsole *console = CLAY_ERROR_CONSOLE (object);

  if (console->file_dialog)
    gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (console->file_dialog));

  console->clay->message_handler = CLAY_MESSAGE_BOX;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_error_console_unmap (GtkWidget *widget)
{
  ClayErrorConsole *console = CLAY_ERROR_CONSOLE (widget);

  if (console->file_dialog)
    gtk_native_dialog_destroy (GTK_NATIVE_DIALOG (console->file_dialog));

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

GtkWidget *
clay_error_console_new (Clay            *clay,
                        ClayMenuFactory *menu_factory)
{
  ClayErrorConsole *console;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  console = g_object_new (CLAY_TYPE_ERROR_CONSOLE,
                          "menu-factory",   menu_factory,
                          "menu-identifier", "<ErrorConsole>",
                          "ui-path",        "/error-console-popup",
                          NULL);

  console->clay = clay;

  console->clay->message_handler = CLAY_ERROR_CONSOLE;

  return GTK_WIDGET (console);
}

void
clay_error_console_add (ClayErrorConsole    *console,
                        ClayMessageSeverity  severity,
                        const gchar         *domain,
                        const gchar         *message)
{
  const gchar        *desc;
  GtkTextIter         end;
  GtkTextMark        *end_mark;
  GtkTextChildAnchor *anchor;
  GtkWidget          *image;
  gchar              *str;

  g_return_if_fail (CLAY_IS_ERROR_CONSOLE (console));
  g_return_if_fail (domain != NULL);
  g_return_if_fail (message != NULL);

  clay_enum_get_value (CLAY_TYPE_MESSAGE_SEVERITY, severity,
                       NULL, NULL, &desc, NULL);

  gtk_text_buffer_get_end_iter (console->text_buffer, &end);

  anchor = gtk_text_child_anchor_new ();
  gtk_text_buffer_insert_child_anchor (console->text_buffer, &end, anchor);

  image = gtk_image_new_from_icon_name (clay_get_message_icon_name (severity),
                                        GTK_ICON_SIZE_BUTTON);
  gtk_text_view_add_child_at_anchor (GTK_TEXT_VIEW (console->text_view),
                                     image, anchor);
  gtk_widget_set_visible (image, TRUE);

  g_object_unref (anchor);

  gtk_text_buffer_insert (console->text_buffer, &end, "  ", -1);

  str = g_strdup_printf ("%s %s", domain, desc);
  gtk_text_buffer_insert_with_tags_by_name (console->text_buffer, &end,
                                            str, -1,
                                            "title",
                                            NULL);
  g_free (str);

  gtk_text_buffer_insert (console->text_buffer, &end, "\n", -1);

  gtk_text_buffer_insert_with_tags_by_name (console->text_buffer, &end,
                                            message, -1,
                                            "message",
                                            NULL);

  gtk_text_buffer_insert (console->text_buffer, &end, "\n\n", -1);

  end_mark = gtk_text_buffer_create_mark (console->text_buffer,
                                          NULL, &end, TRUE);
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (console->text_view), end_mark,
                                FALSE, TRUE, 1.0, 0.0);
  gtk_text_buffer_delete_mark (console->text_buffer, end_mark);
}


/*  private functions  */

static gboolean
clay_error_console_button_press (GtkWidget        *widget,
                                 GdkEventButton   *bevent,
                                 ClayErrorConsole *console)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      return clay_editor_popup_menu_at_pointer (CLAY_EDITOR (console), (GdkEvent *) bevent);
    }

  return FALSE;
}

static const gchar * const aux_info_highlight[] =
{
  [CLAY_MESSAGE_ERROR]   = "highlight-error",
  [CLAY_MESSAGE_WARNING] = "highlight-warning",
  [CLAY_MESSAGE_INFO]    = "highlight-info"
};

static void
clay_error_console_set_aux_info (ClayDocked *docked,
                                 GList      *aux_info)
{
  ClayErrorConsole *console = CLAY_ERROR_CONSOLE (docked);
  GList            *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;
      gint                i;

      for (i = 0; i < G_N_ELEMENTS (aux_info_highlight); i++)
        {
          if (! strcmp (aux->name, aux_info_highlight[i]))
            {
              console->highlight[i] = ! strcmp (aux->value, "yes");
              break;
            }
        }
    }
}

static GList *
clay_error_console_get_aux_info (ClayDocked *docked)
{
  ClayErrorConsole   *console = CLAY_ERROR_CONSOLE (docked);
  GList              *aux_info;
  gint                i;

  aux_info = parent_docked_iface->get_aux_info (docked);

  for (i = 0; i < G_N_ELEMENTS (aux_info_highlight); i++)
    {
      ClaySessionInfoAux *aux;

      aux = clay_session_info_aux_new (aux_info_highlight[i],
                                       console->highlight[i] ? "yes" : "no");

      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}
