/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylayertreeview.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#if 0
/* For mask features in clay_layer_tree_view_layer_clicked() */
#include "config/claydialogconfig.h"
#endif

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/clayimage.h"
#include "core/clayitemundo.h"
#include "core/claylayer.h"
#include "core/claylayer-floating-selection.h"
#include "core/claylayer-new.h"
#include "core/claylayermask.h"
#include "core/claypattern.h"
#include "core/claytreehandler.h"

#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "file/file-open.h"

#include "clayactiongroup.h"
#include "claycellrendererviewable.h"
#include "claycontainertreestore.h"
#include "claycontainerview.h"
#include "claycontainerview-cruft.h"
#include "claydnd.h"
#include "clayhelp-ids.h"
#include "claylayermodebox.h"
#include "claylayertreeview.h"
#include "claytoggleaction.h"
#include "clayuimanager.h"
#include "clayviewrenderer.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


struct _ClayLayerTreeViewPrivate
{
  GtkWidget       *layer_mode_box;
  GtkAdjustment   *opacity_adjustment;
  GtkWidget       *anchor_button;
  GtkWidget       *delete_mask_button;

  gint             model_column_mask;
  gint             model_column_mask_visible;

  GtkCellRenderer *mask_cell;

  PangoAttrList   *italic_attrs;
  PangoAttrList   *bold_attrs;

  ClayTreeHandler *mode_changed_handler;
  ClayTreeHandler *opacity_changed_handler;
  ClayTreeHandler *mask_changed_handler;
  ClayTreeHandler *alpha_changed_handler;
};


static void       clay_layer_tree_view_view_iface_init            (ClayContainerViewInterface *iface);

static void       clay_layer_tree_view_constructed                (GObject                    *object);
static void       clay_layer_tree_view_finalize                   (GObject                    *object);

static void       clay_layer_tree_view_set_container              (ClayContainerView          *view,
                                                                   ClayContainer              *container);
static void       clay_layer_tree_view_set_context                (ClayContainerView          *view,
                                                                   ClayContext                *context);
static void       clay_layer_tree_view_set_view_size              (ClayContainerView          *view);
static gboolean   clay_layer_tree_view_set_selected               (ClayContainerView          *view,
                                                                   GList                      *items);

static gpointer   clay_layer_tree_view_insert_item                (ClayContainerView          *view,
                                                                   ClayViewable               *viewable,
                                                                   gpointer                    parent_insert_data,
                                                                   gint                        index);
static gboolean   clay_layer_tree_view_drop_possible              (ClayContainerTreeView      *view,
                                                                   ClayDndType                 src_type,
                                                                   GList                      *src_viewables,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreePath                *drop_path,
                                                                   GtkTreeViewDropPosition     drop_pos,
                                                                   GtkTreeViewDropPosition    *return_drop_pos,
                                                                   GdkDragAction              *return_drag_action);
