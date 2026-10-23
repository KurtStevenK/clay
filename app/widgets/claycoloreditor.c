/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycoloreditor.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"
#include "libclaywidgets/claywidgets-private.h"
#include "libclayconfig/clayconfig.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-color-profile.h"

#include "display/display-types.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "claycoloreditor.h"
#include "claycolorhistory.h"
#include "claydocked.h"
#include "clayfgbgeditor.h"
#include "clayfgbgview.h"
#include "claysessioninfo-aux.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};


static void   clay_color_editor_docked_iface_init (ClayDockedInterface  *iface);

static void   clay_color_editor_constructed       (GObject           *object);
static void   clay_color_editor_dispose           (GObject           *object);
static void   clay_color_editor_set_property      (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   clay_color_editor_get_property      (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);

static void   clay_color_editor_style_updated     (GtkWidget         *widget);

static void   clay_color_editor_set_aux_info      (ClayDocked        *docked,
                                                   GList             *aux_info);
static GList *clay_color_editor_get_aux_info       (ClayDocked       *docked);
static GtkWidget *clay_color_editor_get_preview   (ClayDocked        *docked,
                                                   ClayContext       *context,
                                                   GtkIconSize        size);
static void   clay_color_editor_set_context       (ClayDocked        *docked,
                                                   ClayContext       *context);

static void   clay_color_editor_fg_changed        (ClayContext       *context,
                                                   GeglColor         *color,
                                                   ClayColorEditor   *editor);
static void   clay_color_editor_bg_changed        (ClayContext       *context,
                                                   GeglColor         *color,
                                                   ClayColorEditor   *editor);
static void   clay_color_editor_color_changed     (ClayColorSelector *selector,
                                                   GeglColor         *color,
                                                   ClayColorEditor   *editor);
static void   clay_color_editor_tab_toggled       (GtkWidget         *widget,
                                                   ClayColorEditor   *editor);
static void   clay_color_editor_fg_bg_notify      (GtkWidget         *widget,
                                                   GParamSpec        *pspec,
                                                   ClayColorEditor   *editor);
static void   clay_color_editor_color_picked      (GtkWidget         *widget,
                                                   const GeglColor   *color,
                                                   ClayColorEditor   *editor);
static void   clay_color_editor_entry_changed     (ClayColorHexEntry *entry,
                                                   ClayColorEditor   *editor);

static void  clay_color_editor_history_selected   (ClayColorHistory *history,
                                                   GeglColor        *color,
                                                   ClayColorEditor  *editor);

static void  clay_color_editor_display_changed    (ClayContext     *context,
                                                   ClayDisplay     *display,
                                                   ClayColorEditor *editor);
static void  clay_color_editor_image_changed      (ClayDisplay     *display,
                                                   GParamSpec      *pspec,
                                                   ClayColorEditor *editor);
static void  clay_color_editor_shell_changed      (ClayDisplay     *display,
                                                   GParamSpec      *pspec,
                                                   ClayColorEditor *editor);
static void  clay_color_editor_update_simulation  (ClayImage       *image,
                                                   ClayColorEditor *editor);
static void  clay_color_editor_enable_simulation  (ClayColorEditor *editor);
static void  clay_color_editor_update_format      (ClayColorEditor *editor);


G_DEFINE_TYPE_WITH_CODE (ClayColorEditor, clay_color_editor, CLAY_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_color_editor_docked_iface_init))

#define parent_class clay_color_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_color_editor_class_init (ClayColorEditorClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = clay_color_editor_constructed;
  object_class->dispose       = clay_color_editor_dispose;
  object_class->set_property  = clay_color_editor_set_property;
  object_class->get_property  = clay_color_editor_get_property;

  widget_class->style_updated = clay_color_editor_style_updated;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        G_PARAM_CONSTRUCT |
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_color_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->get_preview  = clay_color_editor_get_preview;
  iface->set_aux_info = clay_color_editor_set_aux_info;
  iface->get_aux_info = clay_color_editor_get_aux_info;
  iface->set_context  = clay_color_editor_set_context;
}

