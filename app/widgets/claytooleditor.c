/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooleditor.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
 *                         Stephen Griffiths <scgmk5@gmail.com>
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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claytoolgroup.h"
#include "core/claytreehandler.h"

#include "tools/clay-tools.h"

#include "claycontainertreestore.h"
#include "claycontainerview.h"
#include "claycontainerview-cruft.h"
#include "claydnd.h"
#include "clayviewrenderer.h"
#include "claytooleditor.h"
#include "clayhelp-ids.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


struct _ClayToolEditorPrivate
{
  ClayContainer   *container;
  ClayContext     *context;

  GtkWidget       *scrolled;

  GtkWidget       *new_group_button;
  GtkWidget       *raise_button;
  GtkWidget       *lower_button;
  GtkWidget       *delete_button;
  GtkWidget       *reset_button;

  ClayTreeHandler *tool_item_notify_handler;

  /* State of tools at creation of the editor, stored to support
   * reverting changes
   */
  gchar           *initial_tool_state;
};


/*  local function prototypes  */

static void            clay_tool_editor_view_iface_init           (ClayContainerViewInterface *iface);

static void            clay_tool_editor_constructed               (GObject                    *object);

static void            clay_tool_editor_set_container             (ClayContainerView          *container_view,
                                                                   ClayContainer              *container);
static void            clay_tool_editor_set_context               (ClayContainerView          *container_view,
                                                                   ClayContext                *context);
static gboolean        clay_tool_editor_set_selected              (ClayContainerView          *view,
                                                                   GList                      *items);

static gboolean        clay_tool_editor_drop_possible             (ClayContainerTreeView      *tree_view,
                                                                   ClayDndType                 src_type,
                                                                   GList                      *src_viewables,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreePath                *drop_path,
                                                                   GtkTreeViewDropPosition     drop_pos,
                                                                   GtkTreeViewDropPosition    *return_drop_pos,
                                                                   GdkDragAction              *return_drag_action);
static void            clay_tool_editor_drop_viewables            (ClayContainerTreeView      *tree_view,
                                                                   GList                      *src_viewables,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);

static void            clay_tool_editor_tool_item_notify         (ClayToolItem                *tool_item,
                                                                  GParamSpec                  *pspec,
                                                                  ClayToolEditor              *tool_editor);

static void            clay_tool_editor_eye_data_func            (GtkTreeViewColumn           *tree_column,
                                                                  GtkCellRenderer             *cell,
                                                                  GtkTreeModel                *tree_model,
                                                                  GtkTreeIter                 *iter,
                                                                  gpointer                     data);
static void            clay_tool_editor_eye_clicked              (GtkCellRendererToggle       *toggle,
                                                                  gchar                       *path_str,
                                                                  GdkModifierType              state,
                                                                  ClayToolEditor              *tool_editor);

static void            clay_tool_editor_new_group_clicked        (GtkButton                   *button,
                                                                  ClayToolEditor              *tool_editor);
static void            clay_tool_editor_raise_clicked            (GtkButton                   *button,
                                                                  ClayToolEditor              *tool_editor);
static void            clay_tool_editor_raise_extend_clicked     (GtkButton                   *button,
                                                                  GdkModifierType              mask,
                                                                  ClayToolEditor              *tool_editor);
static void            clay_tool_editor_lower_clicked            (GtkButton                   *button,
                                                                  ClayToolEditor              *tool_editor);
static void            clay_tool_editor_lower_extend_clicked     (GtkButton                   *button,
                                                                  GdkModifierType              mask,
                                                                  ClayToolEditor              *tool_editor);
static void            clay_tool_editor_delete_clicked           (GtkButton                   *button,
                                                                  ClayToolEditor              *tool_editor);
static void            clay_tool_editor_reset_clicked            (GtkButton                   *button,
                                                                  ClayToolEditor              *tool_editor);

static ClayToolItem  * clay_tool_editor_get_selected_tool_item   (ClayToolEditor              *tool_editor);
static ClayContainer * clay_tool_editor_get_tool_item_container  (ClayToolEditor              *tool_editor,
                                                                  ClayToolItem                *tool_item);

static void            clay_tool_editor_update_container         (ClayToolEditor              *tool_editor);
static void            clay_tool_editor_update_sensitivity       (ClayToolEditor              *tool_editor);


G_DEFINE_TYPE_WITH_CODE (ClayToolEditor, clay_tool_editor,
                         CLAY_TYPE_CONTAINER_TREE_VIEW,
                         G_ADD_PRIVATE (ClayToolEditor)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_tool_editor_view_iface_init))

#define parent_class clay_tool_editor_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