static void       clay_layer_tree_view_drop_viewables             (ClayContainerTreeView      *view,
                                                                   GList                      *src_viewables,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       clay_layer_tree_view_drop_color                 (ClayContainerTreeView      *view,
                                                                   GeglColor                  *color,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       clay_layer_tree_view_drop_uri_list              (ClayContainerTreeView      *view,
                                                                   GList                      *uri_list,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       clay_layer_tree_view_drop_component             (ClayContainerTreeView      *tree_view,
                                                                   ClayImage                  *image,
                                                                   ClayChannelType             component,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       clay_layer_tree_view_drop_pixbuf                (ClayContainerTreeView      *tree_view,
                                                                   GdkPixbuf                  *pixbuf,
                                                                   ClayViewable               *dest_viewable,
                                                                   GtkTreeViewDropPosition     drop_pos);
static void       clay_layer_tree_view_set_image                  (ClayItemTreeView           *view,
                                                                   ClayImage                  *image);
static ClayItem * clay_layer_tree_view_item_new                   (ClayImage                  *image);
static void       clay_layer_tree_view_floating_selection_changed (ClayImage                  *image,
                                                                   ClayLayerTreeView          *view);

static void       clay_layer_tree_view_layer_mode_box_callback    (GtkWidget                  *widget,
                                                                   const GParamSpec           *pspec,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_opacity_scale_changed      (GtkAdjustment              *adj,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_layer_signal_handler       (ClayLayer                  *layer,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_update_options             (ClayLayerTreeView          *view,
                                                                   GList                      *layers);
static void       clay_layer_tree_view_update_highlight           (ClayLayerTreeView          *view);
static void       clay_layer_tree_view_mask_update                (ClayLayerTreeView          *view,
                                                                   GtkTreeIter                *iter,
                                                                   ClayLayer                  *layer);
static void       clay_layer_tree_view_mask_changed               (ClayLayer                  *layer,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_renderer_update            (ClayViewRenderer           *renderer,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_update_borders             (ClayLayerTreeView          *view,
                                                                   GtkTreeIter                *iter);
static void       clay_layer_tree_view_mask_callback              (ClayLayer                  *mask,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_layer_clicked              (ClayCellRendererViewable   *cell,
                                                                   const gchar                *path,
                                                                   GdkModifierType             state,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_mask_clicked               (ClayCellRendererViewable   *cell,
                                                                   const gchar                *path,
                                                                   GdkModifierType             state,
                                                                   ClayLayerTreeView          *view);
static void       clay_layer_tree_view_alpha_update               (ClayLayerTreeView          *view,
                                                                   GtkTreeIter                *iter,
                                                                   ClayLayer                  *layer);
static void       clay_layer_tree_view_alpha_changed              (ClayLayer                  *layer,
                                                                   ClayLayerTreeView          *view);


G_DEFINE_TYPE_WITH_CODE (ClayLayerTreeView, clay_layer_tree_view,
                         CLAY_TYPE_DRAWABLE_TREE_VIEW,
                         G_ADD_PRIVATE (ClayLayerTreeView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_layer_tree_view_view_iface_init))

#define parent_class clay_layer_tree_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_layer_tree_view_class_init (ClayLayerTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  ClayContainerTreeViewClass *tree_view_class;
  ClayItemTreeViewClass      *item_view_class;

  tree_view_class = CLAY_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = CLAY_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed           = clay_layer_tree_view_constructed;
  object_class->finalize              = clay_layer_tree_view_finalize;

  tree_view_class->drop_possible      = clay_layer_tree_view_drop_possible;
  tree_view_class->drop_viewables     = clay_layer_tree_view_drop_viewables;
  tree_view_class->drop_color         = clay_layer_tree_view_drop_color;
  tree_view_class->drop_uri_list      = clay_layer_tree_view_drop_uri_list;
  tree_view_class->drop_component     = clay_layer_tree_view_drop_component;
  tree_view_class->drop_pixbuf        = clay_layer_tree_view_drop_pixbuf;

  item_view_class->item_type          = CLAY_TYPE_LAYER;
  item_view_class->signal_name        = "selected-layers-changed";

  item_view_class->set_image          = clay_layer_tree_view_set_image;

  item_view_class->new_item           = clay_layer_tree_view_item_new;

  item_view_class->action_group        = "layers";
  item_view_class->activate_action     = "layers-edit";
  item_view_class->new_action          = "layers-new";
  item_view_class->new_default_action  = "layers-new-last-values";
  item_view_class->raise_action        = "layers-raise";
  item_view_class->raise_top_action    = "layers-raise-to-top";
  item_view_class->lower_action        = "layers-lower";
  item_view_class->lower_bottom_action = "layers-lower-to-bottom";
  item_view_class->duplicate_action    = "layers-duplicate";
  item_view_class->delete_action       = "layers-delete";

  item_view_class->move_cursor_up_action        = "layers-select-previous";
  item_view_class->move_cursor_down_action      = "layers-select-next";
  item_view_class->move_cursor_up_flat_action   = "layers-select-flattened-previous";
  item_view_class->move_cursor_down_flat_action = "layers-select-flattened-next";
  item_view_class->move_cursor_start_action     = "layers-select-top";
  item_view_class->move_cursor_end_action       = "layers-select-bottom";

  item_view_class->lock_content_help_id    = CLAY_HELP_LAYER_LOCK_PIXELS;
  item_view_class->lock_position_help_id   = CLAY_HELP_LAYER_LOCK_POSITION;
  item_view_class->lock_visibility_help_id = CLAY_HELP_LAYER_LOCK_VISIBILITY;
}

static void
clay_layer_tree_view_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = clay_layer_tree_view_set_container;
  iface->set_context   = clay_layer_tree_view_set_context;
  iface->set_view_size = clay_layer_tree_view_set_view_size;
  iface->set_selected  = clay_layer_tree_view_set_selected;

  iface->insert_item   = clay_layer_tree_view_insert_item;

  iface->model_is_tree = TRUE;
}

static void
clay_layer_tree_view_init (ClayLayerTreeView *view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  GtkWidget             *scale;
  PangoAttribute        *attr;

  view->priv = clay_layer_tree_view_get_instance_private (view);

  view->priv->model_column_mask =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           CLAY_TYPE_VIEW_RENDERER);

  view->priv->model_column_mask_visible =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  /*  Paint mode menu  */

  view->priv->layer_mode_box = clay_layer_mode_box_new (CLAY_LAYER_MODE_CONTEXT_LAYER);
  clay_layer_mode_box_set_label (CLAY_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                 _("Mode"));
  clay_item_tree_view_add_options (CLAY_ITEM_TREE_VIEW (view), NULL,
                                   view->priv->layer_mode_box);

  g_signal_connect (view->priv->layer_mode_box, "notify::layer-mode",
                    G_CALLBACK (clay_layer_tree_view_layer_mode_box_callback),
                    view);
  clay_help_connect (view->priv->layer_mode_box, NULL, clay_standard_help_func,
                     CLAY_HELP_LAYER_DIALOG_PAINT_MODE_MENU, NULL, NULL);

  /*  Opacity scale  */

  view->priv->opacity_adjustment = gtk_adjustment_new (100.0, 0.0, 100.0,
                                                       1.0, 10.0, 0.0);
  scale = clay_spin_scale_new (view->priv->opacity_adjustment, _("Opacity"), 1);
  clay_spin_scale_set_constrain_drag (CLAY_SPIN_SCALE (scale), TRUE);
  clay_help_connect (scale, NULL, clay_standard_help_func,
                     CLAY_HELP_LAYER_DIALOG_OPACITY_SCALE, NULL, NULL);
  clay_item_tree_view_add_options (CLAY_ITEM_TREE_VIEW (view),
                                   NULL, scale);

  g_signal_connect (view->priv->opacity_adjustment, "value-changed",
                    G_CALLBACK (clay_layer_tree_view_opacity_scale_changed),
                    view);

  view->priv->italic_attrs = pango_attr_list_new ();
  attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (view->priv->italic_attrs, attr);

  view->priv->bold_attrs = pango_attr_list_new ();
  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (view->priv->bold_attrs, attr);
}

static void
clay_layer_tree_view_constructed (GObject *object)
{
  ClayContainerTreeView *tree_view  = CLAY_CONTAINER_TREE_VIEW (object);
  ClayLayerTreeView     *layer_view = CLAY_LAYER_TREE_VIEW (object);
  GtkWidget             *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  layer_view->priv->mask_cell = clay_cell_renderer_viewable_new ();
  gtk_tree_view_column_pack_start (tree_view->main_column,
                                   layer_view->priv->mask_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (tree_view->main_column,
                                       layer_view->priv->mask_cell,
                                       "renderer",
                                       layer_view->priv->model_column_mask,
                                       "visible",
                                       layer_view->priv->model_column_mask_visible,
                                       NULL);

  clay_container_tree_view_add_renderer_cell (tree_view,
                                              layer_view->priv->mask_cell,
                                              layer_view->priv->model_column_mask);

  g_signal_connect (tree_view->renderer_cell, "clicked",
                    G_CALLBACK (clay_layer_tree_view_layer_clicked),
                    layer_view);
  g_signal_connect (layer_view->priv->mask_cell, "clicked",
                    G_CALLBACK (clay_layer_tree_view_mask_clicked),
                    layer_view);

  clay_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  clay_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), CLAY_TYPE_CHANNEL,
                               NULL, tree_view);
  clay_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), CLAY_TYPE_LAYER_MASK,
                               NULL, tree_view);
  clay_dnd_uri_list_dest_add  (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);
  clay_dnd_pixbuf_dest_add    (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  button = clay_editor_add_action_button (CLAY_EDITOR (layer_view), "layers",
                                          "layers-new-group", NULL);
  gtk_box_reorder_child (clay_editor_get_button_box (CLAY_EDITOR (layer_view)),
                         button, 1);

  button = clay_editor_add_action_button (CLAY_EDITOR (layer_view), "layers",
                                          "layers-anchor", NULL);
  layer_view->priv->anchor_button = button;
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  CLAY_TYPE_LAYER);
  gtk_box_reorder_child (clay_editor_get_button_box (CLAY_EDITOR (layer_view)),
                         button, 5);

  button = clay_editor_add_action_button (CLAY_EDITOR (layer_view), "layers",
                                          "layers-merge-down-button",
                                          "layers-merge-group",
                                          GDK_SHIFT_MASK,
                                          "layers-merge-layers",
                                          GDK_CONTROL_MASK,
                                          "layers-merge-layers-last-values",
                                          GDK_CONTROL_MASK |
                                          GDK_SHIFT_MASK,
                                          NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  CLAY_TYPE_LAYER);
  gtk_box_reorder_child (clay_editor_get_button_box (CLAY_EDITOR (layer_view)),
                         button, 6);

  button = clay_editor_add_action_button (CLAY_EDITOR (layer_view), "layers",
                                          "layers-mask-add-button",
                                          "layers-mask-add-last-values",
                                          clay_get_extend_selection_mask (),
                                          "layers-mask-delete",
                                          clay_get_modify_selection_mask (),
                                          "layers-mask-apply",
                                          clay_get_extend_selection_mask () |
                                          clay_get_modify_selection_mask (),
                                          NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (layer_view),
                                  GTK_BUTTON (button),
                                  CLAY_TYPE_LAYER);
  gtk_box_reorder_child (clay_editor_get_button_box (CLAY_EDITOR (layer_view)),
                         button, 7);

  button = clay_editor_add_action_button (CLAY_EDITOR (layer_view), "layers",
                                          "layers-mask-delete", NULL);
  layer_view->priv->delete_mask_button = button;
  gtk_widget_set_visible (layer_view->priv->delete_mask_button, FALSE);

  /*  Lock alpha toggle  */

  clay_item_tree_view_add_lock (CLAY_ITEM_TREE_VIEW (tree_view),
                                CLAY_ICON_LOCK_ALPHA,
                                (ClayIsLockedFunc) clay_layer_get_lock_alpha,
                                (ClayCanLockFunc)  clay_layer_can_lock_alpha,
                                (ClaySetLockFunc)  clay_layer_set_lock_alpha,
                                (ClayUndoLockPush) clay_image_undo_push_layer_lock_alpha,
                                "lock-alpha-changed",
                                CLAY_UNDO_LAYER_LOCK_ALPHA,
                                CLAY_UNDO_GROUP_LAYER_LOCK_ALPHA,
                                _("Lock alpha channel"),
                                _("Unlock alpha channel"),
                                _("Set Item Exclusive Alpha Channel lock"),
                                _("Lock alpha channel"),
                                CLAY_HELP_LAYER_LOCK_ALPHA);
}

static void
clay_layer_tree_view_finalize (GObject *object)
{
  ClayLayerTreeView *layer_view = CLAY_LAYER_TREE_VIEW (object);

  if (layer_view->priv->italic_attrs)
    {
      pango_attr_list_unref (layer_view->priv->italic_attrs);
      layer_view->priv->italic_attrs = NULL;
    }

  if (layer_view->priv->bold_attrs)
    {
      pango_attr_list_unref (layer_view->priv->bold_attrs);
      layer_view->priv->bold_attrs = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  ClayContainerView methods  */

static void
clay_layer_tree_view_set_container (ClayContainerView *view,
                                    ClayContainer     *container)
{
  ClayLayerTreeView *layer_view = CLAY_LAYER_TREE_VIEW (view);
  ClayContainer     *old_container;

  old_container = clay_container_view_get_container (view);

  if (old_container)
    {
      clay_tree_handler_disconnect (layer_view->priv->mode_changed_handler);
      layer_view->priv->mode_changed_handler = NULL;

      clay_tree_handler_disconnect (layer_view->priv->opacity_changed_handler);
      layer_view->priv->opacity_changed_handler = NULL;

      clay_tree_handler_disconnect (layer_view->priv->mask_changed_handler);
      layer_view->priv->mask_changed_handler = NULL;

      clay_tree_handler_disconnect (layer_view->priv->alpha_changed_handler);
      layer_view->priv->alpha_changed_handler = NULL;
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      layer_view->priv->mode_changed_handler =
        clay_tree_handler_connect (container, "mode-changed",
                                   G_CALLBACK (clay_layer_tree_view_layer_signal_handler),
                                   view);

      layer_view->priv->opacity_changed_handler =
        clay_tree_handler_connect (container, "opacity-changed",
                                   G_CALLBACK (clay_layer_tree_view_layer_signal_handler),
                                   view);

      layer_view->priv->mask_changed_handler =
        clay_tree_handler_connect (container, "mask-changed",
                                   G_CALLBACK (clay_layer_tree_view_mask_changed),
                                   view);

      layer_view->priv->alpha_changed_handler =
        clay_tree_handler_connect (container, "alpha-changed",
                                   G_CALLBACK (clay_layer_tree_view_alpha_changed),
                                   view);
    }
}

typedef struct
{
  gint         mask_column;
  ClayContext *context;
} SetContextForeachData;

static gboolean
clay_layer_tree_view_set_context_foreach (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
  SetContextForeachData *context_data = data;
  ClayViewRenderer      *renderer;

  gtk_tree_model_get (model, iter,
                      context_data->mask_column, &renderer,
                      -1);

  if (renderer)
    {
      clay_view_renderer_set_context (renderer, context_data->context);

      g_object_unref (renderer);
    }

  return FALSE;
}

static void
clay_layer_tree_view_set_context (ClayContainerView *view,
                                  ClayContext       *context)
{
  ClayContainerTreeView *tree_view  = CLAY_CONTAINER_TREE_VIEW (view);
  ClayLayerTreeView     *layer_view = CLAY_LAYER_TREE_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (tree_view->model)
    {
      SetContextForeachData context_data = { layer_view->priv->model_column_mask,
                                             context };

      gtk_tree_model_foreach (tree_view->model,
                              clay_layer_tree_view_set_context_foreach,
                              &context_data);
    }
}

typedef struct
{
  gint mask_column;
  gint view_size;
  gint border_width;
} SetSizeForeachData;

static gboolean
clay_layer_tree_view_set_view_size_foreach (GtkTreeModel *model,
                                            GtkTreePath  *path,
                                            GtkTreeIter  *iter,
                                            gpointer      data)
{
  SetSizeForeachData *size_data = data;
  ClayViewRenderer   *renderer;

  gtk_tree_model_get (model, iter,
                      size_data->mask_column, &renderer,
                      -1);

  if (renderer)
    {
      clay_view_renderer_set_size (renderer,
                                   size_data->view_size,
                                   size_data->border_width);

      g_object_unref (renderer);
    }

  return FALSE;
}

static void
clay_layer_tree_view_set_view_size (ClayContainerView *view)
{
  ClayContainerTreeView *tree_view  = CLAY_CONTAINER_TREE_VIEW (view);

  if (tree_view->model)
    {
      ClayLayerTreeView *layer_view = CLAY_LAYER_TREE_VIEW (view);
      SetSizeForeachData size_data;

      size_data.mask_column = layer_view->priv->model_column_mask;

      size_data.view_size =
        clay_container_view_get_view_size (view, &size_data.border_width);

      gtk_tree_model_foreach (tree_view->model,
                              clay_layer_tree_view_set_view_size_foreach,
                              &size_data);
    }

  parent_view_iface->set_view_size (view);
}

static gboolean
clay_layer_tree_view_set_selected (ClayContainerView *view,
                                   GList             *items)
{
  ClayLayerTreeView *layer_view = CLAY_LAYER_TREE_VIEW (view);
  gboolean           success;

  success = parent_view_iface->set_selected (view, items);

  if (items && success)
    {
      GList *list;

      for (list = items; list; list = g_list_next (list))
        {
          GtkTreeIter *iter;

          iter = _clay_container_view_lookup (view, list->data);

          if (iter)
            clay_layer_tree_view_update_borders (layer_view, iter);
        }

      clay_layer_tree_view_update_options (layer_view, items);
    }

  if (! success)
    {
      ClayEditor *editor = CLAY_EDITOR (view);

      /* currently, select_items() only ever fails when there is a
       * floating selection, which can be committed/canceled through
       * the editor buttons.
       */
      clay_widget_blink (GTK_WIDGET (clay_editor_get_button_box (editor)));
    }

  return success;
}

static gpointer
clay_layer_tree_view_insert_item (ClayContainerView *view,
                                  ClayViewable      *viewable,
                                  gpointer           parent_insert_data,
                                  gint               index)
{
  ClayLayerTreeView *layer_view = CLAY_LAYER_TREE_VIEW (view);
  ClayLayer         *layer;
  GtkTreeIter       *iter;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  layer = CLAY_LAYER (viewable);

  if (! clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
    clay_layer_tree_view_alpha_update (layer_view, iter, layer);

  clay_layer_tree_view_mask_update (layer_view, iter, layer);

  if (CLAY_IS_LAYER (viewable) && clay_layer_is_floating_sel (CLAY_LAYER (viewable)))
    {
      ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
      ClayLayer             *floating  = CLAY_LAYER (viewable);
      ClayDrawable          *drawable  = clay_layer_get_floating_sel_drawable (floating);

      if (CLAY_IS_LAYER_MASK (drawable))
        {
          /* Display floating mask in the mask column. */
          ClayViewRenderer *renderer  = NULL;

          gtk_tree_model_get (GTK_TREE_MODEL (tree_view->model), iter,
                              CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                              -1);
          if (renderer)
            {
              gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                                  layer_view->priv->model_column_mask,         renderer,
                                  layer_view->priv->model_column_mask_visible, TRUE,
                                  CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER,   NULL,
                                  -1);
              g_object_unref (renderer);
            }
        }
    }

  return iter;
}


/*  ClayContainerTreeView methods  */

static gboolean
clay_layer_tree_view_drop_possible (ClayContainerTreeView   *tree_view,
                                    ClayDndType              src_type,
                                    GList                   *src_viewables,
                                    ClayViewable            *dest_viewable,
                                    GtkTreePath             *drop_path,
                                    GtkTreeViewDropPosition  drop_pos,
                                    GtkTreeViewDropPosition *return_drop_pos,
                                    GdkDragAction           *return_drag_action)
{
  /* If we are dropping a new layer, check if the destination image
   * has a floating selection.
   */
  if  (src_type == CLAY_DND_TYPE_URI_LIST     ||
       src_type == CLAY_DND_TYPE_TEXT_PLAIN   ||
       src_type == CLAY_DND_TYPE_NETSCAPE_URL ||
       src_type == CLAY_DND_TYPE_COMPONENT    ||
       src_type == CLAY_DND_TYPE_PIXBUF       ||
       g_list_length (src_viewables) > 0)
    {
      ClayImage *dest_image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (tree_view));

      if (clay_image_get_floating_selection (dest_image))
        return FALSE;
    }

  return CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                       src_type,
                                                                       src_viewables,
                                                                       dest_viewable,
                                                                       drop_path,
                                                                       drop_pos,
                                                                       return_drop_pos,
                                                                       return_drag_action);
}

static void
clay_layer_tree_view_drop_viewables (ClayContainerTreeView   *view,
                                     GList                   *src_viewables,
                                     ClayViewable            *dest_viewable,
                                     GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (view);
  GList            *iter;

  for (iter = src_viewables; iter; iter = iter->next)
    {
      ClayViewable *src_viewable = iter->data;

      if (dest_viewable                                         &&
          clay_item_is_vector_layer (CLAY_ITEM (dest_viewable)) &&
          CLAY_IS_PATTERN (src_viewable))
        {
          clay_vector_layer_set (CLAY_VECTOR_LAYER (dest_viewable), NULL,
                                 "enable-fill",  TRUE,
                                 "fill-style",   CLAY_CUSTOM_STYLE_PATTERN,
                                 "fill-pattern", CLAY_PATTERN (src_viewable),
                                 NULL);

          clay_vector_layer_refresh (CLAY_VECTOR_LAYER (dest_viewable));
          clay_image_flush (clay_item_tree_view_get_image (item_view));
          return;
        }
    }

  CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);
}

static void
clay_layer_tree_view_drop_color (ClayContainerTreeView   *view,
                                 GeglColor               *color,
                                 ClayViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayLayerMask    *mask      = clay_layer_get_mask (CLAY_LAYER (dest_viewable));

  if (mask && clay_layer_get_edit_mask (CLAY_LAYER (dest_viewable)))
    {
      dest_viewable = CLAY_VIEWABLE (mask);
    }
  else if (clay_item_is_text_layer (CLAY_ITEM (dest_viewable)))
    {
      clay_text_layer_set (CLAY_TEXT_LAYER (dest_viewable), NULL,
                           "color", color,
                           NULL);
      clay_image_flush (clay_item_tree_view_get_image (item_view));
      return;
    }
  else if (clay_item_is_vector_layer (CLAY_ITEM (dest_viewable)))
    {
      clay_vector_layer_set (CLAY_VECTOR_LAYER (dest_viewable), NULL,
                             "enable-fill", TRUE,
                             "fill-style",  CLAY_CUSTOM_STYLE_SOLID_COLOR,
                             "fill-color",  color,
                             NULL);

      clay_vector_layer_refresh (CLAY_VECTOR_LAYER (dest_viewable));
      clay_image_flush (clay_item_tree_view_get_image (item_view));
      return;
    }

  CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_color (view, color,
                                                             dest_viewable,
                                                             drop_pos);
}

static void
clay_layer_tree_view_drop_uri_list (ClayContainerTreeView   *view,
                                    GList                   *uri_list,
                                    ClayViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView  *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayContainerView *cont_view = CLAY_CONTAINER_VIEW (view);
  ClayImage         *image     = clay_item_tree_view_get_image (item_view);
  ClayLayer         *parent;
  gint               index;
  GList             *list;

  index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (ClayViewable **) &parent);

  g_object_ref (image);

  for (list = uri_list; list; list = g_list_next (list))
    {
      const gchar       *uri   = list->data;
      GFile             *file  = g_file_new_for_uri (uri);
      GList             *new_layers;
      ClayPDBStatusType  status;
      GError            *error = NULL;

      new_layers = file_open_layers (image->clay,
                                     clay_container_view_get_context (cont_view),
                                     NULL,
                                     image, FALSE, FALSE,
                                     file, CLAY_RUN_INTERACTIVE, NULL,
                                     &status, &error);

      if (new_layers)
        {
          clay_image_add_layers (image, new_layers, parent, index,
                                 0, 0,
                                 clay_image_get_width (image),
                                 clay_image_get_height (image),
                                 _("Drop layers"));

          index += g_list_length (new_layers);

          g_list_free (new_layers);
        }
      else if (status != CLAY_PDB_CANCEL)
        {
          clay_message (image->clay, G_OBJECT (view), CLAY_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  clay_image_flush (image);

  g_object_unref (image);
}

static void
clay_layer_tree_view_drop_component (ClayContainerTreeView   *tree_view,
                                     ClayImage               *src_image,
                                     ClayChannelType          component,
                                     ClayViewable            *dest_viewable,
                                     GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (tree_view);
  ClayImage        *image     = clay_item_tree_view_get_image (item_view);
  ClayChannel      *channel;
  ClayItem         *new_item;
  ClayLayer        *parent;
  gint              index;
  const gchar      *desc;

  index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (ClayViewable **) &parent);

  channel = clay_channel_new_from_component (src_image, component, NULL, NULL);

  new_item = clay_item_convert (CLAY_ITEM (channel), image,
                                CLAY_TYPE_LAYER);

  g_object_unref (channel);

  clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  clay_object_take_name (CLAY_OBJECT (new_item),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  clay_image_add_layer (image, CLAY_LAYER (new_item), parent, index, TRUE);

  clay_image_flush (image);
}

static void
clay_layer_tree_view_drop_pixbuf (ClayContainerTreeView   *tree_view,
                                  GdkPixbuf               *pixbuf,
                                  ClayViewable            *dest_viewable,
                                  GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (tree_view);
  ClayImage        *image     = clay_item_tree_view_get_image (item_view);
  ClayLayer        *new_layer;
  ClayLayer        *parent;
  gint              index;

  index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (ClayViewable **) &parent);

  new_layer =
    clay_layer_new_from_pixbuf (pixbuf, image,
                                clay_image_get_layer_format (image, TRUE),
                                _("Dropped Buffer"),
                                CLAY_OPACITY_OPAQUE,
                                clay_image_get_default_new_layer_mode (image));

  clay_image_add_layer (image, new_layer, parent, index, TRUE);

  clay_image_flush (image);
}


/*  ClayItemTreeView methods  */

static void
clay_layer_tree_view_set_image (ClayItemTreeView *view,
                                ClayImage        *image)
{
  ClayLayerTreeView *layer_view = CLAY_LAYER_TREE_VIEW (view);

  if (clay_item_tree_view_get_image (view))
    {
      g_signal_handlers_disconnect_by_func (clay_item_tree_view_get_image (view),
                                            clay_layer_tree_view_floating_selection_changed,
                                            view);
    }

  CLAY_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (clay_item_tree_view_get_image (view))
    {
      g_signal_connect (clay_item_tree_view_get_image (view),
                        "floating-selection-changed",
                        G_CALLBACK (clay_layer_tree_view_floating_selection_changed),
                        view);

      /* call clay_layer_tree_view_floating_selection_changed() now, to update
       * the floating selection's row attributes.
       */
      clay_layer_tree_view_floating_selection_changed (
        clay_item_tree_view_get_image (view),
        layer_view);
    }

  clay_layer_tree_view_update_highlight (layer_view);
}

static ClayItem *
clay_layer_tree_view_item_new (ClayImage *image)
{
  ClayLayer *new_layer;

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                               C_("singular", "New Layer"));

  new_layer = clay_layer_new (image,
                              clay_image_get_width (image),
                              clay_image_get_height (image),
                              clay_image_get_layer_format (image, TRUE),
                              NULL,
                              CLAY_OPACITY_OPAQUE,
                              clay_image_get_default_new_layer_mode (image));

  clay_image_add_layer (image, new_layer,
                        CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

  clay_image_undo_group_end (image);

  return CLAY_ITEM (new_layer);
}


/*  callbacks  */

static void
clay_layer_tree_view_floating_selection_changed (ClayImage         *image,
                                                 ClayLayerTreeView *layer_view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (layer_view);
  ClayContainerView     *view      = CLAY_CONTAINER_VIEW (layer_view);
  ClayLayer             *floating_sel;
  GtkTreeIter           *iter;

  floating_sel = clay_image_get_floating_selection (image);

  if (floating_sel)
    {
      iter = _clay_container_view_lookup (view, (ClayViewable *) floating_sel);

      if (iter)
        gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                            CLAY_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                            layer_view->priv->italic_attrs,
                            -1);
    }
  else
    {
      GList *all_layers;
      GList *list;

      all_layers = clay_image_get_layer_list (image);

      for (list = all_layers; list; list = g_list_next (list))
        {
          ClayDrawable *drawable = list->data;

          iter = _clay_container_view_lookup (view, (ClayViewable *) drawable);

          if (iter)
            gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                                CLAY_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                                clay_drawable_has_alpha (drawable) ?
                                NULL : layer_view->priv->bold_attrs,
                                -1);
        }

      g_list_free (all_layers);
    }

  clay_layer_tree_view_update_highlight (layer_view);
}


/*  Paint Mode, Opacity and Lock alpha callbacks  */

#define BLOCK() \
        g_signal_handlers_block_by_func (layer, \
        clay_layer_tree_view_layer_signal_handler, view)

#define UNBLOCK() \
        g_signal_handlers_unblock_by_func (layer, \
        clay_layer_tree_view_layer_signal_handler, view)


static void
clay_layer_tree_view_layer_mode_box_callback (GtkWidget         *widget,
                                              const GParamSpec  *pspec,
                                              ClayLayerTreeView *view)
{
  ClayImage     *image;
  GList         *layers    = NULL;
  GList         *iter;
  ClayUndo      *undo;
  gboolean       push_undo = TRUE;
  gint           n_layers  = 0;
  ClayLayerMode  mode;

  image = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));
  mode  = clay_layer_mode_box_get_mode (CLAY_LAYER_MODE_BOX (widget));
  undo  = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                        CLAY_UNDO_LAYER_MODE);

  if (image)
    layers = clay_image_get_selected_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mode (iter->data) != mode)
        {
          n_layers++;

          if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (iter->data))
            push_undo = FALSE;
        }
    }

  if (n_layers > 1)
    {
      /*  Don't compress mode undos with more than 1 layer changed. */
      push_undo = TRUE;

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_OPACITY,
                                   _("Set layers mode"));
    }

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *layer = iter->data;

      if (clay_layer_get_mode (layer) != mode)
        {
          BLOCK();
          clay_layer_set_mode (layer, (ClayLayerMode) mode, push_undo);
          UNBLOCK();
        }
    }
  clay_image_flush (image);

  if (! push_undo)
    clay_undo_refresh_preview (undo, clay_container_view_get_context (CLAY_CONTAINER_VIEW (view)));

  if (n_layers > 1)
    clay_image_undo_group_end (image);
}

