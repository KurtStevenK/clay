/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerlistview.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "claycontainerlistview.h"
#include "claycontainerview.h"
#include "clayrow.h"
#include "clayrow-utils.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


typedef struct _ClayContainerListViewPrivate ClayContainerListViewPrivate;

struct _ClayContainerListViewPrivate
{
  GtkListBox *view;

  gboolean    reordering;
  GList      *reordering_selected;
};

#define GET_PRIVATE(obj) \
  ((ClayContainerListViewPrivate *) clay_container_list_view_get_instance_private ((ClayContainerListView *) obj))


static void     clay_container_list_view_view_iface_init   (ClayContainerViewInterface *iface);

static void     clay_container_list_view_constructed       (GObject               *object);
static void     clay_container_list_view_finalize          (GObject               *object);

static gboolean clay_container_list_view_popup_menu        (GtkWidget             *widget);

static void     clay_container_list_view_set_container     (ClayContainerView     *view,
                                                            ClayContainer         *container);
static void     clay_container_list_view_set_context       (ClayContainerView     *view,
                                                            ClayContext           *context);
static void     clay_container_list_view_set_selection_mode(ClayContainerView     *view,
                                                            GtkSelectionMode       mode);
static void     clay_container_list_view_set_view_size     (ClayContainerView     *view);
static gboolean clay_container_list_view_set_selected      (ClayContainerView     *view,
                                                            GList                 *items);
static gint     clay_container_list_view_get_selected      (ClayContainerView     *view,
                                                            GList                **items);

static void     clay_container_list_view_reorder           (ClayContainer         *container,
                                                            ClayObject            *object,
                                                            gint                   old_index,
                                                            gint                   new_index,
                                                            ClayContainerView     *view);
static void     clay_container_list_view_before_items_changed
                                                           (ClayContainer         *container,
                                                            guint                  position,
                                                            guint                  removed,
                                                            guint                  added,
                                                            ClayContainerView     *view);
static void     clay_container_list_view_after_items_changed
                                                           (ClayContainer         *container,
                                                            guint                  position,
                                                            guint                  removed,
                                                            guint                  added,
                                                            ClayContainerView     *view);

static void     clay_container_list_view_selected_rows_changed
                                                            (GtkListBox            *box,
                                                             ClayContainerListView *list_view);
static void     clay_container_list_view_row_activated      (GtkListBox            *box,
                                                             ClayRow               *row,
                                                             ClayContainerListView *list_view);

static void     clay_container_list_view_monitor_changed    (ClayContainerListView *view);


G_DEFINE_TYPE_WITH_CODE (ClayContainerListView,
                         clay_container_list_view,
                         CLAY_TYPE_CONTAINER_BOX,
                         G_ADD_PRIVATE (ClayContainerListView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_container_list_view_view_iface_init))

#define parent_class clay_container_list_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_container_list_view_class_init (ClayContainerListViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = clay_container_list_view_constructed;
  object_class->finalize      = clay_container_list_view_finalize;

  widget_class->popup_menu    = clay_container_list_view_popup_menu;

  klass->create_row_func      = clay_row_create_for_container_view;
}

static void
clay_container_list_view_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (CLAY_TYPE_CONTAINER_VIEW);

  iface->set_container      = clay_container_list_view_set_container;
  iface->set_context        = clay_container_list_view_set_context;
  iface->set_selection_mode = clay_container_list_view_set_selection_mode;
  iface->set_view_size      = clay_container_list_view_set_view_size;
  iface->set_selected       = clay_container_list_view_set_selected;
  iface->get_selected       = clay_container_list_view_get_selected;

  iface->use_list_model     = TRUE;
}

static void
clay_container_list_view_init (ClayContainerListView *list_view)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (list_view);
  ClayContainerBox             *box  = CLAY_CONTAINER_BOX (list_view);

  priv->view = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_list_box_set_activate_on_single_click (priv->view, FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (priv->view)),
                               "view");
  gtk_container_add (GTK_CONTAINER (box->scrolled_win),
                     GTK_WIDGET (priv->view));
  gtk_widget_set_visible (GTK_WIDGET (priv->view), TRUE);

  g_signal_connect (priv->view, "selected-rows-changed",
                    G_CALLBACK (clay_container_list_view_selected_rows_changed),
                    list_view);
  g_signal_connect (priv->view, "row-activated",
                    G_CALLBACK (clay_container_list_view_row_activated),
                    list_view);

  clay_container_view_set_dnd_widget (CLAY_CONTAINER_VIEW (list_view),
                                      GTK_WIDGET (priv->view));

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box->scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  clay_widget_track_monitor (GTK_WIDGET (list_view),
                             G_CALLBACK (clay_container_list_view_monitor_changed),
                             NULL, NULL);
}