static void
clay_color_editor_init (ClayColorEditor *editor)
{
  GtkWidget   *notebook;
  GtkWidget   *hbox;
  GtkWidget   *button;
  gint         content_spacing;
  gint         button_spacing;
  GtkIconSize  button_icon_size;
  GeglColor   *color;
  GList       *list;
  GSList      *group;
  gint         icon_width  = 40;
  gint         icon_height = 38;

  editor->context = NULL;
  editor->edit_bg = FALSE;

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content-spacing",  &content_spacing,
                        "button-spacing",   &button_spacing,
                        "button-icon-size", &button_icon_size,
                        NULL);

  editor->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
  gtk_box_set_homogeneous (GTK_BOX (editor->hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (editor), editor->hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->hbox, TRUE);

  color = gegl_color_new ("black");
  editor->notebook = clay_color_selector_new (CLAY_TYPE_COLOR_NOTEBOOK, color,
                                              CLAY_COLOR_SELECTOR_HUE);
  g_object_unref (color);
  clay_color_selector_set_show_alpha (CLAY_COLOR_SELECTOR (editor->notebook),
                                      FALSE);
  gtk_box_pack_start (GTK_BOX (editor), editor->notebook,
                      TRUE, TRUE, content_spacing);
  gtk_widget_set_visible (editor->notebook, TRUE);

  g_signal_connect (editor->notebook, "color-changed",
                    G_CALLBACK (clay_color_editor_color_changed),
                    editor);

  notebook = clay_color_notebook_get_notebook (CLAY_COLOR_NOTEBOOK (editor->notebook));

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  clay_color_notebook_set_has_page (CLAY_COLOR_NOTEBOOK (editor->notebook),
                                    CLAY_TYPE_COLOR_SCALES, TRUE);

  group = NULL;

  for (list = clay_color_notebook_get_selectors (CLAY_COLOR_NOTEBOOK (editor->notebook));
       list;
       list = g_list_next (list))
    {
      ClayColorSelector      *selector;
      ClayColorSelectorClass *selector_class;
      GtkWidget              *button;
      GtkWidget              *image;

      selector       = CLAY_COLOR_SELECTOR (list->data);
      selector_class = CLAY_COLOR_SELECTOR_GET_CLASS (selector);

      button = gtk_radio_button_new (group);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_box_pack_start (GTK_BOX (editor->hbox), button, TRUE, TRUE, 0);
      gtk_widget_set_visible (button, TRUE);

      image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_set_visible (image, TRUE);

      clay_help_set_help_data (button,
                               selector_class->name, selector_class->help_id);
      clay_widget_set_identifier (button, selector_class->name);

      g_object_set_data (G_OBJECT (button),   "selector", selector);
      g_object_set_data (G_OBJECT (selector), "button",   button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (clay_color_editor_tab_toggled),
                        editor);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /*  FG/BG editor  */
  editor->fg_bg = clay_fg_bg_editor_new (NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_width, &icon_height);
  gtk_widget_set_size_request (editor->fg_bg,
                               (gint) (icon_width * 1.75),
                               (gint) (icon_height * 1.75));
  gtk_box_pack_start (GTK_BOX (hbox), editor->fg_bg, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->fg_bg, TRUE);

  g_signal_connect (editor->fg_bg, "notify::active-color",
                    G_CALLBACK (clay_color_editor_fg_bg_notify),
                    editor);

  /*  The color picker  */
  button = clay_pick_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "color-picked",
                    G_CALLBACK (clay_color_editor_color_picked),
                    editor);

  /*  The hex triplet entry  */
  editor->hex_entry = clay_color_hex_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), editor->hex_entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (editor->hex_entry, TRUE);

  g_signal_connect (editor->hex_entry, "color-changed",
                    G_CALLBACK (clay_color_editor_entry_changed),
                    editor);
}

