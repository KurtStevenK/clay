/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainericonview.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "claycellrendererviewable.h"
#include "claycontainertreestore.h"
#include "claycontainericonview.h"
#include "claycontainerview.h"
#include "claycontainerview-cruft.h"
#include "claydnd.h"
#include "clayviewrenderer.h"
#include "claywidgets-utils.h"


struct _ClayContainerIconViewPrivate
{
  ClayViewRenderer *dnd_renderer;

  gulong            color_scheme_handler_id;
};


static void          clay_container_icon_view_view_iface_init   (ClayContainerViewInterface  *iface);

static void          clay_container_icon_view_constructed       (GObject                     *object);
static void          clay_container_icon_view_finalize          (GObject                     *object);

static void          clay_container_icon_view_unmap             (GtkWidget                   *widget);
static gboolean      clay_container_icon_view_popup_menu        (GtkWidget                   *widget);

static void          clay_container_icon_view_set_container     (ClayContainerView           *view,
                                                                 ClayContainer               *container);
static void          clay_container_icon_view_set_context       (ClayContainerView           *view,
                                                                 ClayContext                 *context);
static void          clay_container_icon_view_set_selection_mode(ClayContainerView           *view,
                                                                 GtkSelectionMode             mode);
static void          clay_container_icon_view_set_view_size     (ClayContainerView           *view);
static gboolean      clay_container_icon_view_set_selected      (ClayContainerView           *view,
                                                                 GList                       *items);
static gint          clay_container_icon_view_get_selected        (ClayContainerView      *view,
                                                                   GList                 **items);

static gpointer      clay_container_icon_view_insert_item       (ClayContainerView           *view,
                                                                 ClayViewable                *viewable,
                                                                 gpointer                     parent_insert_data,
                                                                 gint                         index);
static void          clay_container_icon_view_remove_item       (ClayContainerView           *view,
                                                                 ClayViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          clay_container_icon_view_reorder_item      (ClayContainerView           *view,
                                                                 ClayViewable                *viewable,
                                                                 gint                         new_index,
                                                                 gpointer                     insert_data);
static void          clay_container_icon_view_rename_item       (ClayContainerView           *view,
                                                                 ClayViewable                *viewable,
                                                                 gpointer                     insert_data);
static void          clay_container_icon_view_clear_items       (ClayContainerView           *view);

static void          clay_container_icon_view_invalidate        (ClayContainerIconView       *view);

static void          clay_container_icon_view_selection_changed (GtkIconView                 *view,
                                                                 ClayContainerIconView       *icon_view);
static void          clay_container_icon_view_item_activated    (GtkIconView                 *view,
                                                                 GtkTreePath                 *path,
                                                                 ClayContainerIconView       *icon_view);
static gboolean      clay_container_icon_view_button_press      (GtkWidget                   *widget,
                                                                 GdkEventButton              *bevent,
                                                                 ClayContainerIconView       *icon_view);
static gboolean      clay_container_icon_view_tooltip           (GtkWidget                   *widget,
                                                                 gint                         x,
                                                                 gint                         y,
                                                                 gboolean                     keyboard_tip,
                                                                 GtkTooltip                  *tooltip,
                                                                 ClayContainerIconView       *icon_view);

static ClayViewable * clay_container_icon_view_drag_viewable    (GtkWidget    *widget,
                                                                 ClayContext **context,
                                                                 gpointer      data);
static GdkPixbuf   * clay_container_icon_view_drag_pixbuf       (GtkWidget *widget,
                                                                 gpointer   data);
static gboolean      clay_container_icon_view_get_1_selected    (ClayContainerIconView  *icon_view,
                                                                 GtkTreeIter            *iter);

static void          clay_container_icon_view_trigger_redraw    (ClayContainerIconView *view);


G_DEFINE_TYPE_WITH_CODE (ClayContainerIconView, clay_container_icon_view,
                         CLAY_TYPE_CONTAINER_BOX,
                         G_ADD_PRIVATE (ClayContainerIconView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_container_icon_view_view_iface_init))

#define parent_class clay_container_icon_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_container_icon_view_class_init (ClayContainerIconViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = clay_container_icon_view_constructed;
  object_class->finalize    = clay_container_icon_view_finalize;

  widget_class->unmap       = clay_container_icon_view_unmap;
  widget_class->popup_menu  = clay_container_icon_view_popup_menu;

  gtk_widget_class_set_css_name (widget_class, "ClayContainerIconView");
}

