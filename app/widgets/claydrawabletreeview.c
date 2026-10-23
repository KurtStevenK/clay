/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawabletreeview.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
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
#include "libclaywidgets/claywidgets-private.h"

#include "clayhelp-ids.h"
#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydrawable.h"
#include "core/claydrawable-edit.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawablefilter.h"
#include "core/clayfilloptions.h"
#include "core/claylayer.h"
#include "core/claylist.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/claypattern.h"
#include "core/claytreehandler.h"

#include "claycontainertreestore.h"
#include "claycontainerview.h"
#include "claycontainerview-cruft.h"
#include "claydnd.h"
#include "claydrawabletreeview.h"
#include "claydrawabletreeview-filters.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


struct _ClayDrawableTreeViewPrivate
{
  GtkWidget       *filters_header_image;

  gint             model_column_filters;
  GtkCellRenderer *filters_cell;

  ClayTreeHandler *filters_changed_handler;
};


static void   clay_drawable_tree_view_view_iface_init (ClayContainerViewInterface *iface);

static void     clay_drawable_tree_view_constructed   (GObject                    *object);
static void     clay_drawable_tree_view_dispose       (GObject                    *object);

static void     clay_drawable_tree_view_unmap         (GtkWidget                  *widget);
static void     clay_drawable_tree_view_style_updated (GtkWidget                  *widget);

static void     clay_drawable_tree_view_set_container (ClayContainerView          *view,
                                                       ClayContainer              *container);
static gboolean clay_drawable_tree_view_set_selected  (ClayContainerView          *view,
                                                       GList                      *items);

static gpointer clay_drawable_tree_view_insert_item   (ClayContainerView          *view,
                                                       ClayViewable               *viewable,
                                                       gpointer                    parent_insert_data,
                                                       gint                        index);

static gboolean clay_drawable_tree_view_drop_possible (ClayContainerTreeView      *view,
                                                       ClayDndType                 src_type,
                                                       GList                      *src_viewables,
                                                       ClayViewable               *dest_viewable,
                                                       GtkTreePath                *drop_path,
                                                       GtkTreeViewDropPosition     drop_pos,
                                                       GtkTreeViewDropPosition    *return_drop_pos,
                                                       GdkDragAction              *return_drag_action);
static void   clay_drawable_tree_view_drop_viewables  (ClayContainerTreeView      *view,
                                                       GList                      *src_viewables,
                                                       ClayViewable               *dest_viewable,
                                                       GtkTreeViewDropPosition     drop_pos);
static void   clay_drawable_tree_view_drop_color      (ClayContainerTreeView      *view,
                                                       GeglColor                  *color,
                                                       ClayViewable               *dest_viewable,
                                                       GtkTreeViewDropPosition     drop_pos);

static void   clay_drawable_tree_view_set_image       (ClayItemTreeView            *view,
                                                       ClayImage                   *image);

static void   clay_drawable_tree_view_floating_selection_changed
                                                      (ClayImage                   *image,
                                                       ClayDrawableTreeView        *view);

static void   clay_drawable_tree_view_filters_changed (ClayDrawable                *drawable,
                                                       ClayDrawableTreeView        *view);

static void   clay_drawable_tree_view_filters_cell_clicked
                                                      (GtkCellRendererToggle       *toggle,
                                                       gchar                       *path,
                                                       GdkModifierType              state,
                                                       ClayDrawableTreeView        *view);

static void   clay_drawable_tree_view_new_pattern_dropped
                                                      (GtkWidget                   *widget,
                                                       gint                         x,
                                                       gint                         y,
                                                       ClayViewable                *viewable,
                                                       gpointer                     data);
static void   clay_drawable_tree_view_new_color_dropped
                                                      (GtkWidget                   *widget,
                                                       gint                         x,
                                                       gint                         y,
                                                       GeglColor                   *color,
                                                       gpointer                     data);


G_DEFINE_TYPE_WITH_CODE (ClayDrawableTreeView, clay_drawable_tree_view,
                         CLAY_TYPE_ITEM_TREE_VIEW,
                         G_ADD_PRIVATE (ClayDrawableTreeView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_drawable_tree_view_view_iface_init))