static void
clay_color_editor_constructed (GObject *object)
{
  ClayColorEditor *editor       = CLAY_COLOR_EDITOR (object);
  ClayContext     *user_context = editor->context->clay->user_context;
  ClayDisplay     *display      = clay_context_get_display (user_context);
  GtkWidget       *history;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* The color history */
  history = clay_color_history_new (editor->context, 12);
  gtk_box_pack_end (GTK_BOX (editor), history, FALSE, FALSE, 0);
  gtk_widget_set_visible (history, TRUE);

  g_signal_connect (history, "color-selected",
                    G_CALLBACK (clay_color_editor_history_selected),
                    editor);

  g_signal_connect_object (user_context, "display-changed",
                           G_CALLBACK (clay_color_editor_display_changed),
                           editor, 0);

  clay_color_editor_display_changed (user_context, display, editor);
}

static void
clay_color_editor_dispose (GObject *object)
{
  ClayColorEditor *editor = CLAY_COLOR_EDITOR (object);

  g_clear_weak_pointer (&editor->active_display);
  g_clear_weak_pointer (&editor->active_image);
  g_clear_weak_pointer (&editor->active_shell);

  if (editor->context)
    clay_docked_set_context (CLAY_DOCKED (editor), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_color_editor_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_CONTEXT:
      clay_docked_set_context (CLAY_DOCKED (object),
                               g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_editor_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayColorEditor *editor = CLAY_COLOR_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
clay_color_editor_get_preview (ClayDocked  *docked,
                               ClayContext *context,
                               GtkIconSize  size)
{
  GtkWidget *preview;
  gint       width;
  gint       height;

  preview = clay_fg_bg_view_new (context);

  if (gtk_icon_size_lookup (size, &width, &height))
    gtk_widget_set_size_request (preview, width, height);

  return preview;
}

#define AUX_INFO_CURRENT_PAGE "current-page"

static void
clay_color_editor_set_aux_info (ClayDocked *docked,
                                GList      *aux_info)
{
  ClayColorEditor *editor   = CLAY_COLOR_EDITOR (docked);
  GtkWidget       *notebook;
  GList           *list;

  notebook = clay_color_notebook_get_notebook (CLAY_COLOR_NOTEBOOK (editor->notebook));

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_CURRENT_PAGE))
        {
          GList *children;
          GList *child;

          children = gtk_container_get_children (GTK_CONTAINER (notebook));

          for (child = children; child; child = g_list_next (child))
            {
              if (! strcmp (G_OBJECT_TYPE_NAME (child->data), aux->value))
                {
                  GtkWidget *button;

                  button = g_object_get_data (G_OBJECT (child->data), "button");

                  if (button)
                    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                                  TRUE);

                  break;
                }
            }

          g_list_free (children);
        }
    }
}