static void
clay_container_icon_view_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (CLAY_TYPE_CONTAINER_VIEW);

  iface->set_container      = clay_container_icon_view_set_container;
  iface->set_context        = clay_container_icon_view_set_context;
  iface->set_selection_mode = clay_container_icon_view_set_selection_mode;
  iface->set_view_size      = clay_container_icon_view_set_view_size;
  iface->set_selected       = clay_container_icon_view_set_selected;
  iface->get_selected       = clay_container_icon_view_get_selected;

  iface->insert_item        = clay_container_icon_view_insert_item;
  iface->remove_item        = clay_container_icon_view_remove_item;
  iface->reorder_item       = clay_container_icon_view_reorder_item;
  iface->rename_item        = clay_container_icon_view_rename_item;
  iface->clear_items        = clay_container_icon_view_clear_items;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
clay_container_icon_view_init (ClayContainerIconView *icon_view)
{
  ClayContainerBox *box = CLAY_CONTAINER_BOX (icon_view);

  icon_view->priv = clay_container_icon_view_get_instance_private (icon_view);

  icon_view->priv->color_scheme_handler_id = 0;

  clay_container_tree_store_columns_init (icon_view->model_columns,
                                          &icon_view->n_model_columns);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
}

static void
clay_container_icon_view_constructed (GObject *object)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (object);
  ClayContainerView     *view      = CLAY_CONTAINER_VIEW (object);
  ClayContainerBox      *box       = CLAY_CONTAINER_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  icon_view->model = clay_container_tree_store_new (view,
                                                    icon_view->n_model_columns,
                                                    icon_view->model_columns);

  icon_view->view = g_object_new (GTK_TYPE_ICON_VIEW,
                                  "model",          icon_view->model,
                                  "row-spacing",    0,
                                  "column-spacing", 0,
                                  "margin",         0,
                                  "item-padding",   1,
                                  "has-tooltip",    TRUE,
                                  NULL);
  g_object_unref (icon_view->model);

  gtk_container_add (GTK_CONTAINER (box->scrolled_win),
                     GTK_WIDGET (icon_view->view));
  gtk_widget_set_visible (GTK_WIDGET (icon_view->view), TRUE);

  clay_container_view_set_dnd_widget (view, GTK_WIDGET (icon_view->view));

  icon_view->renderer_cell = clay_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view->view),
                              icon_view->renderer_cell,
                              FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view->view),
                                  icon_view->renderer_cell,
                                  "renderer", CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  NULL);

  clay_container_tree_store_add_renderer_cell (CLAY_CONTAINER_TREE_STORE (icon_view->model),
                                               icon_view->renderer_cell, -1);

  g_signal_connect (icon_view->view, "selection-changed",
                    G_CALLBACK (clay_container_icon_view_selection_changed),
                    icon_view);
  g_signal_connect (icon_view->view, "item-activated",
                    G_CALLBACK (clay_container_icon_view_item_activated),
                    icon_view);
  g_signal_connect (icon_view->view, "query-tooltip",
                    G_CALLBACK (clay_container_icon_view_tooltip),
                    icon_view);
}