static void
clay_container_list_view_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
clay_container_list_view_finalize (GObject *object)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (object);

  g_list_free_full (priv->reordering_selected, g_object_unref);
  priv->reordering_selected = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_container_list_view_popup_menu (GtkWidget *widget)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (widget);
  GList                        *rows;
  GtkWidget                    *row;
  GtkAllocation                 allocation;

  rows = gtk_list_box_get_selected_rows (priv->view);

  if (g_list_length (rows) != 1)
    {
      g_list_free (rows);

      return FALSE;
    }

  row = rows->data;

  g_list_free (rows);

  gtk_widget_get_allocation (row, &allocation);

  return clay_editor_popup_menu_at_rect (CLAY_EDITOR (widget),
                                         gtk_widget_get_window (row),
                                         (GdkRectangle *) &allocation,
                                         GDK_GRAVITY_CENTER,
                                         GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

GtkWidget *
clay_container_list_view_new (ClayContainer *container,
                              ClayContext   *context,
                              gint           view_size,
                              gint           view_border_width)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (CLAY_TYPE_CONTAINER_LIST_VIEW,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "context",           context,
                       "container",         container,
                       NULL);
}

void
clay_container_list_view_set_search_entry (ClayContainerListView *list_view,
                                           GtkSearchEntry        *entry)
{
  g_return_if_fail (CLAY_IS_CONTAINER_LIST_VIEW (list_view));
  g_return_if_fail (GTK_IS_SEARCH_ENTRY (entry));

  g_printerr ("implement clay_container_list_view_set_search_entry()\n");
}


/*  ClayContainerView methods  */

static void
clay_container_list_view_set_container (ClayContainerView *view,
                                        ClayContainer     *container)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);
  ClayContainer                *old_container;

  old_container = clay_container_view_get_container (view);

  if (old_container)
    {
      g_signal_handlers_disconnect_by_func (old_container,
                                            clay_container_list_view_reorder,
                                            view);
      g_signal_handlers_disconnect_by_func (old_container,
                                            clay_container_list_view_before_items_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (old_container,
                                            clay_container_list_view_after_items_changed,
                                            view);

#if 0
      if (! container)
        {
          if (clay_dnd_viewable_list_source_remove (GTK_WIDGET (list_view->view),
                                                    clay_container_get_child_type (old_container)))
            {
              gtk_drag_source_unset (GTK_WIDGET (list_view->view));
            }
        }
#endif

      gtk_list_box_bind_model (priv->view, NULL, NULL, NULL, NULL);
    }
  else if (container)
    {
#if 0
      if (clay_dnd_drag_source_set_by_type (GTK_WIDGET (list_view->view),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            clay_container_get_child_type (container),
                                            GDK_ACTION_COPY))
        {
          clay_dnd_viewable_list_source_add (GTK_WIDGET (list_view->view),
                                             clay_container_get_child_type (container),
                                             clay_container_list_view_drag_viewable_list,
                                             list_view);
        }
#endif
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      ClayContainerListViewClass *view_class =
        CLAY_CONTAINER_LIST_VIEW_GET_CLASS (view);

      g_signal_connect (container, "reorder",
                        G_CALLBACK (clay_container_list_view_reorder),
                        view);
      g_signal_connect (container, "items-changed",
                        G_CALLBACK (clay_container_list_view_before_items_changed),
                        view);

      gtk_list_box_bind_model (priv->view, G_LIST_MODEL (container),
                               view_class->create_row_func,
                               view, NULL);

      g_signal_connect (container, "items-changed",
                        G_CALLBACK (clay_container_list_view_after_items_changed),
                        view);
    }
}

static void
clay_container_list_view_set_context (ClayContainerView *view,
                                      ClayContext       *context)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);
  GList                        *children;
  GList                        *child;

  parent_view_iface->set_context (view, context);

  children = gtk_container_get_children (GTK_CONTAINER (priv->view));

  for (child = children; child; child = g_list_next (child))
    {
      clay_row_set_context (child->data, context);
    }

  g_list_free (children);
}

static void
clay_container_list_view_set_selection_mode (ClayContainerView *view,
                                             GtkSelectionMode   mode)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);

  parent_view_iface->set_selection_mode (view, mode);

  gtk_list_box_set_selection_mode (priv->view, mode);
}

static void
clay_container_list_view_set_view_size (ClayContainerView *view)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);
  GList                        *children;
  GList                        *child;
  gint                          view_size;
  gint                          border_width;

  view_size = clay_container_view_get_view_size (view, &border_width);

  children = gtk_container_get_children (GTK_CONTAINER (priv->view));

  for (child = children; child; child = g_list_next (child))
    {
      clay_row_set_view_size (child->data, view_size, border_width);
    }

  g_list_free (children);
}

