/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolgui.c
 * Copyright (C) 2013  Michael Natterer <mitch@gimp.org>
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

#include "display-types.h"

#include "core/claycontext.h"
#include "core/claytoolinfo.h"

#include "widgets/claydialogfactory.h"
#include "widgets/clayoverlaybox.h"
#include "widgets/clayoverlaydialog.h"
#include "widgets/claywidgets-utils.h"

#include "claydisplayshell.h"
#include "claytooldialog.h"
#include "claytoolgui.h"


enum
{
  RESPONSE,
  LAST_SIGNAL
};


typedef struct _ResponseEntry ResponseEntry;

struct _ResponseEntry
{
  gint      response_id;
  gchar    *button_text;
  gint      alternative_position;
  gboolean  sensitive;
};

typedef struct _ClayToolGuiPrivate ClayToolGuiPrivate;

struct _ClayToolGuiPrivate
{
  ClayToolInfo     *tool_info;
  gchar            *title;
  gchar            *description;
  gchar            *icon_name;
  gchar            *help_id;
  GList            *response_entries;
  gint              default_response;
  gboolean          focus_on_map;

  gboolean          overlay;
  gboolean          auto_overlay;

  ClayDisplayShell *shell;
  GList            *viewables;

  GtkWidget        *dialog;
  GtkWidget        *vbox;
};

#define GET_PRIVATE(gui) ((ClayToolGuiPrivate *) clay_tool_gui_get_instance_private ((ClayToolGui *) (gui)))


static void            clay_tool_gui_dispose           (GObject       *object);
static void            clay_tool_gui_finalize          (GObject       *object);

static void            clay_tool_gui_create_dialog     (ClayToolGui   *gui,
                                                        GdkMonitor    *monitor);
static void            clay_tool_gui_add_dialog_button (ClayToolGui   *gui,
                                                        ResponseEntry *entry);
static void            clay_tool_gui_update_buttons    (ClayToolGui   *gui);
static void            clay_tool_gui_update_shell      (ClayToolGui   *gui);
static void            clay_tool_gui_update_viewable   (ClayToolGui   *gui);

static void            clay_tool_gui_dialog_response   (GtkWidget     *dialog,
                                                        gint           response_id,
                                                        ClayToolGui   *gui);
static void            clay_tool_gui_canvas_resized    (GtkWidget     *canvas,
                                                        GtkAllocation *allocation,
                                                        ClayToolGui   *gui);

static ResponseEntry * response_entry_new              (gint           response_id,
                                                        const gchar   *button_text);
static void            response_entry_free             (ResponseEntry *entry);
static ResponseEntry * response_entry_find             (GList         *entries,
                                                        gint           response_id);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolGui, clay_tool_gui, CLAY_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0, };

#define parent_class clay_tool_gui_parent_class


static void
clay_tool_gui_class_init (ClayToolGuiClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = clay_tool_gui_dispose;
  object_class->finalize = clay_tool_gui_finalize;

  signals[RESPONSE] =
    g_signal_new ("response",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayToolGuiClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);
}

static void
clay_tool_gui_init (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);

  private->default_response = -1;
  private->focus_on_map     = TRUE;

  private->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  g_object_ref_sink (private->vbox);
}