static void
clay_container_icon_view_finalize (GObject *object)
{
  //ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_container_icon_view_unmap (GtkWidget *widget)
{
  //ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static gboolean
clay_container_icon_view_popup_menu (GtkWidget *widget)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (widget);
  GtkTreeIter            iter;
  GtkTreePath           *path;
  GdkRectangle           rect;

  if (! clay_container_icon_view_get_1_selected (icon_view, &iter))
    return FALSE;

  path = gtk_tree_model_get_path (icon_view->model, &iter);
  gtk_icon_view_get_cell_rect (icon_view->view, path, NULL, &rect);
  gtk_tree_path_free (path);

  return clay_editor_popup_menu_at_rect (CLAY_EDITOR (widget),
                                         gtk_widget_get_window (GTK_WIDGET (icon_view->view)),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

GtkWidget *
clay_container_icon_view_new (ClayContainer *container,
                              ClayContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  ClayContainerIconView *icon_view;
  ClayContainerView     *view;

  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  icon_view = g_object_new (CLAY_TYPE_CONTAINER_ICON_VIEW, NULL);

  view = CLAY_CONTAINER_VIEW (icon_view);

  clay_container_view_set_view_size (view, view_size, 0 /* ignore border */);

  if (context)
    clay_container_view_set_context (view, context);

  if (container)
    clay_container_view_set_container (view, container);

  return GTK_WIDGET (icon_view);
}


/*  ClayContainerView methods  */

static void
clay_container_icon_view_set_container (ClayContainerView *view,
                                        ClayContainer     *container)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);
  ClayContainer         *old_container;

  old_container = clay_container_view_get_container (view);

  if (old_container)
    {
      if (! container)
        {
          if (clay_dnd_viewable_source_remove (GTK_WIDGET (icon_view->view),
                                               clay_container_get_child_type (old_container)))
            {
              if (CLAY_VIEWABLE_CLASS (g_type_class_peek (clay_container_get_child_type (old_container)))->get_size)
                clay_dnd_pixbuf_source_remove (GTK_WIDGET (icon_view->view));

              gtk_drag_source_unset (GTK_WIDGET (icon_view->view));
            }

          g_signal_handlers_disconnect_by_func (icon_view->view,
                                                clay_container_icon_view_button_press,
                                                icon_view);
        }
    }
  else if (container)
    {
      if (clay_dnd_drag_source_set_by_type (GTK_WIDGET (icon_view->view),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            clay_container_get_child_type (container),
                                            GDK_ACTION_COPY))
        {
          clay_dnd_viewable_source_add (GTK_WIDGET (icon_view->view),
                                        clay_container_get_child_type (container),
                                        clay_container_icon_view_drag_viewable,
                                        icon_view);

          if (CLAY_VIEWABLE_CLASS (g_type_class_peek (clay_container_get_child_type (container)))->get_size)
            clay_dnd_pixbuf_source_add (GTK_WIDGET (icon_view->view),
                                        clay_container_icon_view_drag_pixbuf,
                                        icon_view);
        }

      g_signal_connect_object (icon_view->view, "button-press-event",
                               G_CALLBACK (clay_container_icon_view_button_press),
                               icon_view,
                               0);
    }

  parent_view_iface->set_container (view, container);
}

static void
clay_container_icon_view_set_context (ClayContainerView *view,
                                      ClayContext       *context)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (icon_view->model)
    clay_container_tree_store_set_context (CLAY_CONTAINER_TREE_STORE (icon_view->model),
                                           context);

  if (context)
    {
      if (! icon_view->priv->color_scheme_handler_id)
        icon_view->priv->color_scheme_handler_id =
          g_signal_connect_object (context->clay->config,
                                   "notify::theme-color-scheme",
                                   G_CALLBACK (clay_container_icon_view_trigger_redraw),
                                   icon_view, G_CONNECT_SWAPPED);
    }
}

static void
clay_container_icon_view_set_selection_mode (ClayContainerView *view,
                                             GtkSelectionMode   mode)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);

  gtk_icon_view_set_selection_mode (icon_view->view, mode);

  parent_view_iface->set_selection_mode (view, mode);
}