static gboolean
clay_container_list_view_set_selected (ClayContainerView *view,
                                       GList             *items)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);
  GtkAdjustment                *adjustment;
  GtkAllocation                 allocation;
  GList                        *children;
  GList                        *child;
  gdouble                       scroll_offset;
  gint                          top_selected    = G_MAXINT;
  gint                          bottom_selected = 0;

  adjustment = gtk_list_box_get_adjustment (priv->view);

  scroll_offset = gtk_adjustment_get_value (adjustment);

  g_signal_handlers_block_by_func (priv->view,
                                   clay_container_list_view_selected_rows_changed,
                                   view);

  gtk_list_box_unselect_all (priv->view);

  adjustment = gtk_list_box_get_adjustment (priv->view);

  children = gtk_container_get_children (GTK_CONTAINER (priv->view));

  for (child = children; child; child = g_list_next (child))
    {
      ClayViewable *viewable = clay_row_get_viewable (child->data);

      if (g_list_find (items, viewable))
        {

          gtk_list_box_select_row (priv->view, child->data);

          gtk_widget_get_allocation (child->data, &allocation);

          top_selected    = MIN (top_selected, allocation.y);
          bottom_selected = MAX (bottom_selected, allocation.y + allocation.height);
        }
    }

  g_list_free (children);

  g_signal_handlers_unblock_by_func (priv->view,
                                     clay_container_list_view_selected_rows_changed,
                                     view);

  gtk_widget_get_allocation (gtk_widget_get_parent (GTK_WIDGET (priv->view)),
                             &allocation);

  if (top_selected < scroll_offset)
    {
      gtk_adjustment_set_value (adjustment, top_selected);
    }
  else if (bottom_selected > scroll_offset + allocation.height)
    {
      gtk_adjustment_set_value (adjustment, bottom_selected - allocation.height);
    }

  _clay_container_view_selection_changed (view);

  return TRUE;
}

static gint
clay_container_list_view_get_selected (ClayContainerView  *view,
                                       GList             **items)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);
  GList                        *rows;
  gint                          n_selected;

  rows = gtk_list_box_get_selected_rows (priv->view);

  n_selected = g_list_length (rows);

  if (items)
    {
      GList *row;

      for (row = rows; row; row = g_list_next (row))
        {
          *items = g_list_prepend (*items, clay_row_get_viewable (row->data));
        }

      *items = g_list_reverse (*items);
    }

  g_list_free (rows);

  return n_selected;
}

static void
clay_container_list_view_reorder (ClayContainer     *container,
                                  ClayObject        *object,
                                  gint               old_index,
                                  gint               new_index,
                                  ClayContainerView *view)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);

  priv->reordering = TRUE;
}

static void
clay_container_list_view_before_items_changed (ClayContainer     *container,
                                               guint              position,
                                               guint              removed,
                                               guint              added,
                                               ClayContainerView *view)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);

  if (priv->reordering)
    {
      clay_container_view_get_selected (view, &priv->reordering_selected);
      g_list_foreach (priv->reordering_selected, (GFunc) g_object_ref, NULL);
    }
}

static void
clay_container_list_view_after_items_changed (ClayContainer     *container,
                                              guint              position,
                                              guint              removed,
                                              guint              added,
                                              ClayContainerView *view)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);

  if (priv->reordering)
    {
      clay_container_view_set_selected (view, priv->reordering_selected);

      g_list_free_full (priv->reordering_selected, g_object_unref);
      priv->reordering_selected = NULL;

      priv->reordering = FALSE;
    }
}

static void
clay_container_list_view_selected_rows_changed (GtkListBox            *box,
                                                ClayContainerListView *list_view)
{
  _clay_container_view_selection_changed (CLAY_CONTAINER_VIEW (list_view));
}

static void
clay_container_list_view_row_activated (GtkListBox            *box,
                                        ClayRow               *row,
                                        ClayContainerListView *list_view)
{
  _clay_container_view_item_activated (CLAY_CONTAINER_VIEW (list_view),
                                       clay_row_get_viewable (row));
}

static void
clay_container_list_view_monitor_changed (ClayContainerListView *view)
{
  ClayContainerListViewPrivate *priv = GET_PRIVATE (view);
  GList                        *children;
  GList                        *child;

  children = gtk_container_get_children (GTK_CONTAINER (priv->view));

  for (child = children; child; child = g_list_next (child))
    {
      clay_row_monitor_changed (child->data);
    }

  g_list_free (children);
}
