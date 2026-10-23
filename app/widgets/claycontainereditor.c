/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainereditor.c
 * Copyright (C) 2001-2011 Michael Natterer <mitch@gimp.org>
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

#include "config/clayguiconfig.h" /* playground */

#include "core/clay.h" /* playground */
#include "core/clayasyncset.h"
#include "core/claycontext.h"
#include "core/claylist.h"
#include "core/clayviewable.h"

#include "claycontainereditor.h"
#include "claycontainericonview.h"
#include "claycontainerlistview.h"
#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claydocked.h"
#include "claymenufactory.h"
#include "clayviewrenderer.h"
#include "clayuimanager.h"


enum
{
  PROP_0,
  PROP_VIEW_TYPE,
  PROP_CONTAINER,
  PROP_CONTEXT,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH,
  PROP_MENU_FACTORY,
  PROP_MENU_IDENTIFIER,
  PROP_UI_PATH
};


struct _ClayContainerEditorPrivate
{
  ClayViewType     view_type;
  ClayContainer   *container;
  ClayContext     *context;
  gint             view_size;
  gint             view_border_width;
  ClayMenuFactory *menu_factory;
  gchar           *menu_identifier;
  gchar           *ui_path;
  GtkWidget       *busy_box;
  GBinding        *async_set_binding;
};


static void  clay_container_editor_docked_iface_init (ClayDockedInterface *iface);