static void
clay_tool_gui_dispose (GObject *object)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->tool_info);

  if (private->shell)
    clay_tool_gui_set_shell (CLAY_TOOL_GUI (object), NULL);

  if (private->viewables)
    clay_tool_gui_set_viewables (CLAY_TOOL_GUI (object), NULL);

  g_clear_object (&private->vbox);

  if (private->dialog)
    {
      if (gtk_widget_get_visible (private->dialog))
        clay_tool_gui_hide (CLAY_TOOL_GUI (object));

      if (private->overlay)
        g_object_unref (private->dialog);
      else
        gtk_widget_destroy (private->dialog);

      private->dialog = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_tool_gui_finalize (GObject *object)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->title,       g_free);
  g_clear_pointer (&private->description, g_free);
  g_clear_pointer (&private->icon_name,   g_free);
  g_clear_pointer (&private->help_id,     g_free);

  if (private->response_entries)
    {
      g_list_free_full (private->response_entries,
                        (GDestroyNotify) response_entry_free);
      private->response_entries = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * clay_tool_gui_new: (skip)
 * @tool_info:   a #ClayToolInfo
 * @description: a string to use in the gui header or %NULL to use the help
 *               field from #ClayToolInfo
 * @...:         a %NULL-terminated valist of button parameters as described in
 *               gtk_gui_new_with_buttons().
 *
 * This function creates a #ClayToolGui using the information stored
 * in @tool_info.
 *
 * Returns: a new #ClayToolGui
 **/
ClayToolGui *
clay_tool_gui_new (ClayToolInfo *tool_info,
                   const gchar  *title,
                   const gchar  *description,
                   const gchar  *icon_name,
                   const gchar  *help_id,
                   GdkMonitor   *monitor,
                   gboolean      overlay,
                   ...)
{
  ClayToolGui        *gui;
  ClayToolGuiPrivate *private;
  va_list             args;

  g_return_val_if_fail (CLAY_IS_TOOL_INFO (tool_info), NULL);

  gui = g_object_new (CLAY_TYPE_TOOL_GUI, NULL);

  private = GET_PRIVATE (gui);

  if (! title)
    title = tool_info->label;

  if (! description)
    description = tool_info->label;

  if (! icon_name)
    icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool_info));

  if (! help_id)
    help_id = tool_info->help_id;

  private->tool_info   = g_object_ref (tool_info);
  private->title       = g_strdup (title);
  private->description = g_strdup (description);
  private->icon_name   = g_strdup (icon_name);
  private->help_id     = g_strdup (help_id);
  private->overlay     = overlay;

  va_start (args, overlay);

  clay_tool_gui_add_buttons_valist (gui, args);

  va_end (args);

  clay_tool_gui_create_dialog (gui, monitor);

  return gui;
}

void
clay_tool_gui_set_title (ClayToolGui *gui,
                         const gchar *title)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (title == private->title)
    return;

  g_set_str (&private->title, title);

  if (! title)
    title = private->tool_info->label;

  g_object_set (private->dialog, "title", title, NULL);
}

void
clay_tool_gui_set_description (ClayToolGui *gui,
                               const gchar *description)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (description == private->description)
    return;

  g_set_str (&private->description, description);

  if (! description)
    description = private->tool_info->tooltip;

  if (private->overlay)
    {
      /* TODO */
    }
  else
    {
      g_object_set (private->dialog, "description", description, NULL);
    }
}

void
clay_tool_gui_set_icon_name (ClayToolGui *gui,
                             const gchar *icon_name)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (icon_name == private->icon_name)
    return;

  g_set_str (&private->icon_name, icon_name);

  if (! icon_name)
    icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (private->tool_info));

  g_object_set (private->dialog, "icon-name", icon_name, NULL);
}

void
clay_tool_gui_set_help_id (ClayToolGui *gui,
                           const gchar *help_id)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (help_id == private->help_id)
    return;

  g_set_str (&private->help_id, help_id);

  if (! help_id)
    help_id = private->tool_info->help_id;

  if (private->overlay)
    {
      /* TODO */
    }
  else
    {
      g_object_set (private->dialog, "help-id", help_id, NULL);
    }
}

void
clay_tool_gui_set_shell (ClayToolGui      *gui,
                         ClayDisplayShell *shell)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));
  g_return_if_fail (shell == NULL || CLAY_IS_DISPLAY_SHELL (shell));

  private = GET_PRIVATE (gui);

  if (shell == private->shell)
    return;

  if (private->shell)
    g_signal_handlers_disconnect_by_func (private->shell->canvas,
                                          clay_tool_gui_canvas_resized,
                                          gui);

  g_set_weak_pointer (&private->shell, shell);

  if (private->shell)
    g_signal_connect (private->shell->canvas, "size-allocate",
                      G_CALLBACK (clay_tool_gui_canvas_resized),
                      gui);

  clay_tool_gui_update_shell (gui);
}