static void
clay_layer_tree_view_opacity_scale_changed (GtkAdjustment     *adjustment,
                                            ClayLayerTreeView *view)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  ClayUndo  *undo;
  gboolean   push_undo = TRUE;
  gint       n_layers = 0;
  gdouble    opacity;

  image   = clay_item_tree_view_get_image (CLAY_ITEM_TREE_VIEW (view));
  layers  = clay_image_get_selected_layers (image);
  undo    = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                          CLAY_UNDO_LAYER_OPACITY);
  opacity = gtk_adjustment_get_value (adjustment) / 100.0;

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_opacity (iter->data) != opacity)
        {
          n_layers++;

          if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (iter->data))
            push_undo = FALSE;
        }
    }
  if (n_layers > 1)
    {
      /*  Don't compress opacity undos with more than 1 layer changed. */
      push_undo = TRUE;

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_OPACITY,
                                   _("Set layers opacity"));
    }

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *layer = iter->data;

      if (clay_layer_get_opacity (layer) != opacity)
        {
          BLOCK();
          clay_layer_set_opacity (layer, opacity, push_undo);
          UNBLOCK();
        }
    }
  clay_image_flush (image);

  if (! push_undo)
    clay_undo_refresh_preview (undo, clay_container_view_get_context (CLAY_CONTAINER_VIEW (view)));

  if (n_layers > 1)
    clay_image_undo_group_end (image);
}