static void   clay_container_editor_constructed      (GObject             *object);
static void   clay_container_editor_dispose          (GObject             *object);
static void   clay_container_editor_set_property     (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void   clay_container_editor_get_property     (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static void   clay_container_editor_selection_changed(ClayContainerView   *view,
                                                      ClayContainerEditor *editor);
static void   clay_container_editor_item_activated   (GtkWidget           *widget,
                                                      ClayViewable        *viewable,
                                                      ClayContainerEditor *editor);

static GtkWidget * clay_container_editor_get_preview (ClayDocked       *docked,
                                                      ClayContext      *context,
                                                      GtkIconSize       size);
static void        clay_container_editor_set_context (ClayDocked       *docked,
                                                      ClayContext      *context);
static ClayUIManager * clay_container_editor_get_menu(ClayDocked       *docked,
                                                      const gchar     **ui_path,
                                                      gpointer         *popup_data);

static gboolean  clay_container_editor_has_button_bar      (ClayDocked *docked);
static void      clay_container_editor_set_show_button_bar (ClayDocked *docked,
                                                            gboolean    show);
static gboolean  clay_container_editor_get_show_button_bar (ClayDocked *docked);


G_DEFINE_TYPE_WITH_CODE (ClayContainerEditor, clay_container_editor,
                         GTK_TYPE_BOX,
                         G_ADD_PRIVATE (ClayContainerEditor)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_container_editor_docked_iface_init))

#define parent_class clay_container_editor_parent_class


static void
clay_container_editor_class_init (ClayContainerEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed   = clay_container_editor_constructed;
  object_class->dispose       = clay_container_editor_dispose;
  object_class->set_property  = clay_container_editor_set_property;
  object_class->get_property  = clay_container_editor_get_property;

  klass->select_item     = NULL;
  klass->activate_item   = NULL;

  g_object_class_install_property (object_class, PROP_VIEW_TYPE,
                                   g_param_spec_enum ("view-type",
                                                      NULL, NULL,
                                                      CLAY_TYPE_VIEW_TYPE,
                                                      CLAY_VIEW_TYPE_LIST,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_int ("view-size",
                                                     NULL, NULL,
                                                     1, CLAY_VIEWABLE_MAX_PREVIEW_SIZE,
                                                     CLAY_VIEW_SIZE_MEDIUM,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_VIEW_BORDER_WIDTH,
                                   g_param_spec_int ("view-border-width",
                                                     NULL, NULL,
                                                     0,
                                                     CLAY_VIEW_MAX_BORDER_WIDTH,
                                                     1,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        CLAY_TYPE_MENU_FACTORY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_IDENTIFIER,
                                   g_param_spec_string ("menu-identifier",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_PATH,
                                   g_param_spec_string ("ui-path",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_container_editor_docked_iface_init (ClayDockedInterface *iface)
{
  iface->get_preview         = clay_container_editor_get_preview;
  iface->set_context         = clay_container_editor_set_context;
  iface->get_menu            = clay_container_editor_get_menu;
  iface->has_button_bar      = clay_container_editor_has_button_bar;
  iface->set_show_button_bar = clay_container_editor_set_show_button_bar;
  iface->get_show_button_bar = clay_container_editor_get_show_button_bar;
}

static void
clay_container_editor_init (ClayContainerEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  editor->priv = clay_container_editor_get_instance_private (editor);
}

static void
clay_container_editor_constructed (GObject *object)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CONTAINER (editor->priv->container));
  clay_assert (CLAY_IS_CONTEXT (editor->priv->context));

  switch (editor->priv->view_type)
    {
    case CLAY_VIEW_TYPE_GRID:
      editor->view =
        CLAY_CONTAINER_VIEW (clay_container_icon_view_new (editor->priv->container,
                                                           editor->priv->context,
                                                           editor->priv->view_size,
                                                           editor->priv->view_border_width));
      break;

    case CLAY_VIEW_TYPE_LIST:
      if (CLAY_GUI_CONFIG (editor->priv->context->clay->config)->playground_use_list_box)
        editor->view =
          CLAY_CONTAINER_VIEW (clay_container_list_view_new (editor->priv->container,
                                                             editor->priv->context,
                                                             editor->priv->view_size,
                                                             editor->priv->view_border_width));
      else
        editor->view =
          CLAY_CONTAINER_VIEW (clay_container_tree_view_new (editor->priv->container,
                                                             editor->priv->context,
                                                             editor->priv->view_size,
                                                             editor->priv->view_border_width));
      break;

    default:
      clay_assert_not_reached ();
    }

  clay_editor_set_show_name (CLAY_EDITOR (editor->view),
                             (editor->priv->view_type == CLAY_VIEW_TYPE_GRID));

  if (CLAY_IS_LIST (editor->priv->container))
    clay_container_view_set_reorderable (CLAY_CONTAINER_VIEW (editor->view),
                                         ! clay_list_get_sort_func (CLAY_LIST (editor->priv->container)));

  if (editor->priv->menu_factory    &&
      editor->priv->menu_identifier &&
      editor->priv->ui_path)
    {
      clay_editor_create_menu (CLAY_EDITOR (editor->view),
                               editor->priv->menu_factory,
                               editor->priv->menu_identifier,
                               editor->priv->ui_path,
                               editor);
    }

  gtk_box_pack_start (GTK_BOX (editor), GTK_WIDGET (editor->view),
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (GTK_WIDGET (editor->view), TRUE);

  editor->priv->busy_box = clay_busy_box_new (NULL);
  gtk_box_pack_start (GTK_BOX (editor), editor->priv->busy_box, TRUE, TRUE, 0);

  g_object_bind_property (editor->priv->busy_box, "visible",
                          editor->view,           "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_signal_connect_object (editor->view, "selection-changed",
                           G_CALLBACK (clay_container_editor_selection_changed),
                           editor, 0);

  g_signal_connect_object (editor->view, "item-activated",
                           G_CALLBACK (clay_container_editor_item_activated),
                           editor, 0);
}

static void
clay_container_editor_dispose (GObject *object)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (object);

  clay_container_editor_bind_to_async_set (editor, NULL, NULL);

  g_clear_object (&editor->priv->container);
  g_clear_object (&editor->priv->context);
  g_clear_object (&editor->priv->menu_factory);

  g_clear_pointer (&editor->priv->menu_identifier, g_free);
  g_clear_pointer (&editor->priv->ui_path,         g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_container_editor_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_TYPE:
      editor->priv->view_type = g_value_get_enum (value);
      break;

    case PROP_CONTAINER:
      editor->priv->container = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      editor->priv->context = g_value_dup_object (value);
      break;

    case PROP_VIEW_SIZE:
      editor->priv->view_size = g_value_get_int (value);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      editor->priv->view_border_width = g_value_get_int (value);
      break;

    case PROP_MENU_FACTORY:
      editor->priv->menu_factory = g_value_dup_object (value);
      break;

    case PROP_MENU_IDENTIFIER:
      editor->priv->menu_identifier = g_value_dup_string (value);
      break;

    case PROP_UI_PATH:
      editor->priv->ui_path = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_container_editor_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_TYPE:
      g_value_set_enum (value, editor->priv->view_type);
      break;

    case PROP_CONTAINER:
      g_value_set_object (value, editor->priv->container);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, editor->priv->context);
      break;

    case PROP_VIEW_SIZE:
      g_value_set_int (value, editor->priv->view_size);
      break;

    case PROP_VIEW_BORDER_WIDTH:
      g_value_set_int (value, editor->priv->view_border_width);
      break;

    case PROP_MENU_FACTORY:
      g_value_set_object (value, editor->priv->menu_factory);
      break;

    case PROP_MENU_IDENTIFIER:
      g_value_set_string (value, editor->priv->menu_identifier);
      break;

    case PROP_UI_PATH:
      g_value_set_string (value, editor->priv->ui_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkSelectionMode
clay_container_editor_get_selection_mode (ClayContainerEditor *editor)
{
  return clay_container_view_get_selection_mode (CLAY_CONTAINER_VIEW (editor->view));
}

void
clay_container_editor_set_selection_mode (ClayContainerEditor *editor,
                                          GtkSelectionMode     mode)
{
  clay_container_view_set_selection_mode (CLAY_CONTAINER_VIEW (editor->view),
                                          mode);
}


/*  private functions  */

static void
clay_container_editor_selection_changed (ClayContainerView   *view,
                                         ClayContainerEditor *editor)
{
  ClayContainerEditorClass *klass       = CLAY_CONTAINER_EDITOR_GET_CLASS (editor);
  ClayViewable             *viewable    = NULL;
  ClayEditor               *editor_view = NULL;
  GList                    *items;

  clay_container_view_get_selected (view, &items);

  /* XXX Right now a ClayContainerEditor only supports 1 item selected
   * at once. Let's see later if we want to allow more.
   *
   * g_warn_if_fail (n_items < 2);
   */

  /* The editor view may get destroyed through a chain of signals when
   * changing the context viewables with clay_context_set_by_type().
   * We make sure it still exists before working on it with a weak
   * pointer.
   */
  g_set_weak_pointer (&editor_view, CLAY_EDITOR (editor->view));

  if (items)
    viewable = items->data;

  if (klass->select_item)
    klass->select_item (editor, viewable);

  if (viewable && editor_view)
    {
      gchar *desc = clay_viewable_get_description (viewable, NULL);

      clay_editor_set_name (editor_view, desc);
      g_free (desc);
    }

  if (editor_view && clay_editor_get_ui_manager (editor_view))
    clay_ui_manager_update (clay_editor_get_ui_manager (editor_view),
                            clay_editor_get_popup_data (editor_view));

  g_clear_weak_pointer (&editor_view);

  g_list_free (items);
}

static void
clay_container_editor_item_activated (GtkWidget           *widget,
                                      ClayViewable        *viewable,
                                      ClayContainerEditor *editor)
{
  ClayContainerEditorClass *klass = CLAY_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->activate_item)
    klass->activate_item (editor, viewable);
}

static GtkWidget *
clay_container_editor_get_preview (ClayDocked   *docked,
                                   ClayContext  *context,
                                   GtkIconSize   size)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (docked);

  return clay_docked_get_preview (CLAY_DOCKED (editor->view),
                                  context, size);
}

static void
clay_container_editor_set_context (ClayDocked  *docked,
                                   ClayContext *context)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (docked);

  clay_docked_set_context (CLAY_DOCKED (editor->view), context);
}

static ClayUIManager *
clay_container_editor_get_menu (ClayDocked   *docked,
                                const gchar **ui_path,
                                gpointer     *popup_data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (docked);

  return clay_docked_get_menu (CLAY_DOCKED (editor->view), ui_path, popup_data);
}

static gboolean
clay_container_editor_has_button_bar (ClayDocked *docked)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (docked);

  return clay_docked_has_button_bar (CLAY_DOCKED (editor->view));
}

static void
clay_container_editor_set_show_button_bar (ClayDocked *docked,
                                           gboolean    show)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (docked);

  clay_docked_set_show_button_bar (CLAY_DOCKED (editor->view), show);
}

static gboolean
clay_container_editor_get_show_button_bar (ClayDocked *docked)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (docked);

  return clay_docked_get_show_button_bar (CLAY_DOCKED (editor->view));
}

void
clay_container_editor_bind_to_async_set (ClayContainerEditor *editor,
                                         ClayAsyncSet        *async_set,
                                         const gchar         *message)
{
  g_return_if_fail (CLAY_IS_CONTAINER_EDITOR (editor));
  g_return_if_fail (async_set == NULL || CLAY_IS_ASYNC_SET (async_set));
  g_return_if_fail (async_set == NULL || message != NULL);

  if (! async_set && ! editor->priv->async_set_binding)
    return;

  g_clear_object (&editor->priv->async_set_binding);

  if (async_set)
    {
      clay_busy_box_set_message (CLAY_BUSY_BOX (editor->priv->busy_box),
                                 message);

      editor->priv->async_set_binding = g_object_bind_property (
        async_set,              "empty",
        editor->priv->busy_box, "visible",
        G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
    }
  else
    {
      gtk_widget_set_visible (editor->priv->busy_box, FALSE);
    }
}