/*  private functions  */

static void
clay_tool_editor_class_init (ClayToolEditorClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  ClayContainerTreeViewClass *tree_view_class = CLAY_CONTAINER_TREE_VIEW_CLASS (klass);

  object_class->constructed       = clay_tool_editor_constructed;

  tree_view_class->drop_possible  = clay_tool_editor_drop_possible;
  tree_view_class->drop_viewables = clay_tool_editor_drop_viewables;
}

static void
clay_tool_editor_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (CLAY_TYPE_CONTAINER_VIEW);

  iface->set_container = clay_tool_editor_set_container;
  iface->set_context   = clay_tool_editor_set_context;
  iface->set_selected  = clay_tool_editor_set_selected;
}

static void
clay_tool_editor_init (ClayToolEditor *tool_editor)
{
  tool_editor->priv = clay_tool_editor_get_instance_private (tool_editor);
}

static void
clay_tool_editor_constructed (GObject *object)
{
  ClayToolEditor        *tool_editor    = CLAY_TOOL_EDITOR (object);
  ClayContainerTreeView *tree_view      = CLAY_CONTAINER_TREE_VIEW (object);
  ClayContainerView     *container_view = CLAY_CONTAINER_VIEW (object);
  gint                   view_size;
  gint                   border_width;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view_size = clay_container_view_get_view_size (container_view,
                                                 &border_width);

  clay_editor_set_show_name (CLAY_EDITOR (tool_editor), FALSE);

  gtk_tree_view_set_level_indentation (tree_view->view,
                                       0.8 * (view_size + 2 * border_width));

  clay_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view),
                              CLAY_TYPE_TOOL_ITEM,
                              NULL, NULL);

  /* construct tree view */
  {
    GtkTreeViewColumn *column;
    GtkCellRenderer   *eye_cell;
    GtkStyleContext   *tree_style;
    GtkBorder          border;
    gint               icon_size;

    tree_style = gtk_widget_get_style_context (GTK_WIDGET (tool_editor));

    gtk_style_context_get_border (tree_style, 0, &border);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_insert_column (tree_view->view, column, 0);

    eye_cell = clay_cell_renderer_toggle_new (CLAY_ICON_VISIBLE);

    g_object_get (eye_cell, "icon-size", &icon_size, NULL);
    icon_size = MIN (icon_size, MAX (view_size - (border.left + border.right),
                                     view_size - (border.top + border.bottom)));
    g_object_set (eye_cell, "icon-size", icon_size, NULL);

    gtk_tree_view_column_pack_start (column, eye_cell, FALSE);
    gtk_tree_view_column_set_cell_data_func  (column, eye_cell,
                                              clay_tool_editor_eye_data_func,
                                              tree_view, NULL);

    clay_container_tree_view_add_toggle_cell (tree_view, eye_cell);

    g_signal_connect (eye_cell, "clicked",
                      G_CALLBACK (clay_tool_editor_eye_clicked),
                      tool_editor);
  }

  /* buttons */
  tool_editor->priv->new_group_button =
    clay_editor_add_button (CLAY_EDITOR (tool_editor), CLAY_ICON_FOLDER_NEW,
                            _("Create a new tool group"), NULL,
                            G_CALLBACK (clay_tool_editor_new_group_clicked),
                            NULL,
                            G_OBJECT (tool_editor));

  tool_editor->priv->raise_button =
    clay_editor_add_button (CLAY_EDITOR (tool_editor), CLAY_ICON_GO_UP,
                            _("Raise this item"),
                            _("Raise this item to the top"),
                            G_CALLBACK (clay_tool_editor_raise_clicked),
                            G_CALLBACK (clay_tool_editor_raise_extend_clicked),
                            G_OBJECT (tool_editor));

  tool_editor->priv->lower_button =
    clay_editor_add_button (CLAY_EDITOR (tool_editor), CLAY_ICON_GO_DOWN,
                            _("Lower this item"),
                            _("Lower this item to the bottom"),
                            G_CALLBACK (clay_tool_editor_lower_clicked),
                            G_CALLBACK (clay_tool_editor_lower_extend_clicked),
                            G_OBJECT (tool_editor));

  tool_editor->priv->delete_button =
    clay_editor_add_button (CLAY_EDITOR (tool_editor), CLAY_ICON_EDIT_DELETE,
                            _("Delete this tool group"), NULL,
                            G_CALLBACK (clay_tool_editor_delete_clicked),
                            NULL,
                            G_OBJECT (tool_editor));

  tool_editor->priv->reset_button =
    clay_editor_add_button (CLAY_EDITOR (tool_editor), CLAY_ICON_RESET,
                            _("Reset tool order and visibility"), NULL,
                            G_CALLBACK (clay_tool_editor_reset_clicked),
                            NULL,
                            G_OBJECT (tool_editor));

  clay_tool_editor_update_sensitivity (tool_editor);
}