static GList *
clay_color_editor_get_aux_info (ClayDocked *docked)
{
  ClayColorEditor    *editor   = CLAY_COLOR_EDITOR (docked);
  ClayColorNotebook  *notebook = CLAY_COLOR_NOTEBOOK (editor->notebook);
  ClayColorSelector  *current;
  GList              *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  current = clay_color_notebook_get_current_selector (notebook);

  if (current)
    {
      ClaySessionInfoAux *aux;

      aux = clay_session_info_aux_new (AUX_INFO_CURRENT_PAGE,
                                       G_OBJECT_TYPE_NAME (current));
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
clay_color_editor_set_context (ClayDocked  *docked,
                               ClayContext *context)
{
  ClayColorEditor *editor = CLAY_COLOR_EDITOR (docked);

  if (context == editor->context)
    return;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_color_editor_fg_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_color_editor_bg_changed,
                                            editor);

      g_signal_handlers_disconnect_by_func (editor->context->clay->config,
                                            G_CALLBACK (clay_color_editor_style_updated),
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (editor->context)
    {
      GeglColor *color;

      g_object_ref (editor->context);

      g_signal_connect_object (editor->context, "foreground-changed",
                               G_CALLBACK (clay_color_editor_fg_changed),
                               editor, 0);
      g_signal_connect_object (editor->context, "background-changed",
                               G_CALLBACK (clay_color_editor_bg_changed),
                               editor, 0);

      g_signal_connect_object (editor->context->clay->config,
                               "notify::theme",
                               G_CALLBACK (clay_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->clay->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (clay_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->clay->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (clay_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      if (editor->edit_bg)
        {
          color = clay_context_get_background (editor->context);
          clay_color_editor_bg_changed (editor->context, color, editor);
        }
      else
        {
          color = clay_context_get_foreground (editor->context);
          clay_color_editor_fg_changed (editor->context, color, editor);
        }

      g_object_set_data (G_OBJECT (context->clay->config->color_management),
                         "clay-context", editor->context);

      clay_color_selector_set_config (CLAY_COLOR_SELECTOR (editor->notebook),
                                      context->clay->config->color_management);

      g_object_set_data (G_OBJECT (context->clay->config->color_management),
                         "clay-context", NULL);
    }

  clay_fg_bg_editor_set_context (CLAY_FG_BG_EDITOR (editor->fg_bg), context);
}

GtkWidget *
clay_color_editor_new (ClayContext *context)
{
  return g_object_new (CLAY_TYPE_COLOR_EDITOR,
                       "context", context,
                       NULL);
}

static void
clay_color_editor_style_updated (GtkWidget *widget)
{
  ClayColorEditor   *editor   = CLAY_COLOR_EDITOR (widget);
  ClayColorNotebook *notebook = CLAY_COLOR_NOTEBOOK (editor->notebook);
  GtkIconSize        button_icon_size;
  gint               icon_width  = 40;
  gint               icon_height = 38;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (editor->hbox)
    clay_editor_set_box_style (CLAY_EDITOR (editor), GTK_BOX (editor->hbox));

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "button-icon-size", &button_icon_size,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_width, &icon_height);
  gtk_widget_set_size_request (editor->fg_bg,
                               (gint) (icon_width * 1.75),
                               (gint) (icon_height * 1.75));

  GTK_WIDGET_GET_CLASS (notebook)->style_updated (GTK_WIDGET (notebook));
}


static void
clay_color_editor_set_color (ClayColorEditor *editor,
                             GeglColor       *color)
{
  g_signal_handlers_block_by_func (editor->notebook,
                                   clay_color_editor_color_changed,
                                   editor);

  clay_color_selector_set_color (CLAY_COLOR_SELECTOR (editor->notebook), color);

  g_signal_handlers_unblock_by_func (editor->notebook,
                                     clay_color_editor_color_changed,
                                     editor);

  g_signal_handlers_block_by_func (editor->hex_entry,
                                   clay_color_editor_entry_changed,
                                   editor);

  clay_color_hex_entry_set_color (CLAY_COLOR_HEX_ENTRY (editor->hex_entry), color);

  g_signal_handlers_unblock_by_func (editor->hex_entry,
                                     clay_color_editor_entry_changed,
                                     editor);
}

static void
clay_color_editor_fg_changed (ClayContext     *context,
                              GeglColor       *color,
                              ClayColorEditor *editor)
{
  if (! editor->edit_bg)
    clay_color_editor_set_color (editor, color);
}

static void
clay_color_editor_bg_changed (ClayContext     *context,
                              GeglColor       *color,
                              ClayColorEditor *editor)
{
  if (editor->edit_bg)
    clay_color_editor_set_color (editor, color);
}

static void
clay_color_editor_color_changed (ClayColorSelector *selector,
                                 GeglColor         *color,
                                 ClayColorEditor   *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        {
          g_signal_handlers_block_by_func (editor->context,
                                           clay_color_editor_bg_changed,
                                           editor);

          clay_context_set_background (editor->context, color);

          g_signal_handlers_unblock_by_func (editor->context,
                                             clay_color_editor_bg_changed,
                                             editor);
        }
      else
        {
          g_signal_handlers_block_by_func (editor->context,
                                           clay_color_editor_fg_changed,
                                           editor);

          clay_context_set_foreground (editor->context, color);

          g_signal_handlers_unblock_by_func (editor->context,
                                             clay_color_editor_fg_changed,
                                             editor);
        }
    }

  g_signal_handlers_block_by_func (editor->hex_entry,
                                   clay_color_editor_entry_changed,
                                   editor);

  clay_color_hex_entry_set_color (CLAY_COLOR_HEX_ENTRY (editor->hex_entry), color);

  g_signal_handlers_unblock_by_func (editor->hex_entry,
                                     clay_color_editor_entry_changed,
                                     editor);
}

static void
clay_color_editor_tab_toggled (GtkWidget       *widget,
                               ClayColorEditor *editor)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GtkWidget *selector;

      selector = g_object_get_data (G_OBJECT (widget), "selector");

      if (selector)
        {
          GtkWidget *notebook;
          gint       page_num;

          notebook = clay_color_notebook_get_notebook (CLAY_COLOR_NOTEBOOK (editor->notebook));

          page_num = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), selector);

          if (page_num >= 0)
            gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);
        }
    }
}

