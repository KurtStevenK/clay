/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextEditor
 * Copyright (C) 2002-2003, 2008  Sven Neumann <sven@gimp.org>
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

#include "core/clay.h"
#include "core/claydatafactory.h"

#include "text/claytext.h"

#include "clayhelp-ids.h"
#include "claymenufactory.h"
#include "claytextbuffer.h"
#include "claytexteditor.h"
#include "claytextstyleeditor.h"
#include "claytoolbar.h"
#include "clayuimanager.h"

#include "clay-intl.h"


enum
{
  TEXT_CHANGED,
  DIR_CHANGED,
  LAST_SIGNAL
};


static void   clay_text_editor_finalize     (GObject         *object);

static void   clay_text_editor_text_changed (GtkTextBuffer   *buffer,
                                             ClayTextEditor  *editor);
static void   clay_text_editor_font_toggled (GtkToggleButton *button,
                                             ClayTextEditor  *editor);


G_DEFINE_TYPE (ClayTextEditor, clay_text_editor, CLAY_TYPE_DIALOG)

#define parent_class clay_text_editor_parent_class

static guint text_editor_signals[LAST_SIGNAL] = { 0 };


static void
clay_text_editor_class_init (ClayTextEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_text_editor_finalize;

  klass->text_changed    = NULL;
  klass->dir_changed     = NULL;

  text_editor_signals[TEXT_CHANGED] =
    g_signal_new ("text-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayTextEditorClass, text_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  text_editor_signals[DIR_CHANGED] =
    g_signal_new ("dir-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayTextEditorClass, dir_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
clay_text_editor_init (ClayTextEditor *editor)
{
  switch (gtk_widget_get_default_direction ())
    {
    case GTK_TEXT_DIR_NONE:
    case GTK_TEXT_DIR_LTR:
      editor->base_dir = CLAY_TEXT_DIRECTION_LTR;
      break;
    case GTK_TEXT_DIR_RTL:
      editor->base_dir = CLAY_TEXT_DIRECTION_RTL;
      break;
    }
}

static void
clay_text_editor_finalize (GObject *object)
{
  ClayTextEditor *editor = CLAY_TEXT_EDITOR (object);

  g_clear_pointer (&editor->font_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GtkWidget *
clay_text_editor_new (const gchar     *title,
                      GtkWindow       *parent,
                      Clay            *clay,
                      ClayMenuFactory *menu_factory,
                      ClayText        *text,
                      ClayTextBuffer  *text_buffer,
                      gdouble          xres,
                      gdouble          yres)
{
  ClayTextEditor  *editor;
  GtkWidget       *content_area;
  GtkWidget       *toolbar;
  ClayMenuModel   *toolbar_model;
  GtkWidget       *style_editor;
  GtkWidget       *scrolled_window;
  gboolean         use_header_bar;
  GtkStyleContext *style_context;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT (text), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT_BUFFER (text_buffer), NULL);

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  editor = g_object_new (CLAY_TYPE_TEXT_EDITOR,
                         "title",          title,
                         "role",           "clay-text-editor",
                         "transient-for",  parent,
                         "help-func",      clay_standard_help_func,
                         "help-id",        CLAY_HELP_TEXT_EDITOR_DIALOG,
                         "use-header-bar", use_header_bar,
                         NULL);

  gtk_dialog_add_button (GTK_DIALOG (editor),
                         _("_Close"), GTK_RESPONSE_CLOSE);

  g_signal_connect (editor, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  g_signal_connect_object (text_buffer, "changed",
                           G_CALLBACK (clay_text_editor_text_changed),
                           editor, 0);

  editor->ui_manager = clay_menu_factory_get_manager (menu_factory,
                                                      "<TextEditor>",
                                                      editor);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (editor));

  toolbar_model = clay_ui_manager_get_model (editor->ui_manager,
                                             "/text-editor-toolbar");
  toolbar = clay_toolbar_new (toolbar_model, editor->ui_manager);
  g_object_unref (toolbar_model);

  if (toolbar)
    {
      gtk_box_pack_start (GTK_BOX (content_area), toolbar, FALSE, FALSE, 0);
      gtk_widget_set_visible (toolbar, TRUE);
    }

  style_editor = clay_text_style_editor_new (clay, text, text_buffer,
                                             clay_data_factory_get_container (clay->font_factory),
                                             xres, yres);
  gtk_widget_set_visible (CLAY_TEXT_STYLE_EDITOR (style_editor)->dnd_handle,
                          FALSE);
  gtk_box_pack_start (GTK_BOX (content_area), style_editor, FALSE, FALSE, 0);
  gtk_widget_set_visible (style_editor, TRUE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_box_pack_start (GTK_BOX (content_area), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_window, TRUE);

  editor->view = gtk_text_view_new_with_buffer (GTK_TEXT_BUFFER (text_buffer));
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (editor->view),
                               GTK_WRAP_WORD_CHAR);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->view);
  gtk_widget_set_visible (editor->view, TRUE);

  style_context = gtk_widget_get_style_context (editor->view);

  editor->font_css = gtk_css_provider_new ();
  gtk_style_context_add_provider (style_context,
                                  GTK_STYLE_PROVIDER (editor->font_css),
                                  GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref (editor->font_css);

  switch (editor->base_dir)
    {
    case CLAY_TEXT_DIRECTION_LTR:
    case CLAY_TEXT_DIRECTION_TTB_RTL:
    case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
    case CLAY_TEXT_DIRECTION_TTB_LTR:
    case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_LTR);
      break;
    case CLAY_TEXT_DIRECTION_RTL:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_RTL);
      break;
    }

  gtk_widget_set_size_request (editor->view, 200, 64);

  editor->font_toggle =
    gtk_check_button_new_with_mnemonic (_("_Use selected font"));
  gtk_box_pack_start (GTK_BOX (content_area), editor->font_toggle,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->font_toggle, TRUE);

  g_signal_connect (editor->font_toggle, "toggled",
                    G_CALLBACK (clay_text_editor_font_toggled),
                    editor);

  gtk_widget_grab_focus (editor->view);

  clay_ui_manager_update (editor->ui_manager, editor);

  return GTK_WIDGET (editor);
}

void
clay_text_editor_set_text (ClayTextEditor *editor,
                           const gchar    *text,
                           gint            len)
{
  GtkTextBuffer *buffer;

  g_return_if_fail (CLAY_IS_TEXT_EDITOR (editor));
  g_return_if_fail (text != NULL || len == 0);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));

  if (text)
    gtk_text_buffer_set_text (buffer, text, len);
  else
    gtk_text_buffer_set_text (buffer, "", 0);
}

