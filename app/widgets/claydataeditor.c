/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydataeditor.c
 * Copyright (C) 2002-2004 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydata.h"
#include "core/claydatafactory.h"

#include "claydataeditor.h"
#include "claydocked.h"
#include "claymenufactory.h"
#include "claysessioninfo-aux.h"
#include "clayuimanager.h"

#include "clay-intl.h"


#define DEFAULT_MINIMAL_HEIGHT 96


enum
{
  PROP_0,
  PROP_DATA_FACTORY,
  PROP_CONTEXT,
  PROP_DATA
};


static void       clay_data_editor_docked_iface_init (ClayDockedInterface *iface);

static void       clay_data_editor_constructed       (GObject        *object);
static void       clay_data_editor_dispose           (GObject        *object);
static void       clay_data_editor_set_property      (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void       clay_data_editor_get_property      (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);

static void       clay_data_editor_style_updated     (GtkWidget      *widget);

static void       clay_data_editor_set_context       (ClayDocked     *docked,
                                                      ClayContext    *context);
static void       clay_data_editor_set_aux_info      (ClayDocked     *docked,
                                                      GList          *aux_info);
static GList    * clay_data_editor_get_aux_info      (ClayDocked     *docked);
static gchar    * clay_data_editor_get_title         (ClayDocked     *docked);

static void       clay_data_editor_real_set_data     (ClayDataEditor *editor,
                                                      ClayData       *data);

static void       clay_data_editor_data_changed      (ClayContext    *context,
                                                      ClayData       *data,
                                                      ClayDataEditor *editor);
static gboolean   clay_data_editor_name_key_press    (GtkWidget      *widget,
                                                      GdkEventKey    *kevent,
                                                      ClayDataEditor *editor);
static void       clay_data_editor_name_activate     (GtkWidget      *widget,
                                                      ClayDataEditor *editor);
static gboolean   clay_data_editor_name_focus_out    (GtkWidget      *widget,
                                                      GdkEvent       *event,
                                                      ClayDataEditor *editor);

static void       clay_data_editor_data_name_changed (ClayObject     *object,
                                                      ClayDataEditor *editor);

static void       clay_data_editor_save_dirty        (ClayDataEditor *editor);


G_DEFINE_TYPE_WITH_CODE (ClayDataEditor, clay_data_editor, CLAY_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_data_editor_docked_iface_init))

#define parent_class clay_data_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_data_editor_class_init (ClayDataEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = clay_data_editor_constructed;
  object_class->set_property  = clay_data_editor_set_property;
  object_class->get_property  = clay_data_editor_get_property;
  object_class->dispose       = clay_data_editor_dispose;

  widget_class->style_updated = clay_data_editor_style_updated;

  klass->set_data             = clay_data_editor_real_set_data;

  g_object_class_install_property (object_class, PROP_DATA_FACTORY,
                                   g_param_spec_object ("data-factory",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DATA_FACTORY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DATA,
                                   g_param_spec_object ("data",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DATA,
                                                        CLAY_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimal-height",
                                                             NULL, NULL,
                                                             32,
                                                             G_MAXINT,
                                                             DEFAULT_MINIMAL_HEIGHT,
                                                             CLAY_PARAM_READABLE));
}

static void
clay_data_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context  = clay_data_editor_set_context;
  iface->set_aux_info = clay_data_editor_set_aux_info;
  iface->get_aux_info = clay_data_editor_get_aux_info;
  iface->get_title    = clay_data_editor_get_title;
}

static void
clay_data_editor_init (ClayDataEditor *editor)
{
  editor->data_factory  = NULL;
  editor->context       = NULL;
  editor->data          = NULL;
  editor->data_editable = FALSE;

  editor->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (editor), editor->name_entry, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->name_entry, TRUE);

  gtk_editable_set_editable (GTK_EDITABLE (editor->name_entry), FALSE);

  g_signal_connect (editor->name_entry, "key-press-event",
                    G_CALLBACK (clay_data_editor_name_key_press),
                    editor);
  g_signal_connect (editor->name_entry, "activate",
                    G_CALLBACK (clay_data_editor_name_activate),
                    editor);
  g_signal_connect (editor->name_entry, "focus-out-event",
                    G_CALLBACK (clay_data_editor_name_focus_out),
                    editor);
}

static void
clay_data_editor_constructed (GObject *object)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DATA_FACTORY (editor->data_factory));
  clay_assert (CLAY_IS_CONTEXT (editor->context));

  clay_data_editor_set_edit_active (editor, TRUE);
}