void
clay_tool_gui_set_viewables (ClayToolGui *gui,
                             GList       *viewables)
{
  ClayToolGuiPrivate *private;
  GList              *iter;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (g_list_length (viewables) == g_list_length (private->viewables))
    {
      for (iter = private->viewables; iter; iter = iter->next)
        {
          g_return_if_fail (iter->data == NULL || CLAY_IS_VIEWABLE (iter->data));

          if (! g_list_find (private->viewables, iter->data))
            break;
        }

      if (iter == NULL)
        /* Identical viewable list. */
        return;
    }

  if (private->viewables)
    {
      for (iter = private->viewables; iter; iter = iter->next)
        g_clear_weak_pointer (&iter->data);

      g_list_free (private->viewables);
    }

  private->viewables = g_list_copy (viewables);

  if (private->viewables)
    {
      for (iter = private->viewables; iter; iter = iter->next)
        /* NOT g_set_weak_pointer() because the pointer is alrerady set */
        g_object_add_weak_pointer (G_OBJECT (iter->data),
                                   (gpointer) &iter->data);
    }

  clay_tool_gui_update_viewable (gui);
}

void
clay_tool_gui_set_viewable (ClayToolGui  *gui,
                            ClayViewable *viewable)
{
  GList *viewables = g_list_prepend (NULL, viewable);

  clay_tool_gui_set_viewables (gui, viewables);
  g_list_free (viewables);
}

GtkWidget *
clay_tool_gui_get_dialog (ClayToolGui *gui)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GUI (gui), NULL);

  return GET_PRIVATE (gui)->dialog;
}

GtkWidget *
clay_tool_gui_get_vbox (ClayToolGui *gui)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GUI (gui), NULL);

  return GET_PRIVATE (gui)->vbox;
}

gboolean
clay_tool_gui_get_visible (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private;

  g_return_val_if_fail (CLAY_IS_TOOL_GUI (gui), FALSE);

  private = GET_PRIVATE (gui);

  if (private->overlay)
    return gtk_widget_get_parent (private->dialog) != NULL;
  else
    return gtk_widget_get_visible (private->dialog);
}

void
clay_tool_gui_show (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  g_return_if_fail (private->shell != NULL);

  if (private->overlay)
    {
      if (! gtk_widget_get_parent (private->dialog))
        {
          clay_overlay_box_add_child (CLAY_OVERLAY_BOX (private->shell->canvas),
                                      private->dialog, 1.0, 0.0);
          gtk_widget_set_visible (private->dialog, TRUE);
        }
    }
  else
    {
      if (gtk_widget_get_visible (private->dialog))
        gdk_window_show (gtk_widget_get_window (private->dialog));
      else
        gtk_widget_set_visible (private->dialog, TRUE);
    }
}

void
clay_tool_gui_hide (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->overlay)
    {
      if (gtk_widget_get_parent (private->dialog))
        {
          gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (private->dialog)),
                                private->dialog);
          gtk_widget_set_visible (private->dialog, FALSE);
        }
    }
  else
    {
      if (clay_dialog_factory_from_widget (private->dialog, NULL))
        {
          clay_dialog_factory_hide_dialog (private->dialog);
        }
      else
        {
          gtk_widget_set_visible (private->dialog, FALSE);
        }
    }
}