#undef BLOCK
#undef UNBLOCK


static void
clay_layer_tree_view_layer_signal_handler (ClayLayer         *layer,
                                           ClayLayerTreeView *view)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (view);
  GList            *selected_layers;

  selected_layers =
    clay_image_get_selected_layers (clay_item_tree_view_get_image (item_view));

  if (g_list_find (selected_layers, layer))
    clay_layer_tree_view_update_options (view, selected_layers);
}


#define BLOCK(object,function) \
        g_signal_handlers_block_by_func ((object), (function), view)

#define UNBLOCK(object,function) \
        g_signal_handlers_unblock_by_func ((object), (function), view)

static void
clay_layer_tree_view_update_options (ClayLayerTreeView *view,
                                     GList             *layers)
{
  GList                *iter;
  ClayLayerMode         mode = CLAY_LAYER_MODE_SEPARATOR;
  ClayLayerModeContext  context = 0;
  /*gboolean              inconsistent_opacity    = FALSE;*/
  gboolean              inconsistent_mode       = FALSE;
  gdouble               opacity = -1.0;

  for (iter = layers; iter; iter = iter->next)
    {
#if 0
      if (opacity != -1.0 &&
          opacity != clay_layer_get_opacity (iter->data))
        /* We don't really have a way to show an inconsistent
         * ClaySpinScale. This is currently unused.
         */
        inconsistent_opacity = TRUE;
#endif

      opacity = clay_layer_get_opacity (iter->data);

      if (clay_viewable_get_children (iter->data))
        context |= CLAY_LAYER_MODE_CONTEXT_GROUP;
      else
        context |= CLAY_LAYER_MODE_CONTEXT_LAYER;

      if (mode != CLAY_LAYER_MODE_SEPARATOR &&
          mode != clay_layer_get_mode (iter->data))
        inconsistent_mode = TRUE;

      mode = clay_layer_get_mode (iter->data);
    }
  if (opacity == -1.0)
    opacity = 1.0;

  if (inconsistent_mode)
    mode = CLAY_LAYER_MODE_SEPARATOR;

  if (! context)
    context = CLAY_LAYER_MODE_CONTEXT_LAYER;

  BLOCK (view->priv->layer_mode_box,
         clay_layer_tree_view_layer_mode_box_callback);

  clay_layer_mode_box_set_context (CLAY_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                   context);
  clay_layer_mode_box_set_mode (CLAY_LAYER_MODE_BOX (view->priv->layer_mode_box),
                                mode);

  UNBLOCK (view->priv->layer_mode_box,
           clay_layer_tree_view_layer_mode_box_callback);

  if (opacity * 100.0 !=
      gtk_adjustment_get_value (view->priv->opacity_adjustment))
    {
      BLOCK (view->priv->opacity_adjustment,
             clay_layer_tree_view_opacity_scale_changed);

      gtk_adjustment_set_value (view->priv->opacity_adjustment,
                                opacity * 100.0);

      UNBLOCK (view->priv->opacity_adjustment,
               clay_layer_tree_view_opacity_scale_changed);
    }
}