static void
clay_color_editor_fg_bg_notify (GtkWidget       *widget,
                                GParamSpec      *pspec,
                                ClayColorEditor *editor)
{
  gboolean edit_bg;

  edit_bg = (CLAY_FG_BG_EDITOR (widget)->active_color ==
             CLAY_ACTIVE_COLOR_BACKGROUND);

  if (edit_bg != editor->edit_bg)
    {
      editor->edit_bg = edit_bg;

      if (editor->context)
        {
          GeglColor *color;

          if (edit_bg)
            {
              color = clay_context_get_background (editor->context);
              clay_color_editor_bg_changed (editor->context, color, editor);
            }
          else
            {
              color = clay_context_get_foreground (editor->context);
              clay_color_editor_fg_changed (editor->context, color, editor);
            }
        }
    }
}

static void
clay_color_editor_color_picked (GtkWidget       *widget,
                                const GeglColor *color,
                                ClayColorEditor *editor)
{
  if (editor->context && color)
    {
      if (editor->edit_bg)
        clay_context_set_background (editor->context, GEGL_COLOR (color));
      else
        clay_context_set_foreground (editor->context, GEGL_COLOR (color));
    }
}

static void
clay_color_editor_entry_changed (ClayColorHexEntry *entry,
                                 ClayColorEditor   *editor)
{
  GeglColor *color;

  color = clay_color_hex_entry_get_color (entry);

  if (editor->context)
    {
      if (editor->edit_bg)
        clay_context_set_background (editor->context, color);
      else
        clay_context_set_foreground (editor->context, color);
    }

  g_object_unref (color);
}

static void
clay_color_editor_history_selected (ClayColorHistory *history,
                                    GeglColor        *color,
                                    ClayColorEditor  *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        clay_context_set_background (editor->context, color);
      else
        clay_context_set_foreground (editor->context, color);
    }
}

static void
clay_color_editor_display_changed (ClayContext     *context,
                                   ClayDisplay     *display,
                                   ClayColorEditor *editor)
{
  if (editor->active_display != display)
    {
      if (editor->active_display)
        {
          g_signal_handlers_disconnect_by_func (editor->active_display,
                                                clay_color_editor_image_changed,
                                                editor);
          g_signal_handlers_disconnect_by_func (editor->active_display,
                                                clay_color_editor_shell_changed,
                                                editor);
        }

      g_set_weak_pointer (&editor->active_display, display);

      if (display)
        {
          g_signal_connect_object (display, "notify::image",
                                   G_CALLBACK (clay_color_editor_image_changed),
                                   editor, 0);
          g_signal_connect_object (display, "notify::shell",
                                   G_CALLBACK (clay_color_editor_shell_changed),
                                   editor, 0);
        }

      clay_color_editor_image_changed (display, NULL, editor);
      clay_color_editor_shell_changed (display, NULL, editor);
    }
}