void
clay_tool_gui_set_overlay (ClayToolGui *gui,
                           GdkMonitor  *monitor,
                           gboolean     overlay)
{
  ClayToolGuiPrivate *private;
  gboolean            visible;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->overlay == overlay)
    return;

  if (! private->dialog)
    {
      private->overlay = overlay;
      return;
    }

  visible = gtk_widget_get_visible (private->dialog);

  if (visible)
    clay_tool_gui_hide (gui);

  gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (private->vbox)),
                        private->vbox);

  if (private->overlay)
    g_object_unref (private->dialog);
  else
    gtk_widget_destroy (private->dialog);

  private->overlay = overlay;

  clay_tool_gui_create_dialog (gui, monitor);

  if (visible)
    clay_tool_gui_show (gui);
}

gboolean
clay_tool_gui_get_overlay (ClayToolGui *gui)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GUI (gui), FALSE);

  return GET_PRIVATE (gui)->overlay;
}

void
clay_tool_gui_set_auto_overlay (ClayToolGui *gui,
                                gboolean     auto_overlay)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->auto_overlay != auto_overlay)
    {
      private->auto_overlay = auto_overlay;

      if (private->shell)
        clay_tool_gui_canvas_resized (private->shell->canvas, NULL, gui);
    }
}

gboolean
clay_tool_gui_get_auto_overlay (ClayToolGui *gui)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GUI (gui), FALSE);

  return GET_PRIVATE (gui)->auto_overlay;
}

void
clay_tool_gui_set_focus_on_map (ClayToolGui *gui,
                                gboolean     focus_on_map)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  if (private->focus_on_map == focus_on_map)
    return;

  private->focus_on_map = focus_on_map ? TRUE : FALSE;

  if (! private->overlay)
    {
      gtk_window_set_focus_on_map (GTK_WINDOW (private->dialog),
                                   private->focus_on_map);
    }
}

gboolean
clay_tool_gui_get_focus_on_map (ClayToolGui *gui)
{
  g_return_val_if_fail (CLAY_IS_TOOL_GUI (gui), FALSE);

  return GET_PRIVATE (gui)->focus_on_map;
}

void
clay_tool_gui_add_buttons_valist (ClayToolGui *gui,
                                  va_list      args)
{
  const gchar *button_text;
  gint         response_id;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  while ((button_text = va_arg (args, const gchar *)))
    {
      response_id = va_arg (args, gint);

      clay_tool_gui_add_button (gui, button_text, response_id);
    }
}

void
clay_tool_gui_add_button (ClayToolGui *gui,
                          const gchar *button_text,
                          gint         response_id)
{
  ClayToolGuiPrivate *private;
  ResponseEntry     *entry;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));
  g_return_if_fail (button_text != NULL);

  private = GET_PRIVATE (gui);

  entry = response_entry_new (response_id, button_text);

  private->response_entries = g_list_append (private->response_entries,
                                             entry);

  if (private->dialog)
    clay_tool_gui_add_dialog_button (gui, entry);
}

void
clay_tool_gui_set_default_response (ClayToolGui *gui,
                                    gint         response_id)
{
  ClayToolGuiPrivate *private;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  g_return_if_fail (response_entry_find (private->response_entries,
                                         response_id) != NULL);

  private->default_response = response_id;

  if (private->overlay)
    {
      clay_overlay_dialog_set_default_response (CLAY_OVERLAY_DIALOG (private->dialog),
                                                response_id);
    }
  else
    {
      gtk_dialog_set_default_response (GTK_DIALOG (private->dialog),
                                       response_id);
    }
}

void
clay_tool_gui_set_response_sensitive (ClayToolGui *gui,
                                      gint         response_id,
                                      gboolean     sensitive)
{
  ClayToolGuiPrivate *private;
  ResponseEntry      *entry;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  entry = response_entry_find (private->response_entries, response_id);

  if (! entry)
    return;

  entry->sensitive = sensitive;

  if (private->overlay)
    {
      clay_overlay_dialog_set_response_sensitive (CLAY_OVERLAY_DIALOG (private->dialog),
                                                  response_id, sensitive);
    }
  else
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (private->dialog),
                                         response_id, sensitive);
    }
}