#undef BLOCK
#undef UNBLOCK

static void
clay_layer_tree_view_update_highlight (ClayLayerTreeView *layer_view)
{
  ClayItemTreeView *item_view    = CLAY_ITEM_TREE_VIEW (layer_view);
  ClayImage        *image        = clay_item_tree_view_get_image (item_view);
  ClayLayer        *floating_sel = NULL;
  GtkReliefStyle    default_relief;

  if (image)
    floating_sel = clay_image_get_floating_selection (image);

  gtk_widget_style_get (GTK_WIDGET (layer_view),
                        "button-relief", &default_relief,
                        NULL);

  clay_button_set_suggested (clay_item_tree_view_get_new_button (item_view),
                             floating_sel &&
                             ! CLAY_IS_CHANNEL (clay_layer_get_floating_sel_drawable (floating_sel)),
                             default_relief);

  clay_button_set_destructive (clay_item_tree_view_get_delete_button (item_view),
                               floating_sel != NULL,
                               default_relief);

  clay_button_set_suggested (layer_view->priv->anchor_button,
                             floating_sel != NULL,
                             default_relief);
  if (floating_sel != NULL)
    {
      if (CLAY_IS_LAYER_MASK (clay_layer_get_floating_sel_drawable (floating_sel)))
        gtk_widget_set_tooltip_text (layer_view->priv->anchor_button,
                                     C_("layers-action", "Anchor the floating mask"));
      else
        gtk_widget_set_tooltip_text (layer_view->priv->anchor_button,
                                     C_("layers-action", "Anchor the floating layer"));
    }
  else
    {
      gtk_widget_set_tooltip_text (layer_view->priv->anchor_button,
                                   C_("layers-action", "Anchor the floating layer or mask"));
    }
}