static void
clay_tool_editor_set_container (ClayContainerView *container_view,
                                ClayContainer     *container)
{
  ClayToolEditor *tool_editor = CLAY_TOOL_EDITOR (container_view);

  parent_view_iface->set_container (container_view, container);

  clay_tool_editor_update_container (tool_editor);
}

static void
clay_tool_editor_set_context (ClayContainerView *container_view,
                              ClayContext       *context)
{
  ClayToolEditor *tool_editor = CLAY_TOOL_EDITOR (container_view);

  parent_view_iface->set_context (container_view, context);

  clay_tool_editor_update_container (tool_editor);
}

static gboolean
clay_tool_editor_set_selected (ClayContainerView *container_view,
                               GList             *items)
{
  ClayToolEditor *tool_editor = CLAY_TOOL_EDITOR (container_view);
  gboolean        result;

  result = parent_view_iface->set_selected (container_view, items);

  clay_tool_editor_update_sensitivity (tool_editor);

  return result;
}

static gboolean
clay_tool_editor_drop_possible (ClayContainerTreeView   *tree_view,
                                ClayDndType              src_type,
                                GList                   *src_viewables,
                                ClayViewable            *dest_viewable,
                                GtkTreePath             *drop_path,
                                GtkTreeViewDropPosition  drop_pos,
                                GtkTreeViewDropPosition *return_drop_pos,
                                GdkDragAction           *return_drag_action)
{
  if (CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (
        tree_view,
        src_type, src_viewables, dest_viewable, drop_path, drop_pos,
        return_drop_pos, return_drag_action))
    {
      if (clay_viewable_get_parent (dest_viewable)       ||
          (clay_viewable_get_children (dest_viewable)    &&
           (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
            drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)))
        {
          GList *iter;

          for (iter = src_viewables; iter; iter = iter->next)
            {
              ClayViewable *src_viewable = iter->data;

              if (clay_viewable_get_children (src_viewable))
                return FALSE;
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
clay_tool_editor_drop_viewables (ClayContainerTreeView   *tree_view,
                                 GList                   *src_viewables,
                                 ClayViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  ClayContainerView *container_view = CLAY_CONTAINER_VIEW (tree_view);

  CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (tree_view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);

  if (src_viewables)
    clay_container_view_set_selected (container_view, src_viewables);
}

static void
clay_tool_editor_new_group_clicked (GtkButton      *button,
                                    ClayToolEditor *tool_editor)
{
  ClayContainerView *container_view = CLAY_CONTAINER_VIEW (tool_editor);
  ClayContainer     *container;
  ClayToolItem      *tool_item;
  ClayToolGroup     *group;
  gint               index = 0;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      if (clay_viewable_get_parent (CLAY_VIEWABLE (tool_item)) != NULL)
        return;

      container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = clay_container_get_child_index (container,
                                              CLAY_OBJECT (tool_item));
    }
  else
    {
      container = tool_editor->priv->container;
    }

  if (container)
    {
      group = clay_tool_group_new ();

      clay_container_insert (container, CLAY_OBJECT (group), index);

      g_object_unref (group);

      clay_container_view_set_1_selected (container_view,
                                          CLAY_VIEWABLE (group));
    }
}

static void
clay_tool_editor_raise_clicked (GtkButton      *button,
                                ClayToolEditor *tool_editor)
{
  ClayToolItem *tool_item;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      ClayContainer *container;
      gint           index;

      container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = clay_container_get_child_index (container,
                                              CLAY_OBJECT (tool_item));

      if (index > 0)
        {
          clay_container_reorder (container,
                                  CLAY_OBJECT (tool_item), index - 1);
        }
    }
}

static void
clay_tool_editor_raise_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       ClayToolEditor  *tool_editor)
{
  ClayToolItem *tool_item;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item && (mask & GDK_SHIFT_MASK))
    {
      ClayContainer *container;
      gint           index;

      container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = clay_container_get_child_index (container,
                                              CLAY_OBJECT (tool_item));

      if (index > 0)
        {
          clay_container_reorder (container,
                                  CLAY_OBJECT (tool_item), 0);
        }
    }
}

static void
clay_tool_editor_lower_clicked (GtkButton      *button,
                                ClayToolEditor *tool_editor)
{
  ClayToolItem *tool_item;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      ClayContainer *container;
      gint           index;

      container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = clay_container_get_child_index (container,
                                              CLAY_OBJECT (tool_item));

      if (index + 1 < clay_container_get_n_children (container))
        {
          clay_container_reorder (container,
                                  CLAY_OBJECT (tool_item), index + 1);
        }
    }
}