#define parent_class clay_drawable_tree_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_drawable_tree_view_class_init (ClayDrawableTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GtkWidgetClass             *widget_class;
  ClayContainerTreeViewClass *tree_view_class;
  ClayItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  widget_class    = GTK_WIDGET_CLASS (klass);
  tree_view_class = CLAY_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = CLAY_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed       = clay_drawable_tree_view_constructed;
  object_class->dispose           = clay_drawable_tree_view_dispose;

  widget_class->unmap             = clay_drawable_tree_view_unmap;
  widget_class->style_updated     = clay_drawable_tree_view_style_updated;

  tree_view_class->drop_possible  = clay_drawable_tree_view_drop_possible;
  tree_view_class->drop_viewables = clay_drawable_tree_view_drop_viewables;
  tree_view_class->drop_color     = clay_drawable_tree_view_drop_color;

  item_view_class->set_image      = clay_drawable_tree_view_set_image;

  item_view_class->lock_content_icon_name    = CLAY_ICON_LOCK_CONTENT;
  item_view_class->lock_content_tooltip      = _("Lock pixels");
  item_view_class->lock_position_icon_name   = CLAY_ICON_LOCK_POSITION;
  item_view_class->lock_position_tooltip     = _("Lock position and size");
  item_view_class->lock_visibility_icon_name = CLAY_ICON_LOCK_VISIBILITY;
  item_view_class->lock_visibility_tooltip   = _("Lock visibility");
}

static void
clay_drawable_tree_view_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = clay_drawable_tree_view_set_container;
  iface->set_selected  = clay_drawable_tree_view_set_selected;

  iface->insert_item   = clay_drawable_tree_view_insert_item;
}

static void
clay_drawable_tree_view_init (ClayDrawableTreeView *view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);

  view->priv = clay_drawable_tree_view_get_instance_private (view);

  view->priv->model_column_filters =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);
}

static void
clay_drawable_tree_view_constructed (GObject *object)
{
  ClayContainerTreeView *tree_view        = CLAY_CONTAINER_TREE_VIEW (object);
  ClayItemTreeView      *item_view        = CLAY_ITEM_TREE_VIEW (object);
  ClayDrawableTreeView  *view             = CLAY_DRAWABLE_TREE_VIEW (object);
  GtkIconSize            button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  GtkTreeViewColumn     *column;
  gint                   pixel_icon_size  = 16;
  gint                   button_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &button_icon_size,
                        "button-spacing",   &button_spacing,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  column = gtk_tree_view_column_new ();
  view->priv->filters_header_image =
    gtk_image_new_from_icon_name (CLAY_ICON_EFFECT, button_icon_size);
  clay_widget_set_identifier (view->priv->filters_header_image,
                              "item-effect-header-icon");
  gtk_tree_view_column_set_widget (column, view->priv->filters_header_image);
  gtk_widget_set_visible (view->priv->filters_header_image, TRUE);
  gtk_tree_view_insert_column (tree_view->view, column, 2);

  view->priv->filters_cell = clay_cell_renderer_toggle_new (CLAY_ICON_EFFECT);
  g_object_set (view->priv->filters_cell,
                "xpad",      0,
                "ypad",      0,
                "icon-size", pixel_icon_size,
                NULL);
  gtk_tree_view_column_pack_start (column, view->priv->filters_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, view->priv->filters_cell,
                                       "active",
                                       view->priv->model_column_filters,
                                       "visible",
                                       view->priv->model_column_filters,
                                       NULL);

  clay_container_tree_view_add_toggle_cell (tree_view,
                                            view->priv->filters_cell);

  g_signal_connect (view->priv->filters_cell, "clicked",
                    G_CALLBACK (clay_drawable_tree_view_filters_cell_clicked),
                    item_view);

  clay_dnd_viewable_dest_add (clay_item_tree_view_get_new_button (item_view),
                              CLAY_TYPE_PATTERN,
                              clay_drawable_tree_view_new_pattern_dropped,
                              item_view);
  clay_dnd_color_dest_add (clay_item_tree_view_get_new_button (item_view),
                           clay_drawable_tree_view_new_color_dropped,
                           item_view);

  clay_dnd_color_dest_add    (GTK_WIDGET (tree_view->view),
                              NULL, tree_view);
  clay_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view), CLAY_TYPE_PATTERN,
                              NULL, tree_view);
}