/*  Layer Mask callbacks  */

static void
clay_layer_tree_view_mask_update (ClayLayerTreeView *layer_view,
                                  GtkTreeIter       *iter,
                                  ClayLayer         *layer)
{
  ClayContainerView     *view         = CLAY_CONTAINER_VIEW (layer_view);
  ClayContainerTreeView *tree_view    = CLAY_CONTAINER_TREE_VIEW (layer_view);
  ClayLayerMask         *mask;
  ClayViewRenderer      *renderer     = NULL;
  gboolean               mask_visible = FALSE;

  mask = clay_layer_get_mask (layer);

  if (mask)
    {
      GClosure *closure;
      gint      view_size;
      gint      border_width;

      view_size = clay_container_view_get_view_size (view, &border_width);

      mask_visible = TRUE;

      renderer = clay_view_renderer_new (clay_container_view_get_context (view),
                                         G_TYPE_FROM_INSTANCE (mask),
                                         view_size, border_width,
                                         FALSE);
      clay_view_renderer_set_viewable (renderer, CLAY_VIEWABLE (mask));

      g_signal_connect (renderer, "update",
                        G_CALLBACK (clay_layer_tree_view_renderer_update),
                        layer_view);

      closure = g_cclosure_new (G_CALLBACK (clay_layer_tree_view_mask_callback),
                                layer_view, NULL);
      g_object_watch_closure (G_OBJECT (renderer), closure);
      g_signal_connect_closure (layer, "apply-mask-changed", closure, FALSE);
      g_signal_connect_closure (layer, "edit-mask-changed",  closure, FALSE);
      g_signal_connect_closure (layer, "show-mask-changed",  closure, FALSE);
    }

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      layer_view->priv->model_column_mask,         renderer,
                      layer_view->priv->model_column_mask_visible, mask_visible,
                      -1);

  clay_layer_tree_view_update_borders (layer_view, iter);

  if (renderer)
    {
      clay_view_renderer_remove_idle (renderer);
      g_object_unref (renderer);
    }
}