void
clay_tool_gui_set_alternative_button_order (ClayToolGui *gui,
                                            ...)
{
  ClayToolGuiPrivate *private;
  va_list             args;
  gint                response_id;
  gint                i;

  g_return_if_fail (CLAY_IS_TOOL_GUI (gui));

  private = GET_PRIVATE (gui);

  va_start (args, gui);

  for (response_id = va_arg (args, gint), i = 0;
       response_id != -1;
       response_id = va_arg (args, gint), i++)
    {
      ResponseEntry *entry = response_entry_find (private->response_entries,
                                                  response_id);

      if (entry)
        entry->alternative_position = i;
    }

  va_end (args);

  clay_tool_gui_update_buttons (gui);
}


/*  private functions  */

static void
clay_tool_gui_create_dialog (ClayToolGui *gui,
                             GdkMonitor  *monitor)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);
  GList              *list;

  if (private->overlay)
    {
      private->dialog = clay_overlay_dialog_new (private->tool_info,
                                                 private->description,
                                                 NULL);
      g_object_ref_sink (private->dialog);

      for (list = private->response_entries; list; list = g_list_next (list))
        {
          ResponseEntry *entry = list->data;

          clay_tool_gui_add_dialog_button (gui, entry);
        }

      if (private->default_response != -1)
        clay_overlay_dialog_set_default_response (CLAY_OVERLAY_DIALOG (private->dialog),
                                                  private->default_response);

      gtk_container_set_border_width (GTK_CONTAINER (private->dialog), 6);

      gtk_container_set_border_width (GTK_CONTAINER (private->vbox), 0);
      gtk_container_add (GTK_CONTAINER (private->dialog), private->vbox);
      gtk_widget_set_visible (private->vbox, TRUE);
    }
  else
    {
      private->dialog = clay_tool_dialog_new (private->tool_info,
                                              monitor,
                                              private->title,
                                              private->description,
                                              private->icon_name,
                                              private->help_id,
                                              NULL);

      for (list = private->response_entries; list; list = g_list_next (list))
        {
          ResponseEntry *entry = list->data;

          clay_tool_gui_add_dialog_button (gui, entry);
        }

      if (private->default_response != -1)
        gtk_dialog_set_default_response (GTK_DIALOG (private->dialog),
                                         private->default_response);

      gtk_window_set_focus_on_map (GTK_WINDOW (private->dialog),
                                   private->focus_on_map);

      gtk_container_set_border_width (GTK_CONTAINER (private->vbox), 6);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (private->dialog))),
                          private->vbox, TRUE, TRUE, 0);
      gtk_widget_set_visible (private->vbox, TRUE);
    }

  clay_tool_gui_update_buttons (gui);

  if (private->shell)
    clay_tool_gui_update_shell (gui);

  if (private->viewables)
    clay_tool_gui_update_viewable (gui);

  g_signal_connect_object (private->dialog, "response",
                           G_CALLBACK (clay_tool_gui_dialog_response),
                           G_OBJECT (gui), 0);
}

static void
clay_tool_gui_add_dialog_button (ClayToolGui   *gui,
                                 ResponseEntry *entry)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);

  if (private->overlay)
    {
      clay_overlay_dialog_add_button (CLAY_OVERLAY_DIALOG (private->dialog),
                                      entry->button_text,
                                      entry->response_id);

      if (! entry->sensitive)
        {
          clay_overlay_dialog_set_response_sensitive (
            CLAY_OVERLAY_DIALOG (private->dialog),
            entry->response_id, FALSE);
        }
    }
  else
    {
      clay_dialog_add_button (CLAY_DIALOG (private->dialog),
                              entry->button_text,
                              entry->response_id);

      if (! entry->sensitive)
        gtk_dialog_set_response_sensitive (GTK_DIALOG (private->dialog),
                                           entry->response_id,
                                           FALSE);
    }
}