static void
clay_color_editor_image_changed (ClayDisplay     *display,
                                 GParamSpec      *pspec,
                                 ClayColorEditor *editor)
{
  ClayImage *image = display ? clay_display_get_image (display) : NULL;

  if (editor->active_image != image)
    {
      if (editor->active_image)
        {
          g_signal_handlers_disconnect_by_func (editor->active_image,
                                                clay_color_editor_update_simulation,
                                                editor);
          g_signal_handlers_disconnect_by_func (editor->active_image,
                                                clay_color_editor_update_format,
                                                editor);
        }

      g_set_weak_pointer (&editor->active_image, image);

      if (image)
        {
          g_signal_connect_object (image, "simulation-profile-changed",
                                   G_CALLBACK (clay_color_editor_update_simulation),
                                   editor, 0);
          g_signal_connect_object (image, "simulation-intent-changed",
                                   G_CALLBACK (clay_color_editor_update_simulation),
                                   editor, 0);
          g_signal_connect_object (image, "simulation-bpc-changed",
                                   G_CALLBACK (clay_color_editor_update_simulation),
                                   editor, 0);

          g_signal_connect_object (image, "profile-changed",
                                   G_CALLBACK (clay_color_editor_update_format),
                                   editor, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "precision-changed",
                                   G_CALLBACK (clay_color_editor_update_format),
                                   editor, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "notify::base-type",
                                   G_CALLBACK (clay_color_editor_update_format),
                                   editor, G_CONNECT_SWAPPED);
        }

      clay_color_editor_update_simulation (image, editor);
      clay_color_editor_update_format (editor);
    }
}

static void
clay_color_editor_shell_changed (ClayDisplay     *display,
                                 GParamSpec      *pspec,
                                 ClayColorEditor *editor)
{
  ClayDisplayShell *shell = display ? clay_display_get_shell (display) : NULL;

  if (editor->active_shell != shell)
    {
      ClayColorConfig *config;

      if (editor->active_shell)
        {
          config = clay_display_shell_get_color_config (editor->active_shell);

          g_signal_handlers_disconnect_by_func (config,
                                                clay_color_editor_enable_simulation,
                                                editor);
        }

      g_set_weak_pointer (&editor->active_shell, shell);

      if (shell)
        {
          config = clay_display_shell_get_color_config (shell);

          g_signal_connect_object (config, "notify::mode",
                                   G_CALLBACK (clay_color_editor_enable_simulation),
                                   editor, G_CONNECT_SWAPPED);
        }

      clay_color_editor_enable_simulation (editor);
    }
}

static void
clay_color_editor_update_simulation (ClayImage       *image,
                                     ClayColorEditor *editor)
{
  g_return_if_fail (CLAY_IS_COLOR_EDITOR (editor));

  if (image)
    clay_color_notebook_set_simulation (CLAY_COLOR_NOTEBOOK (editor->notebook),
                                        clay_image_get_simulation_profile (image),
                                        clay_image_get_simulation_intent (image),
                                        clay_image_get_simulation_bpc (image));
  else
    clay_color_notebook_set_simulation (CLAY_COLOR_NOTEBOOK (editor->notebook),
                                        NULL,
                                        CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                        FALSE);
}

static void
clay_color_editor_enable_simulation (ClayColorEditor *editor)
{
  g_return_if_fail (CLAY_IS_COLOR_EDITOR (editor));

  if (editor->active_shell && editor->active_image)
    {
      ClayColorConfig         *config;
      ClayColorManagementMode  mode;

      config = clay_display_shell_get_color_config (editor->active_shell);
      mode   = clay_color_config_get_mode (config);
      clay_color_notebook_enable_simulation (CLAY_COLOR_NOTEBOOK (editor->notebook),
                                             (mode == CLAY_COLOR_MANAGEMENT_SOFTPROOF));
    }
  else
    {
      clay_color_notebook_enable_simulation (CLAY_COLOR_NOTEBOOK (editor->notebook), FALSE);
    }
}

static void
clay_color_editor_update_format (ClayColorEditor *editor)
{
  g_return_if_fail (CLAY_IS_COLOR_EDITOR (editor));

  if (editor->active_image)
    clay_color_notebook_set_format (CLAY_COLOR_NOTEBOOK (editor->notebook),
                                    clay_image_get_layer_format (editor->active_image, FALSE));
  else
    clay_color_notebook_set_format (CLAY_COLOR_NOTEBOOK (editor->notebook), NULL);
}