static void
clay_layer_tree_view_mask_changed (ClayLayer         *layer,
                                   ClayLayerTreeView *layer_view)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = _clay_container_view_lookup (view, CLAY_VIEWABLE (layer));

  if (iter)
    clay_layer_tree_view_mask_update (layer_view, iter, layer);
}

static void
clay_layer_tree_view_renderer_update (ClayViewRenderer  *renderer,
                                      ClayLayerTreeView *layer_view)
{
  ClayContainerView     *view      = CLAY_CONTAINER_VIEW (layer_view);
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (layer_view);
  ClayLayerMask         *mask;
  GtkTreeIter           *iter;

  mask = CLAY_LAYER_MASK (renderer->viewable);

  iter = _clay_container_view_lookup (view, (ClayViewable *)
                                      clay_layer_mask_get_layer (mask));

  if (iter)
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (tree_view->model, iter);

      gtk_tree_model_row_changed (tree_view->model, path, iter);

      gtk_tree_path_free (path);
    }
}

static void
clay_layer_tree_view_update_borders (ClayLayerTreeView *layer_view,
                                     GtkTreeIter       *iter)
{
  ClayContainerTreeView *tree_view   = CLAY_CONTAINER_TREE_VIEW (layer_view);
  ClayViewRenderer      *layer_renderer;
  ClayViewRenderer      *mask_renderer;
  ClayLayer             *layer       = NULL;
  ClayLayerMask         *mask        = NULL;
  ClayViewBorderType     layer_type  = CLAY_VIEW_BORDER_BLACK;
  GtkWidget             *delete_button;
  gboolean               mask_active = FALSE;

  gtk_tree_model_get (tree_view->model, iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &layer_renderer,
                      layer_view->priv->model_column_mask,       &mask_renderer,
                      -1);

  if (layer_renderer)
    layer = CLAY_LAYER (layer_renderer->viewable);
  else if (mask_renderer && CLAY_IS_LAYER (mask_renderer->viewable))
    /* This happens when floating masks are displayed (in the mask column).
     * In such a case, the mask_renderer will actually be a layer renderer
     * showing the floating mask.
     */
    layer = CLAY_LAYER (mask_renderer->viewable);

  g_return_if_fail (layer != NULL);

  delete_button =
    clay_item_tree_view_get_delete_button (CLAY_ITEM_TREE_VIEW (layer_view));

  if (mask_renderer && CLAY_IS_LAYER_MASK (mask_renderer->viewable))
    mask = CLAY_LAYER_MASK (mask_renderer->viewable);

  if (! mask || (mask && ! clay_layer_get_edit_mask (layer)))
    layer_type = CLAY_VIEW_BORDER_WHITE;

  if (layer_renderer)
    clay_view_renderer_set_border_type (layer_renderer, layer_type);

  if (mask)
    {
      ClayViewBorderType mask_color = CLAY_VIEW_BORDER_BLACK;

      if (clay_layer_get_show_mask (layer))
        {
          mask_color = CLAY_VIEW_BORDER_GREEN;
        }
      else if (! clay_layer_get_apply_mask (layer))
        {
          mask_color = CLAY_VIEW_BORDER_RED;
        }
      else if (clay_layer_get_edit_mask (layer))
        {
          mask_color = CLAY_VIEW_BORDER_WHITE;
        }

      clay_view_renderer_set_border_type (mask_renderer, mask_color);
    }
  else if (mask_renderer)
    {
      /* Floating mask. */
      clay_view_renderer_set_border_type (mask_renderer, CLAY_VIEW_BORDER_WHITE);
    }

  /* Swap delete button based on mask or layer selection */
  if (mask && clay_layer_get_edit_mask (layer))
    mask_active = TRUE;

  gtk_widget_set_visible (delete_button, ! mask_active);
  gtk_widget_set_visible (layer_view->priv->delete_mask_button, mask_active);

  if (layer_renderer)
    g_object_unref (layer_renderer);

  if (mask_renderer)
    g_object_unref (mask_renderer);
}

static void
clay_layer_tree_view_mask_callback (ClayLayer         *layer,
                                    ClayLayerTreeView *layer_view)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = _clay_container_view_lookup (view, (ClayViewable *) layer);

  clay_layer_tree_view_update_borders (layer_view, iter);
}

