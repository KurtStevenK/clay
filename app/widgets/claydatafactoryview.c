/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydatafactoryview.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydata.h"
#include "core/claydatafactory.h"
#include "core/claylist.h"
#include "core/claytaggedcontainer.h"

#include "claycombotagentry.h"
#include "claycontainertreestore.h"
#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claydatafactoryview.h"
#include "claydnd.h"
#include "claydocked.h"
#include "claymenufactory.h"
#include "claysessioninfo-aux.h"
#include "claytagentry.h"
#include "clayuimanager.h"
#include "clayviewrenderer.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_DATA_FACTORY,
  PROP_ACTION_GROUP
};


struct _ClayDataFactoryViewPrivate
{
  ClayDataFactory *factory;
  gchar           *action_group;

  ClayContainer   *tagged_container;
  GtkWidget       *query_tag_entry;
  GtkWidget       *assign_tag_entry;
  GList           *selected_items;

  GtkWidget       *follow_theme_toggle;
  gboolean         show_follow_theme_toggle;

  GtkWidget       *edit_button;
  GtkWidget       *new_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;
  GtkWidget       *refresh_button;
};


static void    clay_data_factory_view_docked_iface_init (ClayDockedInterface *iface);

static GObject *
               clay_data_factory_view_constructor    (GType                type,
                                                      guint                n_construct_params,
                                                      GObjectConstructParam *construct_params);
static void    clay_data_factory_view_constructed    (GObject             *object);
static void    clay_data_factory_view_dispose        (GObject             *object);
static void    clay_data_factory_view_set_property   (GObject             *object,
                                                      guint                property_id,
                                                      const GValue        *value,
                                                      GParamSpec          *pspec);
static void    clay_data_factory_view_get_property   (GObject             *object,
                                                      guint                property_id,
                                                      GValue              *value,
                                                      GParamSpec          *pspec);

static void    clay_data_factory_view_set_aux_info   (ClayDocked          *docked,
                                                      GList               *aux_info);
static GList * clay_data_factory_view_get_aux_info   (ClayDocked          *docked);

static void    clay_data_factory_view_activate_item  (ClayContainerEditor *editor,
                                                      ClayViewable        *viewable);
static void    clay_data_factory_view_select_item    (ClayContainerEditor *editor,
                                                      ClayViewable        *viewable);
static void  clay_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                                      const gchar         *path,
                                                      const gchar         *name,
                                                      ClayDataFactoryView *view);


G_DEFINE_TYPE_WITH_CODE (ClayDataFactoryView, clay_data_factory_view,
                         CLAY_TYPE_CONTAINER_EDITOR,
                         G_ADD_PRIVATE (ClayDataFactoryView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_data_factory_view_docked_iface_init))