static void
clay_container_icon_view_set_view_size (ClayContainerView *view)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);

  if (icon_view->model)
    clay_container_tree_store_set_view_size (CLAY_CONTAINER_TREE_STORE (icon_view->model));

  if (icon_view->view)
    {
      gtk_icon_view_set_columns (icon_view->view, -1);
      gtk_icon_view_set_item_width (icon_view->view, -1);

      /* ugly workaround to force the icon view to invalidate all its
       * cached icon sizes
       */
      gtk_icon_view_set_item_orientation (icon_view->view,
                                          GTK_ORIENTATION_VERTICAL);
      gtk_icon_view_set_item_orientation (icon_view->view,
                                          GTK_ORIENTATION_HORIZONTAL);
    }
}

static gboolean
clay_container_icon_view_set_selected (ClayContainerView *view,
                                       GList             *items)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);
  GList                 *list;

  if (items)
    {
      GList *paths = NULL;

      for (list = items; list; list = g_list_next (list))
        {
          GtkTreeIter *iter;
          GtkTreePath *path;

          iter = _clay_container_view_lookup (CLAY_CONTAINER_VIEW (view),
                                              list->data);
          if (! iter)
            /* This may happen when the ClayContainerIconView only
             * shows a subpart of the whole icons. We don't select
             * what is not shown.
             */
            continue;

          path = gtk_tree_model_get_path (icon_view->model, iter);

          paths = g_list_prepend (paths, path);
        }

      paths = g_list_reverse (paths);

      g_signal_handlers_block_by_func (icon_view->view,
                                       clay_container_icon_view_selection_changed,
                                       icon_view);

      gtk_icon_view_unselect_all (icon_view->view);

      for (list = paths; list; list = g_list_next (list))
        {
          gtk_icon_view_select_path (icon_view->view, list->data);
        }

      if (paths)
        {
          gtk_icon_view_set_cursor (icon_view->view, paths->data, NULL, FALSE);
          gtk_icon_view_scroll_to_path (icon_view->view, paths->data,
                                        FALSE, 0.0, 0.0);
        }

      g_signal_handlers_unblock_by_func (icon_view->view,
                                         clay_container_icon_view_selection_changed,
                                         icon_view);

      g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);

      _clay_container_view_selection_changed (CLAY_CONTAINER_VIEW (icon_view));
    }
  else
    {
      gtk_icon_view_unselect_all (icon_view->view);
    }

  return TRUE;
}

static gint
clay_container_icon_view_get_selected (ClayContainerView  *view,
                                       GList             **items)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);
  GList                 *paths;
  gint                   n_selected;

  paths = gtk_icon_view_get_selected_items (icon_view->view);
  n_selected = g_list_length (paths);

  if (items)
    {
      GList *list;

      for (list = paths; list; list = g_list_next (list))
        {
          GtkTreeIter       iter;
          ClayViewRenderer *renderer;

          gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), &iter,
                                   (GtkTreePath *) list->data);

          gtk_tree_model_get (icon_view->model, &iter,
                              CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable)
            *items = g_list_prepend (*items, renderer->viewable);

          g_object_unref (renderer);
        }

      *items = g_list_reverse (*items);
    }

  g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);

  return n_selected;
}

static gpointer
clay_container_icon_view_insert_item (ClayContainerView *view,
                                      ClayViewable      *viewable,
                                      gpointer           parent_insert_data,
                                      gint               index)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter;

  iter = clay_container_tree_store_insert_item (CLAY_CONTAINER_TREE_STORE (icon_view->model),
                                                viewable,
                                                parent_insert_data,
                                                index);

  if (parent_insert_data)
    {
#if 0
      GtkTreePath *path = gtk_tree_model_get_path (icon_view->model, iter);

      gtk_icon_view_expand_to_path (icon_view->view, path);

      gtk_tree_path_free (path);
#endif
    }

  return iter;
}