static void
clay_layer_tree_view_layer_clicked (ClayCellRendererViewable *cell,
                                    const gchar              *path_str,
                                    GdkModifierType           state,
                                    ClayLayerTreeView        *layer_view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (layer_view);
  GtkTreePath           *path      = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter            iter;

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayUIManager    *ui_manager;
      ClayActionGroup  *group;
      ClayViewRenderer *renderer;

      ui_manager = clay_editor_get_ui_manager (CLAY_EDITOR (tree_view));
      group      = clay_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          ClayLayer       *layer     = CLAY_LAYER (renderer->viewable);
          ClayLayerMask   *mask      = clay_layer_get_mask (layer);
          GdkModifierType  modifiers = (state & clay_get_all_modifiers_mask ());

#if 0
/* This feature has been removed because it clashes with the
 * multi-selection (Shift/Ctrl are reserved), and we can't move it to
 * Alt+ because then it would clash with the alpha-to-selection features
 * (cf. clay_item_tree_view_item_pre_clicked()).
 * Just macro-ing them out for now, just in case, but I don't think
 * there is a chance to revive them as there is no infinite modifiers.
 */
          ClayImage       *image;

          image = clay_item_get_image (CLAY_ITEM (layer));

          if ((modifiers & GDK_MOD1_MASK))
            {
              /* Alternative functions (Alt key or equivalent) when
               * clicking on a layer preview. The actions happen only on
               * the clicked layer, not on selected layers.
               *
               * Note: Alt-click is already reserved for Alpha to
               * selection in ClayItemTreeView.
               */
              if (modifiers == (GDK_MOD1_MASK | GDK_SHIFT_MASK))
                {
                  /* Alt-Shift-click adds a layer mask with last values */
                  ClayDialogConfig *config;
                  ClayChannel      *channel = NULL;

                  if (! mask)
                    {
                      config = CLAY_DIALOG_CONFIG (image->clay->config);

                      if (config->layer_add_mask_type == CLAY_ADD_MASK_CHANNEL)
                        {
                          channel = clay_image_get_active_channel (image);

                          if (! channel)
                            {
                              ClayContainer *channels = clay_image_get_channels (image);

                              channel = CLAY_CHANNEL (clay_container_get_first_child (channels));
                            }

                          if (! channel)
                            {
                              /* No channel. We cannot perform the add
                               * mask action. */
                              g_message (_("No channels to create a layer mask from."));
                            }
                        }

                      if (config->layer_add_mask_type != CLAY_ADD_MASK_CHANNEL || channel)
                        {
                          mask = clay_layer_create_mask (layer,
                                                         config->layer_add_mask_type,
                                                         channel);

                          if (config->layer_add_mask_invert)
                            clay_channel_invert (CLAY_CHANNEL (mask), FALSE);

                          clay_layer_add_mask (layer, mask, TRUE, NULL);
                          clay_image_flush (image);
                        }
                    }
                }
              else if (modifiers == (GDK_MOD1_MASK | GDK_CONTROL_MASK))
                {
                  /* Alt-Control-click removes a layer mask */
                  if (mask)
                    {
                      clay_layer_apply_mask (layer, CLAY_MASK_DISCARD, TRUE);
                      clay_image_flush (image);
                    }
                }
              else if (modifiers == (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK))
                {
                  /* Alt-Shift-Control-click applies a layer mask */
                  if (mask)
                    {
                      clay_layer_apply_mask (layer, CLAY_MASK_APPLY, TRUE);
                      clay_image_flush (image);
                    }
                }
            }
          else
#endif
          if (! modifiers)
            {
              /* Simple clicks (without modifiers) activate the layer */
              if (mask && clay_layer_get_edit_mask (layer))
                clay_action_group_set_action_active (group,
                                                     "layers-mask-edit", FALSE);
              else
                clay_layer_tree_view_update_borders (layer_view, &iter);
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}

static void
clay_layer_tree_view_mask_clicked (ClayCellRendererViewable *cell,
                                   const gchar              *path_str,
                                   GdkModifierType           state,
                                   ClayLayerTreeView        *layer_view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (layer_view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayViewRenderer *renderer;
      ClayUIManager    *ui_manager;
      ClayActionGroup  *group;

      ui_manager = clay_editor_get_ui_manager (CLAY_EDITOR (tree_view));
      group      = clay_ui_manager_get_action_group (ui_manager, "layers");

      gtk_tree_model_get (tree_view->model, &iter,
                          CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      if (renderer)
        {
          ClayLayer       *layer     = CLAY_LAYER (renderer->viewable);
          GdkModifierType  modifiers = clay_get_all_modifiers_mask ();

          if ((state & GDK_MOD1_MASK))
            {
              ClayImage *image;

              image = clay_item_get_image (CLAY_ITEM (layer));

              if ((state & modifiers) == GDK_MOD1_MASK)
                {
                  /* Alt-click shows/hides a layer mask */
                  clay_layer_set_show_mask (layer,
                                            ! clay_layer_get_show_mask (layer),
                                            TRUE);
                  clay_image_flush (image);
                }
              else if ((state & modifiers) == (GDK_MOD1_MASK | GDK_CONTROL_MASK))
                {
                  /* Alt-Control-click enables/disables a layer mask */
                  clay_layer_set_apply_mask (layer,
                                             ! clay_layer_get_apply_mask (layer),
                                             TRUE);
                  clay_image_flush (image);
                }
            }
          else if (! clay_layer_get_edit_mask (layer))
            {
              /* Simple click selects the mask for edition. */
              clay_action_group_set_action_active (group, "layers-mask-edit",
                                                   TRUE);
            }

          g_object_unref (renderer);
        }
    }

  gtk_tree_path_free (path);
}


/*  ClayDrawable alpha callbacks  */

static void
clay_layer_tree_view_alpha_update (ClayLayerTreeView *view,
                                   GtkTreeIter       *iter,
                                   ClayLayer         *layer)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
                      clay_drawable_has_alpha (CLAY_DRAWABLE (layer)) ?
                      NULL : view->priv->bold_attrs,
                      -1);
}

static void
clay_layer_tree_view_alpha_changed (ClayLayer         *layer,
                                    ClayLayerTreeView *layer_view)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (layer_view);
  GtkTreeIter       *iter;

  iter = _clay_container_view_lookup (view, (ClayViewable *) layer);

  if (iter)
    {
      ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (view);
      ClayImage        *image;
      GList            *layers;

      clay_layer_tree_view_alpha_update (layer_view, iter, layer);

      image  = clay_item_tree_view_get_image (item_view);
      layers = clay_image_get_selected_layers (image);

      /*  update button states  */
      if (g_list_find (layers, layer))
        clay_container_view_set_selected (CLAY_CONTAINER_VIEW (view),
                                          layers);
    }
}