#define parent_class clay_data_factory_view_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_data_factory_view_class_init (ClayDataFactoryViewClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  ClayContainerEditorClass *editor_class = CLAY_CONTAINER_EDITOR_CLASS (klass);

  object_class->constructor   = clay_data_factory_view_constructor;
  object_class->constructed   = clay_data_factory_view_constructed;
  object_class->dispose       = clay_data_factory_view_dispose;
  object_class->set_property  = clay_data_factory_view_set_property;
  object_class->get_property  = clay_data_factory_view_get_property;

  editor_class->select_item   = clay_data_factory_view_select_item;
  editor_class->activate_item = clay_data_factory_view_activate_item;

  g_object_class_install_property (object_class, PROP_DATA_FACTORY,
                                   g_param_spec_object ("data-factory",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DATA_FACTORY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ACTION_GROUP,
                                   g_param_spec_string ("action-group",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_data_factory_view_init (ClayDataFactoryView *view)
{
  view->priv = clay_data_factory_view_get_instance_private (view);

  view->priv->tagged_container = NULL;
  view->priv->query_tag_entry  = NULL;
  view->priv->assign_tag_entry = NULL;
  view->priv->selected_items   = NULL;
  view->priv->edit_button      = NULL;
  view->priv->new_button       = NULL;
  view->priv->duplicate_button = NULL;
  view->priv->delete_button    = NULL;
  view->priv->refresh_button   = NULL;

  view->priv->show_follow_theme_toggle = FALSE;
}

static void
clay_data_factory_view_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_aux_info = clay_data_factory_view_set_aux_info;
  iface->get_aux_info = clay_data_factory_view_get_aux_info;
}

static GObject *
clay_data_factory_view_constructor (GType                  type,
                                    guint                  n_construct_params,
                                    GObjectConstructParam *construct_params)
{
  ClayDataFactoryView *factory_view;
  GObject             *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

  factory_view = CLAY_DATA_FACTORY_VIEW (object);

  clay_assert (CLAY_IS_DATA_FACTORY (factory_view->priv->factory));
  clay_assert (factory_view->priv->action_group != NULL);

  factory_view->priv->tagged_container =
    clay_tagged_container_new (clay_data_factory_get_container (factory_view->priv->factory));

  /* this must happen in constructor(), because doing it in
   * set_property() warns about wrong construct property usage
   */
  g_object_set (object,
                "container", factory_view->priv->tagged_container,
                NULL);

  return object;
}

static void
clay_data_factory_view_constructed (GObject *object)
{
  ClayDataFactoryView        *factory_view = CLAY_DATA_FACTORY_VIEW (object);
  ClayDataFactoryViewPrivate *priv         = factory_view->priv;
  ClayContainerEditor        *editor       = CLAY_CONTAINER_EDITOR (object);
  ClayUIManager              *manager;
  ClayContext                *context;
  GtkWidget                  *image;
  GtkWidget                  *hbox;
  gchar                      *str;
  GtkIconSize                 button_icon_size;
  GtkReliefStyle              button_relief;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_container_editor_set_selection_mode (editor, GTK_SELECTION_SINGLE);

  if (CLAY_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      ClayContainerTreeView *tree_view;

      tree_view = CLAY_CONTAINER_TREE_VIEW (editor->view);

      clay_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (clay_data_factory_view_tree_name_edited),
                                                    factory_view);
    }

  manager = clay_editor_get_ui_manager (CLAY_EDITOR (editor->view));

  str = g_strdup_printf ("%s-edit", priv->action_group);
  if (clay_ui_manager_find_action (manager, priv->action_group, str))
    priv->edit_button =
      clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  if (clay_data_factory_view_has_data_new_func (factory_view))
    {
      str = g_strdup_printf ("%s-new", priv->action_group);
      priv->new_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                       priv->action_group,
                                       str, NULL);
      g_free (str);
    }

  str = g_strdup_printf ("%s-duplicate", priv->action_group);
  if (clay_ui_manager_find_action (manager, priv->action_group, str))
    priv->duplicate_button =
      clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-delete", priv->action_group);
  if (clay_ui_manager_find_action (manager, priv->action_group, str))
    priv->delete_button =
      clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  str = g_strdup_printf ("%s-refresh", priv->action_group);
  if (clay_ui_manager_find_action (manager, priv->action_group, str))
    priv->refresh_button =
      clay_editor_add_action_button (CLAY_EDITOR (editor->view),
                                     priv->action_group,
                                     str, NULL);
  g_free (str);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (editor->view), hbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor->view), hbox, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /* Query tag entry */
  priv->query_tag_entry =
    clay_combo_tag_entry_new (CLAY_TAGGED_CONTAINER (priv->tagged_container),
                              CLAY_TAG_ENTRY_MODE_QUERY);
  gtk_box_pack_start (GTK_BOX (hbox), priv->query_tag_entry, TRUE, TRUE, 0);
  gtk_widget_set_visible (priv->query_tag_entry, TRUE);

  /* Follow Theme toggle */
  g_object_get (object,
                "context", &context,
                NULL);
  gtk_widget_style_get (GTK_WIDGET (editor->view),
                        "button-icon-size", &button_icon_size,
                        "button-relief",    &button_relief,
                        NULL);
#define CLAY_ICON_FOLLOW_THEME "clay-prefs-theme"
  priv->follow_theme_toggle  = clay_prop_toggle_new (G_OBJECT (context->clay->config),
                                                     "viewables-follow-theme",
                                                     CLAY_ICON_FOLLOW_THEME, NULL,
                                                     &image);
  gtk_button_set_relief (GTK_BUTTON (priv->follow_theme_toggle), button_relief);
  /* Re-setting the image to make sure we use the correct size from theme.  */
  gtk_image_set_from_icon_name (GTK_IMAGE (image), CLAY_ICON_FOLLOW_THEME, button_icon_size);