static void
clay_drawable_tree_view_dispose (GObject *object)
{
  ClayDrawableTreeView *view = CLAY_DRAWABLE_TREE_VIEW (object);

  _clay_drawable_tree_view_filter_editor_destroy (view);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_drawable_tree_view_unmap (GtkWidget *widget)
{
  ClayDrawableTreeView *view = CLAY_DRAWABLE_TREE_VIEW (widget);

  _clay_drawable_tree_view_filter_editor_hide (view);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
clay_drawable_tree_view_style_updated (GtkWidget *widget)
{
  ClayDrawableTreeView *view = CLAY_DRAWABLE_TREE_VIEW (widget);
  const gchar          *old_icon_name;
  gint                  pixel_icon_size;
  GtkIconSize           old_size;
  GtkIconSize           button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;

  gtk_widget_style_get (widget,
                        "button-icon-size", &button_icon_size,
                        NULL);

  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->filters_header_image),
                           &old_icon_name, &old_size);

  if (button_icon_size != old_size)
    {
      gchar *icon_name = g_strdup (old_icon_name);

      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->filters_header_image),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  g_object_set (view->priv->filters_cell,
                "icon-name", CLAY_ICON_EFFECT,
                "icon-size", pixel_icon_size,
                NULL);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}


/*  ClayContainerView methods  */

static void
clay_drawable_tree_view_set_container (ClayContainerView *view,
                                       ClayContainer     *container)
{
  ClayDrawableTreeView *drawable_view = CLAY_DRAWABLE_TREE_VIEW (view);
  ClayContainer        *old_container;

  old_container = clay_container_view_get_container (view);

  if (old_container)
    g_clear_pointer (&drawable_view->priv->filters_changed_handler,
                     clay_tree_handler_disconnect);

  parent_view_iface->set_container (view, container);

  if (container)
    drawable_view->priv->filters_changed_handler =
      clay_tree_handler_connect (container, "filters-changed",
                                 G_CALLBACK (clay_drawable_tree_view_filters_changed),
                                 view);
}

static gboolean
clay_drawable_tree_view_set_selected (ClayContainerView *view,
                                      GList             *items)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayImage        *image     = clay_item_tree_view_get_image (item_view);
  gboolean          success   = TRUE;

  if (image)
    {
      ClayLayer *floating_sel = clay_image_get_floating_selection (image);

      success = (items        == NULL ||
                 floating_sel == NULL ||
                 (g_list_length (items) == 1 &&
                  items->data           == CLAY_VIEWABLE (floating_sel)));

      if (! success)
        {
          Clay        *clay    = image->clay;
          ClayContext *context = clay_get_user_context (clay);
          ClayDisplay *display = clay_context_get_display (context);

          clay_message_literal (clay, G_OBJECT (display), CLAY_MESSAGE_WARNING,
                                _("Cannot select items while a floating "
                                  "selection is active."));
        }
    }

  if (success)
    success = parent_view_iface->set_selected (view, items);

  return success;
}

static gpointer
clay_drawable_tree_view_insert_item (ClayContainerView *view,
                                     ClayViewable      *viewable,
                                     gpointer           parent_insert_data,
                                     gint               index)
{
  ClayContainerTreeView *tree_view     = CLAY_CONTAINER_TREE_VIEW (view);
  ClayDrawableTreeView  *drawable_view = CLAY_DRAWABLE_TREE_VIEW (view);
  ClayContainer         *filters;
  GtkTreeIter           *iter;
  gint                   n_filters;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  filters   = clay_drawable_get_filters (CLAY_DRAWABLE (viewable));
  n_filters = clay_container_get_n_children (filters);

  /* Also account for mask filters */
  if (CLAY_IS_LAYER (viewable) && clay_layer_get_mask (CLAY_LAYER (viewable)))
    {
      ClayDrawable *mask =
        CLAY_DRAWABLE (clay_layer_get_mask (CLAY_LAYER (viewable)));

      filters = clay_drawable_get_filters (CLAY_DRAWABLE (mask));

      n_filters += clay_container_get_n_children (filters);
    }

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      drawable_view->priv->model_column_filters,
                      n_filters > 0,
                      -1);

  return iter;
}


/*  ClayContainerTreeView methods  */