static void
clay_tool_editor_lower_extend_clicked (GtkButton       *button,
                                       GdkModifierType  mask,
                                       ClayToolEditor  *tool_editor)
{
  ClayToolItem *tool_item;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item && (mask & GDK_SHIFT_MASK))
    {
      ClayContainer *container;
      gint           index;

      container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = clay_container_get_n_children (container) - 1;
      index = MAX (index, 0);

      clay_container_reorder (container,
                              CLAY_OBJECT (tool_item), index);
    }
}

static void
clay_tool_editor_delete_clicked (GtkButton      *button,
                                 ClayToolEditor *tool_editor)
{
  ClayContainerView *container_view = CLAY_CONTAINER_VIEW (tool_editor);
  ClayToolItem     *tool_item;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      ClayContainer *src_container;
      ClayContainer *dest_container;
      gint           index;
      gint           dest_index;

      src_container  = clay_viewable_get_children (CLAY_VIEWABLE (tool_item));
      dest_container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                                 tool_item);

      if (! src_container)
        return;

      index      = clay_container_get_child_index (dest_container,
                                                   CLAY_OBJECT (tool_item));
      dest_index = index;

      g_object_ref (tool_item);

      clay_container_freeze (src_container);
      clay_container_freeze (dest_container);

      clay_container_remove (dest_container, CLAY_OBJECT (tool_item));

      while (! clay_container_is_empty (src_container))
        {
          ClayObject *object = clay_container_get_first_child (src_container);

          g_object_ref (object);

          clay_container_remove (src_container,  object);
          clay_container_insert (dest_container, object, dest_index++);

          g_object_unref (object);
        }

      clay_container_thaw (dest_container);
      clay_container_thaw (src_container);

      clay_container_view_set_1_selected
        (container_view,
         CLAY_VIEWABLE (clay_container_get_child_by_index (dest_container,
                                                           index)));

      g_object_unref (tool_item);
    }
}

static void
clay_tool_editor_reset_clicked (GtkButton      *button,
                                ClayToolEditor *tool_editor)
{
  clay_tools_reset (tool_editor->priv->context->clay,
                    tool_editor->priv->container,
                    FALSE);
}