#undef CLAY_ICON_FOLLOW_THEME
  gtk_box_pack_start (GTK_BOX (hbox), priv->follow_theme_toggle, FALSE, FALSE, 0);
  gtk_widget_set_visible (priv->follow_theme_toggle, priv->show_follow_theme_toggle);
  g_object_unref (context);

  /* Assign tag entry */
  priv->assign_tag_entry =
    clay_combo_tag_entry_new (CLAY_TAGGED_CONTAINER (priv->tagged_container),
                              CLAY_TAG_ENTRY_MODE_ASSIGN);
  clay_tag_entry_set_selected_items (CLAY_TAG_ENTRY (priv->assign_tag_entry),
                                     priv->selected_items);
  g_list_free (priv->selected_items);
  priv->selected_items = NULL;
  gtk_box_pack_start (GTK_BOX (editor->view),
                      priv->assign_tag_entry,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (priv->assign_tag_entry, TRUE);

  if (priv->edit_button)
    clay_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->edit_button),
                                    clay_data_factory_get_data_type (priv->factory));

  if (priv->duplicate_button)
    clay_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->duplicate_button),
                                    clay_data_factory_get_data_type (priv->factory));

  if (priv->delete_button)
    clay_container_view_enable_dnd (editor->view,
                                    GTK_BUTTON (priv->delete_button),
                                    clay_data_factory_get_data_type (priv->factory));

  clay_ui_manager_update (manager, editor);
}

static void
clay_data_factory_view_dispose (GObject *object)
{
  ClayDataFactoryView *factory_view = CLAY_DATA_FACTORY_VIEW (object);

  g_clear_object (&factory_view->priv->tagged_container);
  g_clear_object (&factory_view->priv->factory);

  g_clear_pointer (&factory_view->priv->action_group, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_data_factory_view_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayDataFactoryView *factory_view = CLAY_DATA_FACTORY_VIEW (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      factory_view->priv->factory = g_value_dup_object (value);
      break;

    case PROP_ACTION_GROUP:
      factory_view->priv->action_group = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_data_factory_view_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayDataFactoryView *factory_view = CLAY_DATA_FACTORY_VIEW (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      g_value_set_object (value, factory_view->priv->factory);
      break;

    case PROP_ACTION_GROUP:
      g_value_set_string (value, factory_view->priv->action_group);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

#define AUX_INFO_TAG_FILTER "tag-filter"

static void
clay_data_factory_view_set_aux_info (ClayDocked *docked,
                                     GList      *aux_info)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (docked);
  GList               *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_TAG_FILTER))
        {
          gtk_entry_set_text (GTK_ENTRY (view->priv->query_tag_entry),
                              aux->value);
        }
    }
}

static GList *
clay_data_factory_view_get_aux_info (ClayDocked *docked)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (docked);
  GList               *aux_info;
  const gchar         *tag_filter;

  aux_info = parent_docked_iface->get_aux_info (docked);

  tag_filter = gtk_entry_get_text (GTK_ENTRY (view->priv->query_tag_entry));
  if (tag_filter && *tag_filter)
    {
      ClaySessionInfoAux *aux;

      aux = clay_session_info_aux_new (AUX_INFO_TAG_FILTER, tag_filter);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

GtkWidget *
clay_data_factory_view_new (ClayViewType      view_type,
                            ClayDataFactory  *factory,
                            ClayContext      *context,
                            gint              view_size,
                            gint              view_border_width,
                            ClayMenuFactory  *menu_factory,
                            const gchar      *menu_identifier,
                            const gchar      *ui_path,
                            const gchar      *action_group)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        CLAY_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (action_group != NULL, NULL);

  return g_object_new (CLAY_TYPE_DATA_FACTORY_VIEW,
                       "view-type",         view_type,
                       "data-factory",      factory,
                       "context",           context,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "menu-factory",      menu_factory,
                       "menu-identifier",   menu_identifier,
                       "ui-path",           ui_path,
                       "action-group",      action_group,
                       NULL);
}