static void
clay_container_icon_view_remove_item (ClayContainerView *view,
                                      ClayViewable      *viewable,
                                      gpointer           insert_data)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);

  clay_container_tree_store_remove_item (CLAY_CONTAINER_TREE_STORE (icon_view->model),
                                         viewable,
                                         insert_data);
}

static void
clay_container_icon_view_reorder_item (ClayContainerView *view,
                                       ClayViewable      *viewable,
                                       gint               new_index,
                                       gpointer           insert_data)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;
  gboolean               selected  = FALSE;

  if (iter)
    {
      GtkTreeIter selected_iter;

      selected = clay_container_icon_view_get_1_selected (icon_view,
                                                          &selected_iter);

      if (selected)
        {
          ClayViewRenderer *renderer;

          gtk_tree_model_get (icon_view->model, &selected_iter,
                              CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);

          if (renderer->viewable != viewable)
            selected = FALSE;

          g_object_unref (renderer);
        }
    }

  clay_container_tree_store_reorder_item (CLAY_CONTAINER_TREE_STORE (icon_view->model),
                                          viewable,
                                          new_index,
                                          iter);

  if (selected)
    clay_container_view_set_1_selected (view, viewable);
}

static void
clay_container_icon_view_rename_item (ClayContainerView *view,
                                      ClayViewable      *viewable,
                                      gpointer           insert_data)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);
  GtkTreeIter           *iter      = (GtkTreeIter *) insert_data;

  clay_container_tree_store_rename_item (CLAY_CONTAINER_TREE_STORE (icon_view->model),
                                         viewable,
                                         iter);
}

static void
clay_container_icon_view_clear_items (ClayContainerView *view)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (view);

  clay_container_tree_store_clear_items (CLAY_CONTAINER_TREE_STORE (icon_view->model));

  parent_view_iface->clear_items (view);
}

static void
clay_container_icon_view_invalidate (ClayContainerIconView *view)
{
  GtkTreeIter iter;
  gboolean    iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (view->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (view->model, &iter))
    {
      ClayViewRenderer *renderer;

      gtk_tree_model_get (view->model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      clay_view_renderer_invalidate (renderer);

      g_object_unref (renderer);
    }
}


/*  callbacks  */

static void
clay_container_icon_view_selection_changed (GtkIconView           *gtk_icon_view,
                                            ClayContainerIconView *icon_view)
{
  _clay_container_view_selection_changed (CLAY_CONTAINER_VIEW (icon_view));
}

static void
clay_container_icon_view_item_activated (GtkIconView           *view,
                                         GtkTreePath           *path,
                                         ClayContainerIconView *icon_view)
{
  GtkTreeIter       iter;
  ClayViewRenderer *renderer;

  gtk_tree_model_get_iter (icon_view->model, &iter, path);

  gtk_tree_model_get (icon_view->model, &iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  _clay_container_view_item_activated (CLAY_CONTAINER_VIEW (icon_view),
                                       renderer->viewable);

  g_object_unref (renderer);
}

static gboolean
clay_container_icon_view_button_press (GtkWidget             *widget,
                                       GdkEventButton        *bevent,
                                       ClayContainerIconView *icon_view)
{
  ClayContainerView *container_view = CLAY_CONTAINER_VIEW (icon_view);
  GtkTreePath       *path;
  gboolean           handled        = GDK_EVENT_PROPAGATE;

  icon_view->priv->dnd_renderer = NULL;

  path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (widget),
                                        bevent->x, bevent->y);

  if (path)
    {
      ClayViewRenderer *renderer;
      GtkTreeIter       iter;

      gtk_tree_model_get_iter (icon_view->model, &iter, path);

      gtk_tree_model_get (icon_view->model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      icon_view->priv->dnd_renderer = renderer;

      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          /* If the clicked item is not selected, it becomes the new
           * selection. Otherwise, we use the current selection. This
           * allows to not break multiple selection when right-clicking.
           */
          if (! clay_container_view_is_item_selected (container_view,
                                                      renderer->viewable))
            {
              clay_container_view_set_1_selected (container_view,
                                                  renderer->viewable);
            }

          /* Show the context menu. */
          handled = clay_editor_popup_menu_at_pointer (CLAY_EDITOR (icon_view),
                                                       (GdkEvent *) bevent);
        }
      /* Else LMB down or similar.  Propagate. */

      g_object_unref (renderer);

      gtk_tree_path_free (path);
    }
  else
    {
      /* Button down outside any item. */
      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          (void) clay_editor_popup_menu_at_pointer (CLAY_EDITOR (icon_view),
                                                    (GdkEvent *) bevent);
          /* Discard result.  Does not actually popup menu except for managed views, e.g. dockable. */
        }

      /* Else LMB down or similar.
       * Say we handled event even though we did nothing.
       * Otherwise, selection is set to None.
       */
      handled = GDK_EVENT_STOP;
    }

  return handled;
}