static void
clay_tool_editor_tool_item_notify (ClayToolItem   *tool_item,
                                   GParamSpec     *pspec,
                                   ClayToolEditor *tool_editor)
{
  ClayContainerTreeView *tree_view      = CLAY_CONTAINER_TREE_VIEW (tool_editor);
  ClayContainerView     *container_view = CLAY_CONTAINER_VIEW (tool_editor);
  GtkTreeIter           *iter;

  iter = _clay_container_view_lookup (container_view,
                                     CLAY_VIEWABLE (tool_item));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
clay_tool_editor_eye_data_func (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer   *cell,
                                GtkTreeModel      *tree_model,
                                GtkTreeIter       *iter,
                                gpointer           data)
{
  ClayViewRenderer *renderer;
  ClayToolItem     *tool_item;

  gtk_tree_model_get (tree_model, iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  tool_item = CLAY_TOOL_ITEM (renderer->viewable);

  g_object_set (cell,
                "active",       clay_tool_item_get_visible (tool_item),
                "inconsistent", clay_tool_item_get_visible (tool_item) &&
                                ! clay_tool_item_get_shown (tool_item),
                NULL);

  g_object_unref (renderer);
}

static void
clay_tool_editor_eye_clicked (GtkCellRendererToggle *toggle,
                              gchar                 *path_str,
                              GdkModifierType        state,
                              ClayToolEditor        *tool_editor)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (tool_editor);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayViewRenderer *renderer;
      ClayToolItem     *tool_item;
      gboolean          active;

      gtk_tree_model_get (tree_view->model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      tool_item = CLAY_TOOL_ITEM (renderer->viewable);

      g_object_get (toggle,
                    "active", &active,
                    NULL);

      clay_tool_item_set_visible (tool_item, ! active);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}

static ClayToolItem *
clay_tool_editor_get_selected_tool_item (ClayToolEditor *tool_editor)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (tool_editor);

  if (tool_editor->priv->container)
    {
      ClayViewRenderer *renderer;
      ClayToolItem     *tool_item;
      GtkTreeSelection *selection;
      GtkTreeModel     *model;
      GtkTreeIter       iter;

      selection = gtk_tree_view_get_selection (tree_view->view);

      if (! gtk_tree_selection_get_selected (selection, &model, &iter))
        return NULL;

      gtk_tree_model_get (model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      tool_item = CLAY_TOOL_ITEM (renderer->viewable);

      g_object_unref (renderer);

      return tool_item;
    }

  return NULL;
}

static ClayContainer *
clay_tool_editor_get_tool_item_container (ClayToolEditor *tool_editor,
                                          ClayToolItem   *tool_item)
{
  ClayViewable *parent;

  parent = clay_viewable_get_parent (CLAY_VIEWABLE (tool_item));

  if (parent)
    {
      return clay_viewable_get_children (parent);
    }
  else
    {
      return tool_editor->priv->container;
    }
}

static void
clay_tool_editor_update_container (ClayToolEditor *tool_editor)
{
  ClayContainerView *container_view = CLAY_CONTAINER_VIEW (tool_editor);
  ClayContainer     *container;
  ClayContext       *context;

  g_clear_pointer (&tool_editor->priv->tool_item_notify_handler,
                   clay_tree_handler_disconnect);

  g_clear_pointer (&tool_editor->priv->initial_tool_state, g_free);

  container = clay_container_view_get_container (container_view);
  context   = clay_container_view_get_context   (container_view);

  if (container && context)
    {
      GString          *string;
      ClayConfigWriter *writer;

      tool_editor->priv->container = container;
      tool_editor->priv->context   = context;

      tool_editor->priv->tool_item_notify_handler = clay_tree_handler_connect (
        container, "notify",
        G_CALLBACK (clay_tool_editor_tool_item_notify),
        tool_editor);

      /* save initial tool order */
      string = g_string_new (NULL);

      writer = clay_config_writer_new_from_string (string);

      clay_tools_serialize (context->clay, container, writer);

      clay_config_writer_finish (writer, NULL, NULL);

      tool_editor->priv->initial_tool_state = g_string_free (string, FALSE);
    }
}

static void
clay_tool_editor_update_sensitivity (ClayToolEditor *tool_editor)
{
  ClayToolItem *tool_item;

  tool_item = clay_tool_editor_get_selected_tool_item (tool_editor);

  if (tool_item)
    {
      ClayContainer *container;
      gint           index;

      container = clay_tool_editor_get_tool_item_container (tool_editor,
                                                            tool_item);

      index = clay_container_get_child_index (container,
                                              CLAY_OBJECT (tool_item));

      gtk_widget_set_sensitive (
        tool_editor->priv->new_group_button,
        clay_viewable_get_parent (CLAY_VIEWABLE (tool_item)) == NULL);

      gtk_widget_set_sensitive (
        tool_editor->priv->raise_button,
        index > 0);

      gtk_widget_set_sensitive (
        tool_editor->priv->lower_button,
        index < clay_container_get_n_children (container) - 1);

      gtk_widget_set_sensitive (
        tool_editor->priv->delete_button,
        clay_viewable_get_children (CLAY_VIEWABLE (tool_item)) != NULL);
    }
  else
    {
      gtk_widget_set_sensitive (tool_editor->priv->new_group_button, TRUE);
      gtk_widget_set_sensitive (tool_editor->priv->raise_button,     FALSE);
      gtk_widget_set_sensitive (tool_editor->priv->lower_button,     FALSE);
      gtk_widget_set_sensitive (tool_editor->priv->delete_button,    FALSE);
    }
}


/*  public functions  */

GtkWidget *
clay_tool_editor_new (ClayContainer *container,
                      ClayContext   *context,
                      gint           view_size,
                      gint           view_border_width)
{
  ClayContainerView *container_view;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  container_view = g_object_new (CLAY_TYPE_TOOL_EDITOR,
                                 "view-size",         view_size,
                                 "view-border-width", view_border_width,
                                 NULL);

  clay_container_view_set_context     (container_view, context);
  clay_container_view_set_container   (container_view, container);
  clay_container_view_set_reorderable (container_view, TRUE);

  return GTK_WIDGET (container_view);
}

/**
 * clay_tool_editor_revert_changes:
 * @tool_editor:
 *
 * Reverts the tool order and visibility to the state at creation.
 **/
void
clay_tool_editor_revert_changes (ClayToolEditor *tool_editor)
{
  GScanner *scanner;

  g_return_if_fail (CLAY_IS_TOOL_EDITOR (tool_editor));

  scanner = clay_scanner_new_string (tool_editor->priv->initial_tool_state, -1,
                                     NULL);

  clay_tools_deserialize (tool_editor->priv->context->clay,
                          tool_editor->priv->container,
                          scanner);

  clay_scanner_unref (scanner);
}