void
clay_data_factory_view_show_follow_theme_toggle (ClayDataFactoryView *view,
                                                 gboolean             show)
{
  view->priv->show_follow_theme_toggle = show;

  if (view->priv->follow_theme_toggle != NULL)
    gtk_widget_set_visible (view->priv->follow_theme_toggle, show);
}

GtkWidget *
clay_data_factory_view_get_edit_button (ClayDataFactoryView *factory_view)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->edit_button;
}

GtkWidget *
clay_data_factory_view_get_duplicate_button (ClayDataFactoryView *factory_view)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->duplicate_button;
}

ClayDataFactory *
clay_data_factory_view_get_data_factory (ClayDataFactoryView *factory_view)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY_VIEW (factory_view), NULL);

  return factory_view->priv->factory;
}

GType
clay_data_factory_view_get_child_type (ClayDataFactoryView *factory_view)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY_VIEW (factory_view), G_TYPE_NONE);

  return clay_data_factory_get_data_type (factory_view->priv->factory);
}

gboolean
clay_data_factory_view_has_data_new_func (ClayDataFactoryView *factory_view)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY_VIEW (factory_view), FALSE);

  return clay_data_factory_has_data_new_func (factory_view->priv->factory);
}

gboolean
clay_data_factory_view_have (ClayDataFactoryView *factory_view,
                             ClayObject          *object)
{
  g_return_val_if_fail (CLAY_IS_DATA_FACTORY_VIEW (factory_view), FALSE);

  return clay_container_have (clay_data_factory_get_container (factory_view->priv->factory),
                              object);
}

static void
clay_data_factory_view_select_item (ClayContainerEditor *editor,
                                    ClayViewable        *viewable)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (editor);

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  if (view->priv->assign_tag_entry)
    {
      ClayContainerView *container_view = CLAY_CONTAINER_VIEW (editor->view);
      GList             *active_items   = NULL;

      clay_container_view_get_selected (container_view, &active_items);
      clay_tag_entry_set_selected_items (CLAY_TAG_ENTRY (view->priv->assign_tag_entry),
                                         active_items);
      g_list_free (active_items);
    }
  else if (viewable)
    {
      view->priv->selected_items = g_list_append (view->priv->selected_items,
                                                  viewable);
    }
}

static void
clay_data_factory_view_activate_item (ClayContainerEditor *editor,
                                      ClayViewable        *viewable)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (editor);
  ClayData            *data = CLAY_DATA (viewable);

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  if (data && clay_data_factory_view_have (view,
                                           CLAY_OBJECT (data)))
    {
      if (view->priv->edit_button &&
          gtk_widget_is_sensitive (view->priv->edit_button))
        gtk_button_clicked (GTK_BUTTON (view->priv->edit_button));
    }
}

static void
clay_data_factory_view_tree_name_edited (GtkCellRendererText *cell,
                                         const gchar         *path_str,
                                         const gchar         *new_name,
                                         ClayDataFactoryView *view)
{
  ClayContainerTreeView *tree_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = CLAY_CONTAINER_TREE_VIEW (CLAY_CONTAINER_EDITOR (view)->view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayViewRenderer *renderer;
      ClayData         *data;
      gchar            *name;

      gtk_tree_model_get (tree_view->model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      data = CLAY_DATA (renderer->viewable);

      if (! new_name)
        new_name = "";

      name = g_strstrip (g_strdup (new_name));

      /* We must block the edited callback at this point, because either
       * the call to clay_object_take_name() or gtk_tree_store_set() below
       * will trigger a re-ordering and emission of the rows_reordered signal.
       * This in turn will stop the editing operation and cause a call to this
       * very callback function again. Because the order of the rows has
       * changed by then, "path_str" will point at another item and cause the
       * name of this item to be changed as well.
       */
      g_signal_handlers_block_by_func (cell,
                                       clay_data_factory_view_tree_name_edited,
                                       view);

      if (clay_data_is_writable (data) &&
          strlen (name)                &&
          g_strcmp0 (name, clay_object_get_name (data)))
        {
          clay_object_take_name (CLAY_OBJECT (data), name);
        }
      else
        {
          g_free (name);

          name = clay_viewable_get_description (renderer->viewable, NULL);
          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              CLAY_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);
        }

      g_signal_handlers_unblock_by_func (cell,
                                         clay_data_factory_view_tree_name_edited,
                                         view);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