static gboolean
clay_container_icon_view_tooltip (GtkWidget             *widget,
                                  gint                   x,
                                  gint                   y,
                                  gboolean               keyboard_tip,
                                  GtkTooltip            *tooltip,
                                  ClayContainerIconView *icon_view)
{
  ClayViewRenderer *renderer;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  gboolean          show_tip = FALSE;

  if (! gtk_icon_view_get_tooltip_context (GTK_ICON_VIEW (widget), &x, &y,
                                           keyboard_tip,
                                           NULL, &path, &iter))
    return FALSE;

  gtk_tree_model_get (icon_view->model, &iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);

  if (renderer)
    {
      gchar *desc;
      gchar *tip;

      desc = clay_viewable_get_description (renderer->viewable, &tip);

      if (tip)
        {
          gtk_tooltip_set_text (tooltip, tip);
          gtk_icon_view_set_tooltip_cell (GTK_ICON_VIEW (widget), tooltip, path,
                                          icon_view->renderer_cell);

          show_tip = TRUE;

          g_free (tip);
        }

      g_free (desc);
      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);

  return show_tip;
}

static ClayViewable *
clay_container_icon_view_drag_viewable (GtkWidget    *widget,
                                        ClayContext **context,
                                        gpointer      data)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (data);

  if (context)
    *context = clay_container_view_get_context (CLAY_CONTAINER_VIEW (data));

  if (icon_view->priv->dnd_renderer)
    return icon_view->priv->dnd_renderer->viewable;

  return NULL;
}

static GdkPixbuf *
clay_container_icon_view_drag_pixbuf (GtkWidget *widget,
                                      gpointer   data)
{
  ClayContainerIconView *icon_view = CLAY_CONTAINER_ICON_VIEW (data);
  ClayViewRenderer      *renderer  = icon_view->priv->dnd_renderer;
  gint                   width;
  gint                   height;

  if (renderer && clay_viewable_get_size (renderer->viewable, &width, &height))
    return clay_viewable_get_new_pixbuf (renderer->viewable,
                                         renderer->context,
                                         width, height, 1, NULL);

  return NULL;
}

static gboolean
clay_container_icon_view_get_1_selected (ClayContainerIconView *icon_view,
                                         GtkTreeIter           *iter)
{
  GList    *selected_items;
  gboolean  retval;

  selected_items = gtk_icon_view_get_selected_items (icon_view->view);

  if (g_list_length (selected_items) == 1)
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (icon_view->model), iter,
                               selected_items->data);

      retval = TRUE;
    }
  else
    {
      retval = FALSE;
    }

  g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);

  return retval;
}

static void
clay_container_icon_view_trigger_redraw (ClayContainerIconView *view)
{
  clay_container_icon_view_invalidate (view);
  gtk_widget_queue_draw (GTK_WIDGET (view));
}