gchar *
clay_text_editor_get_text (ClayTextEditor *editor)
{
  GtkTextBuffer *buffer;

  g_return_val_if_fail (CLAY_IS_TEXT_EDITOR (editor), NULL);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));

  return clay_text_buffer_get_text (CLAY_TEXT_BUFFER (buffer));
}

void
clay_text_editor_set_direction (ClayTextEditor    *editor,
                                ClayTextDirection  base_dir)
{
  g_return_if_fail (CLAY_IS_TEXT_EDITOR (editor));

  if (editor->base_dir == base_dir)
    return;

  editor->base_dir = base_dir;

  if (editor->view)
    {
      switch (editor->base_dir)
        {
        case CLAY_TEXT_DIRECTION_LTR:
        case CLAY_TEXT_DIRECTION_TTB_RTL:
        case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
        case CLAY_TEXT_DIRECTION_TTB_LTR:
        case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_LTR);
          break;
        case CLAY_TEXT_DIRECTION_RTL:
          gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_RTL);
          break;
        }
    }

  clay_ui_manager_update (editor->ui_manager, editor);

  g_signal_emit (editor, text_editor_signals[DIR_CHANGED], 0);
}

ClayTextDirection
clay_text_editor_get_direction (ClayTextEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_TEXT_EDITOR (editor), CLAY_TEXT_DIRECTION_LTR);

  return editor->base_dir;
}

void
clay_text_editor_set_font_name (ClayTextEditor *editor,
                                const gchar    *font_name)
{
  gchar *css;

  g_return_if_fail (CLAY_IS_TEXT_EDITOR (editor));

  g_set_str (&editor->font_name, font_name);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->font_toggle)) &&
      editor->font_name)
    {
      css = g_strdup_printf ("* { font-family: \"%s\" }",
                             editor->font_name);
    }
  else
    {
      css = g_strdup ("");
    }

  gtk_css_provider_load_from_data (editor->font_css, css, -1, NULL);
  g_free (css);
}

const gchar *
clay_text_editor_get_font_name (ClayTextEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_TEXT_EDITOR (editor), NULL);

  return editor->font_name;
}


/*  private functions  */

static void
clay_text_editor_text_changed (GtkTextBuffer  *buffer,
                               ClayTextEditor *editor)
{
  g_signal_emit (editor, text_editor_signals[TEXT_CHANGED], 0);
}

static void
clay_text_editor_font_toggled (GtkToggleButton *button,
                               ClayTextEditor  *editor)
{
  gchar *css;

  if (gtk_toggle_button_get_active (button) && editor->font_name)
    {
      css = g_strdup_printf ("* { font-family: \"%s\" }",
                             editor->font_name);
    }
  else
    {
      css = g_strdup ("");
    }

  gtk_css_provider_load_from_data (editor->font_css, css, -1, NULL);
  g_free (css);
}