static void
clay_data_editor_dispose (GObject *object)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (object);

  if (editor->data)
    {
      /* Save dirty data before we clear out */
      clay_data_editor_save_dirty (editor);
      clay_data_editor_set_data (editor, NULL);
    }

  if (editor->context)
    clay_docked_set_context (CLAY_DOCKED (editor), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_data_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      editor->data_factory = g_value_get_object (value);
      break;
    case PROP_CONTEXT:
      clay_docked_set_context (CLAY_DOCKED (object),
                               g_value_get_object (value));
      break;
    case PROP_DATA:
      clay_data_editor_set_data (editor, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_data_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      g_value_set_object (value, editor->data_factory);
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;
    case PROP_DATA:
      g_value_set_object (value, editor->data);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_data_editor_style_updated (GtkWidget *widget)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (widget);
  gint            minimal_height;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "minimal-height", &minimal_height,
                        NULL);

  if (editor->view)
    gtk_widget_set_size_request (editor->view, -1, minimal_height);
}

static void
clay_data_editor_set_context (ClayDocked  *docked,
                              ClayContext *context)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (docked);

  if (context == editor->context)
    return;

  if (parent_docked_iface->set_context)
    parent_docked_iface->set_context (docked, context);

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_data_editor_data_changed,
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (editor->context)
    {
      GType     data_type;
      ClayData *data;

      g_object_ref (editor->context);

      data_type = clay_data_factory_get_data_type (editor->data_factory);
      data = CLAY_DATA (clay_context_get_by_type (editor->context, data_type));

      g_signal_connect (editor->context,
                        clay_context_type_to_signal_name (data_type),
                        G_CALLBACK (clay_data_editor_data_changed),
                        editor);

      clay_data_editor_data_changed (editor->context, data, editor);
    }
}

#define AUX_INFO_EDIT_ACTIVE  "edit-active"
#define AUX_INFO_CURRENT_DATA "current-data"

static void
clay_data_editor_set_aux_info (ClayDocked *docked,
                               GList      *aux_info)
{
  ClayDataEditor *editor = CLAY_DATA_EDITOR (docked);
  GList          *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_EDIT_ACTIVE))
        {
          gboolean edit_active;

          edit_active = ! g_ascii_strcasecmp (aux->value, "true");

          clay_data_editor_set_edit_active (editor, edit_active);
        }
      else if (! strcmp (aux->name, AUX_INFO_CURRENT_DATA))
        {
          if (! editor->edit_active)
            {
              ClayData *data;

              data = (ClayData *)
                clay_container_get_child_by_name (clay_data_factory_get_container (editor->data_factory),
                                                  aux->value);

              if (data)
                clay_data_editor_set_data (editor, data);
            }
        }
    }
}

static GList *
clay_data_editor_get_aux_info (ClayDocked *docked)
{
  ClayDataEditor     *editor = CLAY_DATA_EDITOR (docked);
  GList              *aux_info;
  ClaySessionInfoAux *aux;

  aux_info = parent_docked_iface->get_aux_info (docked);

  aux = clay_session_info_aux_new (AUX_INFO_EDIT_ACTIVE,
                                   editor->edit_active ? "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  if (editor->data)
    {
      const gchar *value;

      value = clay_object_get_name (editor->data);

      aux = clay_session_info_aux_new (AUX_INFO_CURRENT_DATA, value);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static gchar *
clay_data_editor_get_title (ClayDocked *docked)
{
  ClayDataEditor      *editor       = CLAY_DATA_EDITOR (docked);
  ClayDataEditorClass *editor_class = CLAY_DATA_EDITOR_GET_CLASS (editor);

  if (editor->data_editable)
    return g_strdup (editor_class->title);
  else
    return g_strdup_printf (_("%s (read only)"), editor_class->title);
}

static void
clay_data_editor_real_set_data (ClayDataEditor *editor,
                                ClayData       *data)
{
  gboolean editable;
  gboolean name_editable;

  if (editor->data)
    {
      clay_data_editor_save_dirty (editor);

      g_signal_handlers_disconnect_by_func (editor->data,
                                            clay_data_editor_data_name_changed,
                                            editor);

      g_object_unref (editor->data);
    }

  editor->data = data;

  name_editable = editor->data &&
                  clay_viewable_is_name_editable (CLAY_VIEWABLE (editor->data));

  if (editor->data)
    {
      gchar *name;

      g_object_ref (editor->data);

      if (name_editable)
        name = g_strdup (clay_object_get_name (editor->data));
      else
        name = g_strdup_printf (_("%s (read only)"),
                                (clay_object_get_name (editor->data)));

      gtk_entry_set_text (GTK_ENTRY (editor->name_entry), name);
      g_free (name);

      g_signal_connect (editor->data, "name-changed",
                        G_CALLBACK (clay_data_editor_data_name_changed),
                        editor);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (editor->name_entry), "");
    }

  gtk_editable_set_editable (GTK_EDITABLE (editor->name_entry),
                             name_editable);

  editable = (editor->data && clay_data_is_writable (editor->data));
  if (editor->data_editable != editable)
    {
      editor->data_editable = editable;

      clay_docked_title_changed (CLAY_DOCKED (editor));
    }
}

void
clay_data_editor_set_data (ClayDataEditor *editor,
                           ClayData       *data)
{
  g_return_if_fail (CLAY_IS_DATA_EDITOR (editor));
  g_return_if_fail (data == NULL || CLAY_IS_DATA (data));
  g_return_if_fail (data == NULL ||
                    g_type_is_a (G_TYPE_FROM_INSTANCE (data),
                                 clay_data_factory_get_data_type (editor->data_factory)));

  if (editor->data != data)
    {
      CLAY_DATA_EDITOR_GET_CLASS (editor)->set_data (editor, data);

      g_object_notify (G_OBJECT (editor), "data");

      if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
        clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                                clay_editor_get_popup_data (CLAY_EDITOR (editor)));
    }
}

ClayData *
clay_data_editor_get_data (ClayDataEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_DATA_EDITOR (editor), NULL);

  return editor->data;
}