static gboolean
clay_drawable_tree_view_drop_possible (ClayContainerTreeView   *tree_view,
                                       ClayDndType              src_type,
                                       GList                   *src_viewables,
                                       ClayViewable            *dest_viewable,
                                       GtkTreePath             *drop_path,
                                       GtkTreeViewDropPosition  drop_pos,
                                       GtkTreeViewDropPosition *return_drop_pos,
                                       GdkDragAction           *return_drag_action)
{
  if (CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                    src_type,
                                                                    src_viewables,
                                                                    dest_viewable,
                                                                    drop_path,
                                                                    drop_pos,
                                                                    return_drop_pos,
                                                                    return_drag_action))
    {
      if (src_type == CLAY_DND_TYPE_COLOR ||
          src_type == CLAY_DND_TYPE_PATTERN)
        {
          if (! dest_viewable ||
              clay_item_is_content_locked (CLAY_ITEM (dest_viewable), NULL) ||
              clay_viewable_get_children (CLAY_VIEWABLE (dest_viewable)))
            return FALSE;

          if (return_drop_pos)
            {
              *return_drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
clay_drawable_tree_view_drop_viewables (ClayContainerTreeView   *view,
                                        GList                   *src_viewables,
                                        ClayViewable            *dest_viewable,
                                        GtkTreeViewDropPosition  drop_pos)
{
  GList *iter;

  for (iter = src_viewables; iter; iter = iter->next)
    {
      ClayViewable *src_viewable = iter->data;

      if (dest_viewable && CLAY_IS_PATTERN (src_viewable))
        {
          ClayImage       *image   = clay_item_get_image (CLAY_ITEM (dest_viewable));
          ClayFillOptions *options = clay_fill_options_new (image->clay, NULL, FALSE);

          clay_fill_options_set_style (options, CLAY_FILL_STYLE_PATTERN);
          clay_context_set_pattern (CLAY_CONTEXT (options),
                                    CLAY_PATTERN (src_viewable));

          clay_drawable_edit_fill (CLAY_DRAWABLE (dest_viewable),
                                   options,
                                   C_("undo-type", "Drop pattern to layer"));

          g_object_unref (options);

          clay_image_flush (image);
          return;
        }
    }

  CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);
}

static void
clay_drawable_tree_view_drop_color (ClayContainerTreeView   *view,
                                    GeglColor               *color,
                                    ClayViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  if (dest_viewable)
    {
      ClayImage       *image   = clay_item_get_image (CLAY_ITEM (dest_viewable));
      ClayFillOptions *options = clay_fill_options_new (image->clay, NULL, FALSE);

      clay_fill_options_set_style (options, CLAY_FILL_STYLE_FG_COLOR);
      clay_context_set_foreground (CLAY_CONTEXT (options), color);

      clay_drawable_edit_fill (CLAY_DRAWABLE (dest_viewable),
                               options,
                               C_("undo-type", "Drop color to layer"));

      g_object_unref (options);

      clay_image_flush (image);
    }
}


/*  ClayItemTreeView methods  */

static void
clay_drawable_tree_view_set_image (ClayItemTreeView *view,
                                   ClayImage        *image)
{
  ClayDrawableTreeView *drawable_view = CLAY_DRAWABLE_TREE_VIEW (view);

  if (clay_item_tree_view_get_image (view))
    {
      _clay_drawable_tree_view_filter_editor_hide (drawable_view);

      g_signal_handlers_disconnect_by_func (clay_item_tree_view_get_image (view),
                                            clay_drawable_tree_view_floating_selection_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (clay_item_tree_view_get_image (view)->clay->config,
                                            G_CALLBACK (clay_drawable_tree_view_style_updated),
                                            view);

    }

  CLAY_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (clay_item_tree_view_get_image (view))
    {
      g_signal_connect (clay_item_tree_view_get_image (view),
                        "floating-selection-changed",
                        G_CALLBACK (clay_drawable_tree_view_floating_selection_changed),
                        view);

      g_signal_connect_object (image->clay->config,
                               "notify::theme",
                               G_CALLBACK (clay_drawable_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (image->clay->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (clay_drawable_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (image->clay->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (clay_drawable_tree_view_style_updated),
                               view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    }
}


/*  callbacks  */

static void
clay_drawable_tree_view_floating_selection_changed (ClayImage            *image,
                                                    ClayDrawableTreeView *view)
{
  GType  item_type;
  GList *items;

  item_type = CLAY_ITEM_TREE_VIEW_GET_CLASS (view)->item_type;

  items = clay_image_get_selected_items (image, item_type);
  items = g_list_copy (items);

  /*  update button states  */
  clay_container_view_set_selected (CLAY_CONTAINER_VIEW (view), items);

  g_list_free (items);
}


/*  filters callbacks  */

static void
clay_drawable_tree_view_filters_changed (ClayDrawable         *drawable,
                                         ClayDrawableTreeView *view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = _clay_container_view_lookup (CLAY_CONTAINER_VIEW (view),
                                      (ClayViewable *) drawable);

  if (iter)
    {
      gint n_editable;

      clay_drawable_n_editable_filters (drawable, &n_editable, NULL, NULL);

        /* Also account for mask filters */
      if (CLAY_IS_LAYER (drawable) &&
          clay_layer_get_mask (CLAY_LAYER (drawable)))
        {
          ClayDrawable *mask;
          gint          n_mask_editable = 0;

          mask = CLAY_DRAWABLE (clay_layer_get_mask (CLAY_LAYER (drawable)));

          clay_drawable_n_editable_filters (mask, &n_mask_editable, NULL,
                                            NULL);
          n_editable += n_mask_editable;
        }

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_filters,
                          n_editable > 0,
                          -1);
    }
}

static void
clay_drawable_tree_view_filters_cell_clicked (GtkCellRendererToggle *toggle,
                                              gchar                 *path_str,
                                              GdkModifierType        state,
                                              ClayDrawableTreeView  *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (CLAY_CONTAINER_TREE_VIEW (view)->model,
                               &iter, path))
    {
      ClayViewRenderer       *renderer;
      ClayContainerTreeStore *store;
      ClayDrawable           *drawable;
      GdkRectangle            rect;

      /* Update the filter state. */
      store = CLAY_CONTAINER_TREE_STORE (CLAY_CONTAINER_TREE_VIEW (view)->model);
      renderer = clay_container_tree_store_get_renderer (store, &iter);
      drawable = CLAY_DRAWABLE (renderer->viewable);
      g_object_unref (renderer);

      gtk_tree_view_get_cell_area (CLAY_CONTAINER_TREE_VIEW (view)->view, path,
                                   gtk_tree_view_get_column (CLAY_CONTAINER_TREE_VIEW (view)->view, 2),
                                   &rect);
      gtk_tree_view_convert_bin_window_to_widget_coords (CLAY_CONTAINER_TREE_VIEW (view)->view,
                                                         rect.x, rect.y, &rect.x, &rect.y);

      _clay_drawable_tree_view_filter_editor_show (view, drawable, &rect);
    }
}


/*  dnd callbacks  */

static void
clay_drawable_tree_view_new_dropped (ClayItemTreeView *view,
                                     ClayFillOptions  *options,
                                     const gchar      *undo_desc)
{
  ClayImage *image = clay_item_tree_view_get_image (view);
  ClayItem  *item;

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                               C_("singular", "New Layer"));

  item = CLAY_ITEM_TREE_VIEW_GET_CLASS (view)->new_item (image);

  if (item)
    clay_drawable_edit_fill (CLAY_DRAWABLE (item), options, undo_desc);

  clay_image_undo_group_end (image);

  clay_image_flush (image);
}

static void
clay_drawable_tree_view_new_pattern_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             ClayViewable *viewable,
                                             gpointer      data)
{
  ClayItemTreeView *view    = CLAY_ITEM_TREE_VIEW (data);
  ClayImage        *image   = clay_item_tree_view_get_image (view);
  ClayFillOptions  *options = clay_fill_options_new (image->clay, NULL, FALSE);

  clay_fill_options_set_style (options, CLAY_FILL_STYLE_PATTERN);
  clay_context_set_pattern (CLAY_CONTEXT (options), CLAY_PATTERN (viewable));

  clay_drawable_tree_view_new_dropped (view, options,
                                       C_("undo-type", "Drop pattern to layer"));

  g_object_unref (options);
}

static void
clay_drawable_tree_view_new_color_dropped (GtkWidget *widget,
                                           gint       x,
                                           gint       y,
                                           GeglColor *color,
                                           gpointer   data)
{
  ClayItemTreeView *view    = CLAY_ITEM_TREE_VIEW (data);
  ClayImage        *image   = clay_item_tree_view_get_image (view);
  ClayFillOptions  *options = clay_fill_options_new (image->clay, NULL, FALSE);

  clay_fill_options_set_style (options, CLAY_FILL_STYLE_FG_COLOR);
  clay_context_set_foreground (CLAY_CONTEXT (options), color);

  clay_drawable_tree_view_new_dropped (view, options,
                                       C_("undo-type", "Drop color to layer"));

  g_object_unref (options);
}