static void
clay_tool_gui_update_buttons (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);
  GList              *list;
  gint               *ids;
  gint                n_ids;
  gint                n_alternatives = 0;

  n_ids = g_list_length (private->response_entries);
  ids   = g_new0 (gint, n_ids);

  for (list = private->response_entries;
       list;
       list = g_list_next (list))
    {
      ResponseEntry *entry = list->data;

      if (entry->alternative_position >= 0 &&
          entry->alternative_position < n_ids)
        {
          ids[entry->alternative_position] = entry->response_id;
          n_alternatives++;
        }
    }

  if (n_ids == n_alternatives)
    {
      if (private->overlay)
        {
          clay_overlay_dialog_set_alternative_button_order (CLAY_OVERLAY_DIALOG (private->dialog),
                                                            n_ids, ids);
        }
      else
        {
          clay_dialog_set_alternative_button_order_from_array (CLAY_DIALOG (private->dialog),
                                                               n_ids, ids);
        }
    }

  g_free (ids);
}

static void
clay_tool_gui_update_shell (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);

  if (private->overlay)
    {
      if (gtk_widget_get_parent (private->dialog))
        {
          clay_tool_gui_hide (gui);

          if (private->shell)
            clay_tool_gui_show (gui);
        }
    }
  else
    {
      clay_tool_dialog_set_shell (CLAY_TOOL_DIALOG (private->dialog),
                                  private->shell);
    }
}

static void
clay_tool_gui_update_viewable (ClayToolGui *gui)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);

  if (! private->overlay)
    {
      ClayContext *context = NULL;

      if (private->tool_info)
        context = CLAY_CONTEXT (private->tool_info->tool_options);

      clay_viewable_dialog_set_viewables (CLAY_VIEWABLE_DIALOG (private->dialog),
                                          g_list_copy (private->viewables), context);
    }
}

static void
clay_tool_gui_dialog_response (GtkWidget   *dialog,
                               gint         response_id,
                               ClayToolGui *gui)
{
  if (response_id == CLAY_RESPONSE_DETACH)
    {
      clay_tool_gui_set_auto_overlay (gui, FALSE);
      clay_tool_gui_set_overlay (gui,
                                 clay_widget_get_monitor (dialog),
                                 FALSE);
    }
  else
    {
      g_signal_emit (gui, signals[RESPONSE], 0,
                     response_id);
    }
}

static void
clay_tool_gui_canvas_resized (GtkWidget     *canvas,
                              GtkAllocation *unused,
                              ClayToolGui   *gui)
{
  ClayToolGuiPrivate *private = GET_PRIVATE (gui);

  if (private->auto_overlay)
    {
      GtkRequisition requisition;
      GtkAllocation  allocation;
      gboolean       overlay = FALSE;

      gtk_widget_get_preferred_size (private->vbox, &requisition, NULL);
      gtk_widget_get_allocation (canvas, &allocation);

      if (allocation.width  > 2 * requisition.width &&
          allocation.height > 3 * requisition.height)
        {
          overlay = TRUE;
        }

      clay_tool_gui_set_overlay (gui,
                                 clay_widget_get_monitor (private->dialog),
                                 overlay);
    }
}

static ResponseEntry *
response_entry_new (gint         response_id,
                    const gchar *button_text)
{
  ResponseEntry *entry = g_slice_new0 (ResponseEntry);

  entry->response_id          = response_id;
  entry->button_text          = g_strdup (button_text);
  entry->alternative_position = -1;
  entry->sensitive            = TRUE;

  return entry;
}

static void
response_entry_free (ResponseEntry *entry)
{
  g_free (entry->button_text);

  g_slice_free (ResponseEntry, entry);
}

static ResponseEntry *
response_entry_find (GList *entries,
                     gint   response_id)
{
  for (; entries; entries = g_list_next (entries))
    {
      ResponseEntry *entry = entries->data;

      if (entry->response_id == response_id)
        return entry;
    }

  return NULL;
}