void
clay_data_editor_set_edit_active (ClayDataEditor *editor,
                                  gboolean        edit_active)
{
  g_return_if_fail (CLAY_IS_DATA_EDITOR (editor));

  if (editor->edit_active != edit_active)
    {
      editor->edit_active = edit_active;

      if (editor->edit_active && editor->context)
        {
          GType     data_type;
          ClayData *data;

          data_type = clay_data_factory_get_data_type (editor->data_factory);
          data = CLAY_DATA (clay_context_get_by_type (editor->context,
                                                      data_type));

          clay_data_editor_set_data (editor, data);
        }
    }
}

gboolean
clay_data_editor_get_edit_active (ClayDataEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_DATA_EDITOR (editor), FALSE);

  return editor->edit_active;
}


/*  private functions  */

static void
clay_data_editor_data_changed (ClayContext    *context,
                               ClayData       *data,
                               ClayDataEditor *editor)
{
  if (editor->edit_active)
    clay_data_editor_set_data (editor, data);
}

static gboolean
clay_data_editor_name_key_press (GtkWidget      *widget,
                                 GdkEventKey    *kevent,
                                 ClayDataEditor *editor)
{
  if (kevent->keyval == GDK_KEY_Escape)
    {
      gtk_entry_set_text (GTK_ENTRY (editor->name_entry),
                          clay_object_get_name (editor->data));
      return TRUE;
    }

  return FALSE;
}

static void
clay_data_editor_name_activate (GtkWidget      *widget,
                                ClayDataEditor *editor)
{
  if (editor->data &&
      clay_viewable_is_name_editable (CLAY_VIEWABLE (editor->data)))
    {
      gchar *new_name;

      new_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
      new_name = g_strstrip (new_name);

      if (strlen (new_name) &&
          g_strcmp0 (new_name, clay_object_get_name (editor->data)))
        {
          clay_object_take_name (CLAY_OBJECT (editor->data), new_name);
        }
      else
        {
          gtk_entry_set_text (GTK_ENTRY (widget),
                              clay_object_get_name (editor->data));
          g_free (new_name);
        }
    }
}

static gboolean
clay_data_editor_name_focus_out (GtkWidget      *widget,
                                 GdkEvent       *event,
                                 ClayDataEditor *editor)
{
  clay_data_editor_name_activate (widget, editor);

  return FALSE;
}

static void
clay_data_editor_data_name_changed (ClayObject     *object,
                                    ClayDataEditor *editor)
{
  gtk_entry_set_text (GTK_ENTRY (editor->name_entry),
                      clay_object_get_name (object));
}

static void
clay_data_editor_save_dirty (ClayDataEditor *editor)
{
  ClayData *data = editor->data;

  if (data                      &&
      clay_data_is_dirty (data) &&
      clay_data_is_writable (data))
    {
      GError *error = NULL;

      if (! clay_data_factory_data_save_single (editor->data_factory, data,
                                                &error))
        {
          clay_message_literal (clay_data_factory_get_clay (editor->data_factory),
                                G_OBJECT (editor),
                                CLAY_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}
