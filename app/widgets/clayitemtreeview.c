/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitemtreeview.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "clayhelp-ids.h"
#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/clayitem.h"
#include "core/clayitem-exclusive.h"
#include "core/clayitemundo.h"
#include "core/claylist.h"
#include "core/claytreehandler.h"

#include "clayaction.h"
#include "claycontainertreestore.h"
#include "claycontainerview.h"
#include "claycontainerview-cruft.h"
#include "claydnd.h"
#include "claydocked.h"
#include "clayitemtreeview.h"
#include "clayitemtreeview-search.h"
#include "claylayertreeview.h"
#include "claymenufactory.h"
#include "clayviewrenderer.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  SET_IMAGE,
  LAST_SIGNAL
};


struct _ClayItemTreeViewPrivate
{
  ClayImage       *image;

  GtkWidget       *options_box;
  GtkSizeGroup    *options_group;

  GtkWidget       *eye_header_image;
  GtkWidget       *lock_header_image;

  ClayItem        *lock_box_item;
  GtkTreePath     *lock_box_path;
  GtkWidget       *lock_popover;
  GtkWidget       *lock_box;

  GList           *locks;

  GtkWidget       *new_button;
  GtkWidget       *raise_button;
  GtkWidget       *lower_button;
  GtkWidget       *duplicate_button;
  GtkWidget       *delete_button;

  gint             model_column_visible;
  gint             model_column_viewable;
  gint             model_column_locked;
  gint             model_column_lock_icon;
  gint             model_column_color_tag;
  GtkCellRenderer *eye_cell;
  GtkCellRenderer *lock_cell;

  ClayTreeHandler *visible_changed_handler;
  ClayTreeHandler *color_tag_changed_handler;

  GtkWidget       *multi_selection_label;

  GtkWidget       *search_button;

  gint             block_selection_changed;
};

typedef struct
{
  GtkWidget        *toggle;
  const gchar      *icon_name;

  ClayIsLockedFunc  is_locked;
  ClayCanLockFunc   can_lock;
  ClaySetLockFunc   lock;
  ClayUndoLockPush  undo_push;

  const gchar      *tooltip;
  const gchar      *help_id;

  /* Signal handling when lock is changed from core. */
  const gchar      *signal_name;
  ClayTreeHandler  *changed_handler;

  /* Undo types and labels. */
  ClayUndoType      undo_type;
  ClayUndoType      group_undo_type;
  const gchar      *undo_lock_desc;
  const gchar      *undo_unlock_desc;
  const gchar      *undo_exclusive_desc;
} LockToggle;


static void   clay_item_tree_view_view_iface_init   (ClayContainerViewInterface *view_iface);
static void   clay_item_tree_view_docked_iface_init (ClayDockedInterface *docked_iface);

static void   clay_item_tree_view_constructed       (GObject           *object);
static void   clay_item_tree_view_dispose           (GObject           *object);

static void   clay_item_tree_view_style_updated     (GtkWidget         *widget);

static void   clay_item_tree_view_real_set_image    (ClayItemTreeView  *view,
                                                     ClayImage         *image);
static gboolean clay_item_tree_view_key_press_event (GtkTreeView       *tree_view,
                                                     GdkEventKey       *event,
                                                     ClayItemTreeView *view);

static void   clay_item_tree_view_image_flush       (ClayImage         *image,
                                                     gboolean           invalidate_preview,
                                                     ClayItemTreeView  *view);
static gboolean
              clay_item_tree_view_image_flush_idle  (gpointer           user_data);

static void   clay_item_tree_view_selection_changed (ClayContainerView *view);
static void   clay_item_tree_view_item_activated    (ClayContainerView *view,
                                                     ClayViewable      *item);

static void   clay_item_tree_view_set_container     (ClayContainerView *view,
                                                     ClayContainer     *container);
static void   clay_item_tree_view_set_context       (ClayContainerView *view,
                                                     ClayContext       *context);

static gpointer clay_item_tree_view_insert_item     (ClayContainerView *view,
                                                     ClayViewable      *viewable,
                                                     gpointer           parent_insert_data,
                                                     gint               index);
static void  clay_item_tree_view_insert_items_after (ClayContainerView *view);

static gboolean clay_item_tree_view_drop_possible   (ClayContainerTreeView *view,
                                                     ClayDndType        src_type,
                                                     GList             *src_viewables,
                                                     ClayViewable      *dest_viewable,
                                                     GtkTreePath       *drop_path,
                                                     GtkTreeViewDropPosition  drop_pos,
                                                     GtkTreeViewDropPosition *return_drop_pos,
                                                     GdkDragAction     *return_drag_action);
static void     clay_item_tree_view_drop_viewables  (ClayContainerTreeView   *view,
                                                     GList                   *src_viewables,
                                                     ClayViewable            *dest_viewable,
                                                     GtkTreeViewDropPosition  drop_pos);

static void   clay_item_tree_view_new_dropped       (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     ClayViewable      *viewable,
                                                     gpointer           data);
static void clay_item_tree_view_new_list_dropped    (GtkWidget         *widget,
                                                     gint               x,
                                                     gint               y,
                                                     GList             *viewables,
                                                     gpointer           data);

static void   clay_item_tree_view_items_changed     (ClayImage         *image,
                                                     ClayItemTreeView  *view);
static void   clay_item_tree_view_size_changed      (ClayImage         *image,
                                                     ClayItemTreeView  *view);

static void   clay_item_tree_view_name_edited       (GtkCellRendererText *cell,
                                                     const gchar       *path,
                                                     const gchar       *new_name,
                                                     ClayItemTreeView  *view);

static void   clay_item_tree_view_visible_changed   (ClayItem          *item,
                                                     ClayItemTreeView  *view);
static void   clay_item_tree_view_color_tag_changed (ClayItem          *item,
                                                     ClayItemTreeView  *view);
static void   clay_item_tree_view_lock_changed      (ClayItem          *item,
                                                     ClayItemTreeView  *view);

static void   clay_item_tree_view_eye_clicked       (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     ClayItemTreeView  *view);
static void   clay_item_tree_view_lock_clicked      (GtkCellRendererToggle *toggle,
                                                     gchar             *path,
                                                     GdkModifierType    state,
                                                     ClayItemTreeView  *view);
static gboolean clay_item_tree_view_lock_button_release (GtkWidget        *widget,
                                                         GdkEvent         *event,
                                                         ClayItemTreeView *view);
static void   clay_item_tree_view_lock_toggled      (GtkWidget         *widget,
                                                     ClayItemTreeView  *view);
static void   clay_item_tree_view_update_lock_box   (ClayItemTreeView  *view,
                                                     ClayItem          *item,
                                                     GtkTreePath       *path);
static gboolean clay_item_tree_view_popover_button_press (GtkWidget        *widget,
                                                          GdkEvent         *event,
                                                          ClayItemTreeView *view);

static gboolean clay_item_tree_view_item_pre_clicked(ClayCellRendererViewable *cell,
                                                     const gchar              *path_str,
                                                     GdkModifierType           state,
                                                     ClayItemTreeView         *item_view);

static void   clay_item_tree_view_row_expanded      (GtkTreeView       *tree_view,
                                                     GtkTreeIter       *iter,
                                                     GtkTreePath       *path,
                                                     ClayItemTreeView  *item_view);

static gint   clay_item_tree_view_get_n_locks       (ClayItemTreeView *view,
                                                     ClayItem         *item,
                                                     const gchar     **icon_name);

/* Functions for the item search/selection feature. */

static void     clay_item_tree_view_floating_selection_changed (ClayImage        *image,
                                                                ClayItemTreeView *view);
static void     clay_item_tree_view_item_links_changed         (ClayImage        *image,
                                                                GType             item_type,
                                                                ClayItemTreeView *view);
static gboolean clay_item_tree_view_start_interactive_search   (GtkTreeView      *tree_view,
                                                                ClayItemTreeView *view);
static gboolean clay_item_tree_view_search_clicked             (GtkWidget        *button,
                                                                GdkEventButton   *event,
                                                                ClayItemTreeView *view);

static gboolean clay_item_tree_view_move_cursor                (ClayItemTreeView *tree_view,
                                                                guint             keyval,
                                                                GdkModifierType   modifiers);
static void     clay_item_tree_view_blink_item_column          (ClayItemTreeView *view,
                                                                ClayItem         *item,
                                                                gint              column);

static GList  * clay_item_tree_view_get_allowed_types          (GType             gtype);


G_DEFINE_TYPE_WITH_CODE (ClayItemTreeView, clay_item_tree_view,
                         CLAY_TYPE_CONTAINER_TREE_VIEW,
                         G_ADD_PRIVATE (ClayItemTreeView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_item_tree_view_view_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_item_tree_view_docked_iface_init))

#define parent_class clay_item_tree_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
clay_item_tree_view_class_init (ClayItemTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass             *widget_class = GTK_WIDGET_CLASS (klass);
  ClayContainerTreeViewClass *tree_view_class;

  tree_view_class = CLAY_CONTAINER_TREE_VIEW_CLASS (klass);

  view_signals[SET_IMAGE] =
    g_signal_new ("set-image",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayItemTreeViewClass, set_image),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_OBJECT);

  object_class->constructed       = clay_item_tree_view_constructed;
  object_class->dispose           = clay_item_tree_view_dispose;

  widget_class->style_updated     = clay_item_tree_view_style_updated;

  tree_view_class->drop_possible  = clay_item_tree_view_drop_possible;
  tree_view_class->drop_viewables = clay_item_tree_view_drop_viewables;

  klass->set_image                = clay_item_tree_view_real_set_image;

  klass->item_type                = G_TYPE_NONE;
  klass->signal_name              = NULL;

  klass->new_item                 = NULL;

  klass->action_group        = NULL;
  klass->new_action          = NULL;
  klass->new_default_action  = NULL;
  klass->raise_action        = NULL;
  klass->raise_top_action    = NULL;
  klass->lower_action        = NULL;
  klass->lower_bottom_action = NULL;
  klass->duplicate_action    = NULL;
  klass->delete_action       = NULL;

  klass->move_cursor_up_action        = NULL;
  klass->move_cursor_down_action      = NULL;
  klass->move_cursor_up_flat_action   = NULL;
  klass->move_cursor_down_flat_action = NULL;
  klass->move_cursor_start_action     = NULL;
  klass->move_cursor_end_action       = NULL;

  klass->lock_content_icon_name  = NULL;
  klass->lock_content_tooltip    = NULL;
  klass->lock_content_help_id    = NULL;

  klass->lock_position_icon_name  = NULL;
  klass->lock_position_tooltip    = NULL;
  klass->lock_position_help_id    = NULL;

  klass->lock_visibility_icon_name = NULL;
  klass->lock_visibility_tooltip   = NULL;
  klass->lock_visibility_help_id   = NULL;
}

static void
clay_item_tree_view_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->selection_changed  = clay_item_tree_view_selection_changed;
  iface->item_activated     = clay_item_tree_view_item_activated;
  iface->set_container      = clay_item_tree_view_set_container;
  iface->set_context        = clay_item_tree_view_set_context;

  iface->insert_item        = clay_item_tree_view_insert_item;
  iface->insert_items_after = clay_item_tree_view_insert_items_after;
}

static void
clay_item_tree_view_docked_iface_init (ClayDockedInterface *iface)
{
  iface->get_preview = NULL;
}

static void
clay_item_tree_view_init (ClayItemTreeView *view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);

  view->priv = clay_item_tree_view_get_instance_private (view);

  view->priv->model_column_visible =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_viewable =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_locked =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_BOOLEAN);

  view->priv->model_column_lock_icon =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           G_TYPE_STRING);

  view->priv->model_column_color_tag =
    clay_container_tree_store_columns_add (tree_view->model_columns,
                                           &tree_view->n_model_columns,
                                           GDK_TYPE_RGBA);

  clay_container_tree_view_set_dnd_drop_to_empty (tree_view, TRUE);
}

static void
clay_item_tree_view_constructed (GObject *object)
{
  ClayItemTreeViewClass *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (object);
  ClayEditor            *editor          = CLAY_EDITOR (object);
  ClayContainerTreeView *tree_view       = CLAY_CONTAINER_TREE_VIEW (object);
  ClayItemTreeView      *item_view       = CLAY_ITEM_TREE_VIEW (object);
  GtkTreeViewColumn     *column;
  GtkWidget             *image;
  GtkWidget             *items_header;
  GtkIconSize            button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gint                   pixel_icon_size  = 16;
  gint                   button_spacing;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect (tree_view->view, "key-press-event",
                    G_CALLBACK (clay_item_tree_view_key_press_event),
                    tree_view);

  gtk_tree_view_set_headers_visible (tree_view->view, TRUE);

  gtk_widget_style_get (GTK_WIDGET (item_view),
                        "button-icon-size", &button_icon_size,
                        "button-spacing", &button_spacing,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

  clay_container_tree_view_connect_name_edited (tree_view,
                                                G_CALLBACK (clay_item_tree_view_name_edited),
                                                item_view);

  g_signal_connect (tree_view->view, "row-expanded",
                    G_CALLBACK (clay_item_tree_view_row_expanded),
                    tree_view);

  g_signal_connect (tree_view->renderer_cell, "pre-clicked",
                    G_CALLBACK (clay_item_tree_view_item_pre_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  image = gtk_image_new_from_icon_name (CLAY_ICON_VISIBLE, button_icon_size);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_set_visible (image, TRUE);
  gtk_tree_view_insert_column (tree_view->view, column, 0);
  item_view->priv->eye_header_image = image;

  item_view->priv->eye_cell = clay_cell_renderer_toggle_new (CLAY_ICON_VISIBLE);
  g_object_set (item_view->priv->eye_cell,
                "xpad",                0,
                "ypad",                0,
                "icon-size",           pixel_icon_size,
                "override-background", TRUE,
                NULL);
  gtk_tree_view_column_pack_start (column, item_view->priv->eye_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, item_view->priv->eye_cell,
                                       "active",
                                       item_view->priv->model_column_visible,
                                       "inconsistent",
                                       item_view->priv->model_column_viewable,
                                       "cell-background-rgba",
                                       item_view->priv->model_column_color_tag,
                                       NULL);

  clay_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->eye_cell);

  g_signal_connect (item_view->priv->eye_cell, "clicked",
                    G_CALLBACK (clay_item_tree_view_eye_clicked),
                    item_view);

  column = gtk_tree_view_column_new ();
  image = gtk_image_new_from_icon_name (CLAY_ICON_LOCK, button_icon_size);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_set_visible (image, TRUE);
  gtk_tree_view_insert_column (tree_view->view, column, 1);
  item_view->priv->lock_header_image = image;

  item_view->priv->lock_cell = clay_cell_renderer_toggle_new (CLAY_ICON_LOCK_MULTI);
  g_object_set (item_view->priv->lock_cell,
                "xpad",      0,
                "ypad",      0,
                "icon-size", pixel_icon_size,
                NULL);
  gtk_tree_view_column_pack_start (column, item_view->priv->lock_cell, FALSE);
  gtk_tree_view_column_set_attributes (column, item_view->priv->lock_cell,
                                       "active",
                                       item_view->priv->model_column_locked,
                                       "icon-name",
                                       item_view->priv->model_column_lock_icon,
                                       NULL);

  clay_container_tree_view_add_toggle_cell (tree_view,
                                            item_view->priv->lock_cell);

  g_signal_connect (item_view->priv->lock_cell, "clicked",
                    G_CALLBACK (clay_item_tree_view_lock_clicked),
                    item_view);

  /*  disable the default ClayContainerView drop handler  */
  clay_container_view_set_dnd_widget (CLAY_CONTAINER_VIEW (item_view), NULL);

  clay_dnd_drag_dest_set_by_type (GTK_WIDGET (tree_view->view),
                                  GTK_DEST_DEFAULT_HIGHLIGHT,
                                  item_view_class->item_type,
                                  TRUE,
                                  GDK_ACTION_MOVE | GDK_ACTION_COPY);

  item_view->priv->new_button =
    clay_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->new_action,
                                   item_view_class->new_default_action,
                                   GDK_SHIFT_MASK,
                                   NULL);
  /*  connect "drop to new" manually as it makes a difference whether
   *  it was clicked or dropped
   */
  clay_dnd_viewable_list_dest_add (item_view->priv->new_button,
                                   item_view_class->item_type,
                                   clay_item_tree_view_new_list_dropped,
                                   item_view);
  clay_dnd_viewable_dest_add (item_view->priv->new_button,
                              item_view_class->item_type,
                              clay_item_tree_view_new_dropped,
                              item_view);

  item_view->priv->raise_button =
    clay_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->raise_action,
                                   item_view_class->raise_top_action,
                                   GDK_SHIFT_MASK,
                                   NULL);

  item_view->priv->lower_button =
    clay_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->lower_action,
                                   item_view_class->lower_bottom_action,
                                   GDK_SHIFT_MASK,
                                   NULL);

  item_view->priv->duplicate_button =
    clay_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->duplicate_action, NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (item_view),
                                  GTK_BUTTON (item_view->priv->duplicate_button),
                                  item_view_class->item_type);

  item_view->priv->delete_button =
    clay_editor_add_action_button (editor, item_view_class->action_group,
                                   item_view_class->delete_action, NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (item_view),
                                  GTK_BUTTON (item_view->priv->delete_button),
                                  item_view_class->item_type);

  item_view->priv->lock_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                           button_spacing);
  clay_item_tree_view_add_lock (item_view,
                                item_view_class->lock_content_icon_name,
                                (ClayIsLockedFunc) clay_item_get_lock_content,
                                (ClayCanLockFunc)  clay_item_can_lock_content,
                                (ClaySetLockFunc)  clay_item_set_lock_content,
                                (ClayUndoLockPush) clay_image_undo_push_item_lock_content,
                                "lock-content-changed",
                                CLAY_UNDO_ITEM_LOCK_CONTENT,
                                CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                                _("Lock content"),
                                _("Unlock content"),
                                _("Set Item Exclusive Content Lock"),
                                item_view_class->lock_content_tooltip,
                                item_view_class->lock_content_help_id);
  clay_item_tree_view_add_lock (item_view,
                                item_view_class->lock_position_icon_name,
                                (ClayIsLockedFunc) clay_item_get_lock_position,
                                (ClayCanLockFunc)  clay_item_can_lock_position,
                                (ClaySetLockFunc)  clay_item_set_lock_position,
                                (ClayUndoLockPush) clay_image_undo_push_item_lock_position,
                                "lock-position-changed",
                                CLAY_UNDO_ITEM_LOCK_POSITION,
                                CLAY_UNDO_GROUP_ITEM_LOCK_POSITION,
                                _("Lock position"),
                                _("Unlock position"),
                                _("Set Item Exclusive Position Lock"),
                                item_view_class->lock_position_tooltip,
                                item_view_class->lock_position_help_id);
  clay_item_tree_view_add_lock (item_view,
                                item_view_class->lock_visibility_icon_name,
                                (ClayIsLockedFunc) clay_item_get_lock_visibility,
                                (ClayCanLockFunc)  clay_item_can_lock_visibility,
                                (ClaySetLockFunc)  clay_item_set_lock_visibility,
                                (ClayUndoLockPush) clay_image_undo_push_item_lock_visibility,
                                "lock-visibility-changed",
                                CLAY_UNDO_ITEM_LOCK_VISIBILITY,
                                CLAY_UNDO_GROUP_ITEM_LOCK_VISIBILITY,
                                _("Lock visibility"),
                                _("Unlock visibility"),
                                _("Set Item Exclusive Visibility Lock"),
                                item_view_class->lock_visibility_tooltip,
                                item_view_class->lock_visibility_help_id);

  item_view->priv->lock_popover = gtk_popover_new (GTK_WIDGET (tree_view->view));
  gtk_popover_set_modal (GTK_POPOVER (item_view->priv->lock_popover), TRUE);
  gtk_container_add (GTK_CONTAINER (item_view->priv->lock_popover),
                     item_view->priv->lock_box);
  gtk_widget_set_visible (item_view->priv->lock_box, TRUE);

  g_signal_connect (item_view->priv->lock_popover,
                    "button-press-event",
                    G_CALLBACK (clay_item_tree_view_popover_button_press),
                    item_view);

  items_header = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_widget_set_hexpand (items_header, TRUE);
  gtk_widget_set_halign (items_header, GTK_ALIGN_FILL);
  gtk_widget_set_margin_end (items_header, 1);

  /* Link popover menu. */

  item_view->priv->search_button =
    gtk_image_new_from_icon_name (CLAY_ICON_EDIT_FIND,
                                  button_icon_size);
  gtk_widget_set_tooltip_text (item_view->priv->search_button,
                               _("Select items by patterns and store item sets"));
  gtk_box_pack_end (GTK_BOX (items_header), item_view->priv->search_button,
                    FALSE, FALSE, 2);

  g_signal_connect (CLAY_CONTAINER_TREE_VIEW (item_view)->view,
                    "start-interactive-search",
                    G_CALLBACK (clay_item_tree_view_start_interactive_search),
                    item_view);

  _clay_item_tree_view_search_create (item_view,
                                      item_view->priv->search_button);

  /* FIXME: Currently the search feature only works on layers.  In the
   * future, we should expand this to channels and paths dockables as
   * well
   */
  if (CLAY_IS_LAYER_TREE_VIEW (object))
    gtk_widget_set_visible (item_view->priv->search_button, TRUE);

  /* Search label */

  item_view->priv->multi_selection_label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (item_view->priv->multi_selection_label),
                            FALSE);
  gtk_widget_set_halign (item_view->priv->multi_selection_label, GTK_ALIGN_START);
  gtk_box_pack_end (GTK_BOX (items_header),
                    item_view->priv->multi_selection_label,
                    FALSE, FALSE, 4);
  gtk_widget_set_visible (item_view->priv->multi_selection_label, TRUE);

  column = CLAY_CONTAINER_TREE_VIEW (item_view)->main_column;
  gtk_tree_view_column_set_widget (column, items_header);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_clickable (column, TRUE);

  g_signal_connect (gtk_tree_view_column_get_button (column),
                    "button-release-event",
                    G_CALLBACK (gtk_true),
                    item_view);

  /* FIXME: I don't use the "clicked" signal of the GtkTreeViewColumn
   * main_column, because the event goes through and reaches
   * clay_container_tree_view_button() code, provoking selection of
   * the top item, depending on where we click.  Catching the signal
   * at the column's button level, we manage to block the event from
   * having any side effect other than opening our popup.
   */
  if (CLAY_IS_LAYER_TREE_VIEW (object))
    g_signal_connect (gtk_tree_view_column_get_button (column),
                      "button-press-event",
                      G_CALLBACK (clay_item_tree_view_search_clicked),
                      item_view);
  gtk_widget_set_visible (items_header, TRUE);
}

static void
clay_item_tree_view_dispose (GObject *object)
{
  ClayItemTreeView *view = CLAY_ITEM_TREE_VIEW (object);

  if (view->priv->image)
    clay_item_tree_view_set_image (view, NULL);

  _clay_item_tree_view_search_destroy (view);

  g_clear_pointer (&view->priv->lock_box_path, gtk_tree_path_free);

  if (view->priv->locks)
    {
      g_list_free_full (view->priv->locks, g_free);
      view->priv->locks = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_item_tree_view_style_updated (GtkWidget *widget)
{
  ClayItemTreeView *view = CLAY_ITEM_TREE_VIEW (widget);
  GList            *children;
  GList            *list;
  const gchar      *old_icon_name;
  gchar            *icon_name;
  GtkReliefStyle    button_relief;
  GtkIconSize       old_size;
  GtkIconSize       button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gint              pixel_icon_size  = 16;
  gint              content_spacing;
  gint              button_spacing;

  gtk_widget_style_get (widget,
                        "button-relief",    &button_relief,
                        "button-icon-size", &button_icon_size,
                        "content-spacing",  &content_spacing,
                        "button-spacing",   &button_spacing,
                        NULL);

  if (view->priv->options_box)
    {
      gtk_box_set_spacing (GTK_BOX (view->priv->options_box), content_spacing);

      children =
        gtk_container_get_children (GTK_CONTAINER (view->priv->options_box));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;

          if (GTK_IS_BOX (child))
            gtk_box_set_spacing (GTK_BOX (child), button_spacing);
        }

      g_list_free (children);
    }

  if (view->priv->lock_box)
    {
      gtk_box_set_spacing (GTK_BOX (view->priv->lock_box), button_spacing);

      children =
        gtk_container_get_children (GTK_CONTAINER (view->priv->lock_box));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;

          if (GTK_IS_BUTTON (child))
            {
              GtkWidget *image;

              gtk_button_set_relief (GTK_BUTTON (child), button_relief);

              image = gtk_bin_get_child (GTK_BIN (child));

              if (GTK_IS_IMAGE (image))
                {
                  GtkIconSize  old_size;

                  gtk_image_get_icon_name (GTK_IMAGE (image),
                                           &old_icon_name, &old_size);
                  if (button_icon_size != old_size)
                    {
                      icon_name = g_strdup (old_icon_name);
                      gtk_image_set_from_icon_name (GTK_IMAGE (image),
                                                    icon_name, button_icon_size);
                      g_free (icon_name);
                    }
                }
            }
        }

      g_list_free (children);
    }

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->lock_header_image),
                           &old_icon_name, &old_size);
  if (button_icon_size != old_size)
    {
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->lock_header_image),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->eye_header_image),
                           &old_icon_name, &old_size);
  if (button_icon_size != old_size)
    {
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->eye_header_image),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  gtk_image_get_icon_name (GTK_IMAGE (view->priv->search_button),
                           &old_icon_name, &old_size);
  if (button_icon_size != old_size)
    {
      icon_name = g_strdup (old_icon_name);
      gtk_image_set_from_icon_name (GTK_IMAGE (view->priv->search_button),
                                    icon_name, button_icon_size);
      g_free (icon_name);
    }

  /* force the eye and toggle cells to recreate their icon */
  gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);
  g_object_set (view->priv->eye_cell,
                "icon-name", CLAY_ICON_VISIBLE,
                "icon-size", pixel_icon_size,
                NULL);
  g_object_set (view->priv->lock_cell,
                "icon-name", CLAY_ICON_LOCK_MULTI,
                "icon-size", pixel_icon_size,
                NULL);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);
}

GtkWidget *
clay_item_tree_view_new (GType            view_type,
                         gint             view_size,
                         gint             view_border_width,
                         gboolean         multiple_selection,
                         ClayImage       *image,
                         ClayMenuFactory *menu_factory,
                         const gchar     *menu_identifier,
                         const gchar     *ui_path)
{
  ClayItemTreeView *item_view;

  g_return_val_if_fail (g_type_is_a (view_type, CLAY_TYPE_ITEM_TREE_VIEW), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (menu_identifier != NULL, NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);

  item_view = g_object_new (view_type,
                            "reorderable",     TRUE,
                            "menu-factory",    menu_factory,
                            "menu-identifier", menu_identifier,
                            "ui-path",         ui_path,
                            "selection-mode",  multiple_selection ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE,
                            NULL);

  clay_container_view_set_view_size (CLAY_CONTAINER_VIEW (item_view),
                                     view_size, view_border_width);

  clay_item_tree_view_set_image (item_view, image);

  return GTK_WIDGET (item_view);
}

void
clay_item_tree_view_set_image (ClayItemTreeView *view,
                               ClayImage        *image)
{
  g_return_if_fail (CLAY_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  g_signal_emit (view, view_signals[SET_IMAGE], 0, image);
}

ClayImage *
clay_item_tree_view_get_image (ClayItemTreeView *view)
{
  g_return_val_if_fail (CLAY_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->image;
}

void
clay_item_tree_view_add_options (ClayItemTreeView *view,
                                 const gchar      *label,
                                 GtkWidget        *options)
{
  gint content_spacing;
  gint button_spacing;

  g_return_if_fail (CLAY_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (GTK_IS_WIDGET (options));

  gtk_widget_style_get (GTK_WIDGET (view),
                        "content-spacing", &content_spacing,
                        "button-spacing",  &button_spacing,
                        NULL);

  if (! view->priv->options_box)
    {
      ClayItemTreeViewClass *item_view_class;

      item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);

      view->priv->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, content_spacing);
      gtk_box_pack_start (GTK_BOX (view), view->priv->options_box,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (view), view->priv->options_box, 0);
      gtk_widget_set_visible (view->priv->options_box, TRUE);

      if (! view->priv->image ||
          ! clay_image_get_selected_items (view->priv->image,
                                           item_view_class->item_type))
        {
          gtk_widget_set_sensitive (view->priv->options_box, FALSE);
        }
    }

  if (label)
    {
      GtkWidget *hbox;
      GtkWidget *label_widget;
      gboolean   group_created = FALSE;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
      gtk_box_pack_start (GTK_BOX (view->priv->options_box), hbox,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      if (! view->priv->options_group)
        {
          view->priv->options_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
          group_created = TRUE;
        }

      label_widget = gtk_label_new (label);
      gtk_label_set_xalign (GTK_LABEL (label_widget), 0.0);
      gtk_size_group_add_widget (view->priv->options_group, label_widget);
      gtk_box_pack_start (GTK_BOX (hbox), label_widget, FALSE, FALSE, 0);
      gtk_widget_set_visible (label_widget, TRUE);

      if (group_created)
        g_object_unref (view->priv->options_group);

      gtk_box_pack_start (GTK_BOX (hbox), options, TRUE, TRUE, 0);
      gtk_widget_set_visible (options, TRUE);
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (view->priv->options_box), options,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (options, TRUE);
    }
}

void
clay_item_tree_view_add_lock (ClayItemTreeView *view,
                              const gchar      *icon_name,
                              ClayIsLockedFunc  is_locked,
                              ClayCanLockFunc   can_lock,
                              ClaySetLockFunc   lock,
                              ClayUndoLockPush  undo_push,
                              const gchar      *signal_name,
                              ClayUndoType      undo_type,
                              ClayUndoType      group_undo_type,
                              const gchar      *undo_lock_desc,
                              const gchar      *undo_unlock_desc,
                              const gchar      *undo_exclusive_desc,
                              const gchar      *tooltip,
                              const gchar      *help_id)
{
  LockToggle  *data;
  GtkWidget   *toggle = gtk_toggle_button_new ();
  GtkWidget   *image;
  GtkIconSize  icon_size;

  data = g_new0 (LockToggle, 1);
  data->toggle       = toggle;
  data->icon_name    = icon_name;
  data->is_locked    = is_locked;
  data->can_lock     = can_lock;
  data->lock         = lock;
  data->undo_push    = undo_push;
  data->signal_name  = signal_name;
  data->tooltip      = tooltip;
  data->help_id      = help_id;

  data->undo_type           = undo_type;
  data->group_undo_type     = group_undo_type;
  data->undo_lock_desc      = undo_lock_desc;
  data->undo_unlock_desc    = undo_unlock_desc;
  data->undo_exclusive_desc = undo_exclusive_desc;

  view->priv->locks = g_list_prepend (view->priv->locks, data);

  gtk_box_pack_end (GTK_BOX (view->priv->lock_box), toggle, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (view->priv->lock_box), toggle, 0);
  gtk_widget_set_visible (toggle, TRUE);

  g_object_set_data (G_OBJECT (toggle), "lock-data", data);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (clay_item_tree_view_lock_toggled),
                    view);
  g_signal_connect (toggle, "button-release-event",
                    G_CALLBACK (clay_item_tree_view_lock_button_release),
                    view);

  clay_help_connect (toggle, tooltip, clay_standard_help_func, help_id, NULL, NULL);

  gtk_widget_style_get (GTK_WIDGET (view),
                        "button-icon-size", &icon_size,
                        NULL);

  image = gtk_image_new_from_icon_name (icon_name, icon_size);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_set_visible (image, TRUE);
}

void
clay_item_tree_view_blink_lock (ClayItemTreeView *view,
                                ClayItem         *item)
{
  clay_item_tree_view_blink_item_column (view, item, 1);
}

void
clay_item_tree_view_blink_item (ClayItemTreeView *view,
                                ClayItem         *item)
{
  ClayContainerTreeView *container_view;
  gint                   item_index;

  container_view = CLAY_CONTAINER_TREE_VIEW (view);

  item_index =
    clay_container_tree_view_get_main_column_position (container_view);

  if (item_index > -1)
    clay_item_tree_view_blink_item_column (view, item, item_index);
}

GtkWidget *
clay_item_tree_view_get_new_button (ClayItemTreeView *view)
{
  g_return_val_if_fail (CLAY_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->new_button;
}

GtkWidget *
clay_item_tree_view_get_delete_button (ClayItemTreeView *view)
{
  g_return_val_if_fail (CLAY_IS_ITEM_TREE_VIEW (view), NULL);

  return view->priv->delete_button;
}

gint
clay_item_tree_view_get_drop_index (ClayItemTreeView         *view,
                                    ClayViewable             *dest_viewable,
                                    GtkTreeViewDropPosition   drop_pos,
                                    ClayViewable            **parent)
{
  gint index = -1;

  g_return_val_if_fail (CLAY_IS_ITEM_TREE_VIEW (view), -1);
  g_return_val_if_fail (dest_viewable == NULL ||
                        CLAY_IS_VIEWABLE (dest_viewable), -1);
  g_return_val_if_fail (parent != NULL, -1);

  *parent = NULL;

  if (dest_viewable)
    {
      *parent = clay_viewable_get_parent (dest_viewable);
      index   = clay_item_get_index (CLAY_ITEM (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
        {
          ClayContainer *children = clay_viewable_get_children (dest_viewable);

          if (children)
            {
              *parent = dest_viewable;
              index   = 0;
            }
          else
            {
              index++;
            }
        }
      else if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        {
          index++;
        }
    }

  return index;
}

static void
clay_item_tree_view_real_set_image (ClayItemTreeView *view,
                                    ClayImage        *image)
{
  ClayItemTreeViewClass *item_view_class;

  if (view->priv->image == image)
    return;

  if (view->priv->image)
    {
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            clay_item_tree_view_items_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            clay_item_tree_view_size_changed,
                                            view);

      clay_container_view_set_container (CLAY_CONTAINER_VIEW (view), NULL);

      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            clay_item_tree_view_image_flush,
                                            view);

      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            G_CALLBACK (clay_item_tree_view_item_links_changed),
                                            view);
      g_signal_handlers_disconnect_by_func (view->priv->image,
                                            G_CALLBACK (clay_item_tree_view_floating_selection_changed),
                                            view);
    }

  view->priv->image = image;

  if (view->priv->image)
    {
      ClayContainer *container;
      GType          item_type;

      item_type = CLAY_ITEM_TREE_VIEW_GET_CLASS (view)->item_type;

      container = clay_image_get_items (view->priv->image, item_type);

      clay_container_view_set_container (CLAY_CONTAINER_VIEW (view), container);

      g_signal_connect (view->priv->image,
                        CLAY_ITEM_TREE_VIEW_GET_CLASS (view)->signal_name,
                        G_CALLBACK (clay_item_tree_view_items_changed),
                        view);
      g_signal_connect (view->priv->image, "size-changed",
                        G_CALLBACK (clay_item_tree_view_size_changed),
                        view);

      g_signal_connect (view->priv->image, "flush",
                        G_CALLBACK (clay_item_tree_view_image_flush),
                        view);

      g_signal_connect (view->priv->image,
                        "item-sets-changed",
                        G_CALLBACK (clay_item_tree_view_item_links_changed),
                        view);
      g_signal_connect (view->priv->image,
                        "floating-selection-changed",
                        G_CALLBACK (clay_item_tree_view_floating_selection_changed),
                        view);

      clay_item_tree_view_items_changed (view->priv->image, view);
    }

  /* Call this even with no image, allowing to empty the link list. */
  item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);
  clay_item_tree_view_item_links_changed (view->priv->image,
                                          item_view_class->item_type,
                                          view);
  clay_item_tree_view_floating_selection_changed (view->priv->image, view);
}

static gboolean
clay_item_tree_view_key_press_event (GtkTreeView      *tree_view,
                                     GdkEventKey      *event,
                                     ClayItemTreeView *view)
{
  if (event->keyval == GDK_KEY_Up           ||
      event->keyval == GDK_KEY_KP_Up        ||
      event->keyval == GDK_KEY_Down         ||
      event->keyval == GDK_KEY_KP_Down      ||
      event->keyval == GDK_KEY_Page_Up      ||
      event->keyval == GDK_KEY_KP_Page_Up   ||
      event->keyval == GDK_KEY_Down         ||
      event->keyval == GDK_KEY_KP_Down      ||
      event->keyval == GDK_KEY_Page_Down    ||
      event->keyval == GDK_KEY_KP_Page_Down ||
      event->keyval == GDK_KEY_Home         ||
      event->keyval == GDK_KEY_KP_Home      ||
      event->keyval == GDK_KEY_End          ||
      event->keyval == GDK_KEY_KP_End)
    {
      if (event->state & clay_get_all_modifiers_mask ())
        /* Let the event propagate, though to be fair, it would be nice
         * to specify what happens with a Shift-arrow, or Ctrl-arrow,
         * with a multi-item aware logic. It may also require a concept
         * of "active" item among the selected one (very likely the last
         * clicked item).
         * TODO for a proper clay-ux spec.
         */
        return FALSE;

      clay_item_tree_view_move_cursor (view, event->keyval, event->state);

      return TRUE;
    }

  return FALSE;
}

static void
clay_item_tree_view_image_flush (ClayImage        *image,
                                 gboolean          invalidate_preview,
                                 ClayItemTreeView *view)
{
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) clay_item_tree_view_image_flush_idle,
                   g_object_ref (view), g_object_unref);
}

static gboolean
clay_item_tree_view_image_flush_idle (gpointer user_data)
{
  ClayItemTreeView *view = user_data;

  /* This needs to be run as idle because we want this to be run in the main
   * thread even when the flush happened in a thread (e.g. from the paint
   * thread).
   */
  if (clay_editor_get_ui_manager (CLAY_EDITOR (view)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (view)), view);

  return G_SOURCE_REMOVE;
}


/*  ClayContainerView methods  */

static void
clay_item_tree_view_selection_changed (ClayContainerView *view)
{
  ClayItemTreeViewClass *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);
  ClayItemTreeView      *item_view       = CLAY_ITEM_TREE_VIEW (view);

  parent_view_iface->selection_changed (view);

  if (item_view->priv->block_selection_changed == 0)
    {
      GList *items;

      clay_container_view_get_selected (view, &items);

      clay_image_set_selected_items (item_view->priv->image,
                                     item_view_class->item_type,
                                     items);

      g_list_free (items);
    }
}

static void
clay_item_tree_view_item_activated (ClayContainerView *view,
                                    ClayViewable      *item)
{
  ClayItemTreeViewClass *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);

  parent_view_iface->item_activated (view, item);

  if (item_view_class->activate_action)
    {
      clay_ui_manager_activate_action (clay_editor_get_ui_manager (CLAY_EDITOR (view)),
                                       item_view_class->action_group,
                                       item_view_class->activate_action);
    }
}

static void
clay_item_tree_view_set_container (ClayContainerView *view,
                                   ClayContainer     *container)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayContainer    *old_container;
  GList            *list;

  old_container = clay_container_view_get_container (view);

  if (old_container)
    {
      clay_tree_handler_disconnect (item_view->priv->visible_changed_handler);
      item_view->priv->visible_changed_handler = NULL;

      clay_tree_handler_disconnect (item_view->priv->color_tag_changed_handler);
      item_view->priv->color_tag_changed_handler = NULL;

      for (list = item_view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          clay_tree_handler_disconnect (data->changed_handler);
          data->changed_handler = NULL;
        }
    }

  parent_view_iface->set_container (view, container);

  if (container)
    {
      item_view->priv->visible_changed_handler =
        clay_tree_handler_connect (container, "visibility-changed",
                                   G_CALLBACK (clay_item_tree_view_visible_changed),
                                   view);

      item_view->priv->color_tag_changed_handler =
        clay_tree_handler_connect (container, "color-tag-changed",
                                   G_CALLBACK (clay_item_tree_view_color_tag_changed),
                                   view);

      for (list = item_view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          data->changed_handler = clay_tree_handler_connect (container,
                                                             data->signal_name,
                                                             G_CALLBACK (clay_item_tree_view_lock_changed),
                                                             view);
        }
    }
}

static void
clay_item_tree_view_set_context (ClayContainerView *view,
                                 ClayContext       *context)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  ClayItemTreeView      *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayImage             *image     = NULL;
  ClayContext           *old_context;

  old_context = clay_container_view_get_context (view);

  if (old_context)
    {
      g_signal_handlers_disconnect_by_func (old_context,
                                            clay_item_tree_view_set_image,
                                            item_view);
      g_signal_handlers_disconnect_by_func (old_context->clay->config,
                                            G_CALLBACK (clay_item_tree_view_style_updated),
                                            item_view);
    }

  parent_view_iface->set_context (view, context);

  if (context)
    {
      if (! tree_view->dnd_clay)
        tree_view->dnd_clay = context->clay;

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (clay_item_tree_view_set_image),
                                item_view);

      g_signal_connect_object (context->clay->config,
                               "notify::theme",
                               G_CALLBACK (clay_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->clay->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (clay_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->clay->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (clay_item_tree_view_style_updated),
                               item_view, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      image = clay_context_get_image (context);
    }

  clay_item_tree_view_set_image (item_view, image);
}

static gpointer
clay_item_tree_view_insert_item (ClayContainerView *view,
                                 ClayViewable      *viewable,
                                 gpointer           parent_insert_data,
                                 gint               index)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  ClayItemTreeView      *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayItem              *item      = CLAY_ITEM (viewable);
  GtkTreeIter           *iter;
  GeglColor             *color     = gegl_color_new ("none");
  GdkRGBA                rgba;
  gboolean               has_color;
  const gchar           *icon_name;
  gint                   n_locks;

  iter = parent_view_iface->insert_item (view, viewable,
                                         parent_insert_data, index);

  has_color = clay_get_color_tag_color (clay_item_get_merged_color_tag (item),
                                        color,
                                        clay_item_get_color_tag (item) ==
                                        CLAY_COLOR_TAG_NONE);
  gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), &rgba);
  g_object_unref (color);

  n_locks = clay_item_tree_view_get_n_locks (item_view, item, &icon_name);

  gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,

                      item_view->priv->model_column_visible,
                      clay_item_get_visible (item),

                      item_view->priv->model_column_viewable,
                      clay_item_get_visible (item) &&
                      ! clay_item_is_visible (item),

                      item_view->priv->model_column_locked,
                      n_locks > 0,

                      item_view->priv->model_column_lock_icon,
                      icon_name,

                      item_view->priv->model_column_color_tag,
                      has_color ? &rgba : NULL,
                      -1);

  return iter;
}

static void
clay_item_tree_view_insert_items_after (ClayContainerView *view)
{
  ClayItemTreeView      *item_view = CLAY_ITEM_TREE_VIEW (view);
  ClayItemTreeViewClass *item_view_class;
  GList                 *selected_items;

  item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (item_view);

  selected_items = clay_image_get_selected_items (item_view->priv->image,
                                                  item_view_class->item_type);
  clay_container_view_set_selected (view, selected_items);
}

static gboolean
clay_item_tree_view_drop_possible (ClayContainerTreeView   *tree_view,
                                   ClayDndType              src_type,
                                   GList                   *src_viewables,
                                   ClayViewable            *dest_viewable,
                                   GtkTreePath             *drop_path,
                                   GtkTreeViewDropPosition  drop_pos,
                                   GtkTreeViewDropPosition *return_drop_pos,
                                   GdkDragAction           *return_drag_action)
{
  GList    *iter;
  gboolean  other_image_items;

  if (src_viewables)
    other_image_items = TRUE;
  else
    other_image_items = FALSE;

  for (iter = src_viewables; iter; iter = iter->next)
    {
      ClayViewable *src_viewable = iter->data;

      if (! CLAY_IS_ITEM (src_viewable) ||
          (dest_viewable != NULL &&
           clay_item_get_image (CLAY_ITEM (src_viewable)) ==
           clay_item_get_image (CLAY_ITEM (dest_viewable))))
        {
          /* Not an item or from the same image. */
          other_image_items = FALSE;
          break;
        }
    }

  if (other_image_items)
    {
      if (return_drop_pos)
        *return_drop_pos = drop_pos;

      if (return_drag_action)
        *return_drag_action = GDK_ACTION_COPY;

      return TRUE;
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
clay_item_tree_view_drop_viewables (ClayContainerTreeView   *tree_view,
                                    GList                   *src_viewables,
                                    ClayViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeViewClass *item_view_class;
  ClayItemTreeView      *item_view              = CLAY_ITEM_TREE_VIEW (tree_view);
  GList                 *dropped_viewables;
  GList                 *dup_children           = NULL;
  GList                 *iter;
  ClayImage             *src_image              = NULL;
  GList                 *item_types;
  GType                  src_viewable_type      = G_TYPE_NONE;
  gint                   dest_index             = -1;

  g_return_if_fail (g_list_length (src_viewables) > 0);

  dropped_viewables = g_list_copy (src_viewables);

  item_types      =  clay_item_tree_view_get_allowed_types (CLAY_TYPE_ITEM);
  item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (item_view);

  for (iter = dropped_viewables; iter; iter = iter->next)
    {
      ClayViewable *src_viewable = iter->data;

      /* All dropped viewables must be of the same finale type and come
       * from the same source image.
       */
      if (src_viewable_type == G_TYPE_NONE)
        {
          for (GList *iter2 = item_types; iter2; iter2 = iter2->next)
            {
              GType item_type = CLAYPOINTER_TO_TYPE (iter2->data);
              if (g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable), item_type))
                {
                  src_viewable_type = item_type;
                  break;
                }
            }
          g_return_if_fail (src_viewable_type != G_TYPE_NONE);
        }
      else
        {
          g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (src_viewable),
                                         src_viewable_type));
        }

      if (src_image == NULL)
        src_image = clay_item_get_image (CLAY_ITEM (iter->data));
      else
        g_return_if_fail (src_image == clay_item_get_image (CLAY_ITEM (iter->data)));
    }

  g_list_free (item_types);

  /* What does it mean when an item and a children of this item are
   * dropped together? Does it mean we want to copy the tree structure
   * but only this child? What when it's not a copy but a move?
   * These are complicated questions for UX discussions. For the time
   * being, we just do the simple answer: all children of a group item
   * are implied, so we can just remove any descendant from the list of
   * viewables.
   */
  for (iter = dropped_viewables; iter; iter = iter->next)
    {
      GList *iter2;

      for (iter2 = dropped_viewables; iter2; iter2 = iter2->next)
        {
          if (iter->data != iter2->data &&
              clay_viewable_is_ancestor (iter2->data, iter->data))
            {
              dup_children = g_list_prepend (dup_children, iter->data);
              break;
            }
        }
    }
  for (iter = dup_children; iter; iter = iter->next)
    dropped_viewables = g_list_remove (dropped_viewables, iter->data);
  g_list_free (dup_children);

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER ||
      (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER &&
       dest_viewable                                &&
       clay_viewable_get_children (dest_viewable)))
    {
      dropped_viewables = g_list_reverse (dropped_viewables);
    }

  if (item_view->priv->image != src_image ||
      ! g_type_is_a (src_viewable_type, item_view_class->item_type))
    {
      clay_image_undo_group_start (item_view->priv->image,
                                   CLAY_UNDO_GROUP_LAYER_ADD,
                                   _("Drop layers"));

      for (iter = dropped_viewables; iter; iter = iter->next)
        {
          ClayViewable *src_viewable = iter->data;
          ClayItem     *new_item;
          ClayItem     *parent;
          GType         item_type    = item_view_class->item_type;

          if (g_type_is_a (src_viewable_type, item_type))
            item_type = G_TYPE_FROM_INSTANCE (src_viewable);

          dest_index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                           drop_pos,
                                                           (ClayViewable **) &parent);

          new_item = clay_item_convert (CLAY_ITEM (src_viewable),
                                        item_view->priv->image, item_type);

          clay_image_add_item (item_view->priv->image, new_item,
                               parent, dest_index, TRUE);
        }
    }
  else if (dest_viewable)
    {
      clay_image_undo_group_start (item_view->priv->image,
                                   CLAY_UNDO_GROUP_IMAGE_ITEM_REORDER,
                                   CLAY_ITEM_GET_CLASS (dropped_viewables->data)->reorder_desc);

      for (iter = dropped_viewables; iter; iter = iter->next)
        {
          ClayViewable *src_viewable = iter->data;
          ClayItem     *src_parent;
          ClayItem     *dest_parent;
          gint          src_index;
          gint          dest_index;

          src_parent = CLAY_ITEM (clay_viewable_get_parent (src_viewable));
          src_index  = clay_item_get_index (CLAY_ITEM (src_viewable));

          dest_index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                           drop_pos,
                                                           (ClayViewable **) &dest_parent);

          if (src_parent == dest_parent)
            {
              if (src_index < dest_index)
                dest_index--;
            }

          clay_image_reorder_item (item_view->priv->image,
                                   CLAY_ITEM (src_viewable),
                                   dest_parent,
                                   dest_index,
                                   TRUE, NULL);
        }
    }

  clay_image_undo_group_end (item_view->priv->image);
  clay_image_flush (item_view->priv->image);

  g_list_free (dropped_viewables);
}


/*  "New" functions  */

static void
clay_item_tree_view_new_dropped (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 ClayViewable *viewable,
                                 gpointer      data)
{
  ClayItemTreeViewClass *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (data);
  ClayContainerView     *view            = CLAY_CONTAINER_VIEW (data);

  if (item_view_class->new_default_action &&
      viewable && _clay_container_view_lookup (view, viewable))
    {
      ClayAction *action;

      action = clay_ui_manager_find_action (clay_editor_get_ui_manager (CLAY_EDITOR (view)),
                                            item_view_class->action_group,
                                            item_view_class->new_default_action);

      if (action)
        {
          g_object_set (action, "viewable", viewable, NULL);
          clay_action_activate (action);
          g_object_set (action, "viewable", NULL, NULL);
        }
    }
}

static void
clay_item_tree_view_new_list_dropped (GtkWidget    *widget,
                                      gint          x,
                                      gint          y,
                                      GList        *viewables,
                                      gpointer      data)
{
  ClayItemTreeViewClass *item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (data);
  ClayContainerView     *view            = CLAY_CONTAINER_VIEW (data);
  ClayAction            *action;

  action = clay_ui_manager_find_action (clay_editor_get_ui_manager (CLAY_EDITOR (view)),
                                        item_view_class->action_group,
                                        item_view_class->new_default_action);

  if (item_view_class->new_default_action && viewables && action &&
      _clay_container_view_contains (view, viewables))
    clay_action_activate (action);
}


/*  ClayImage callbacks  */

static void
clay_item_tree_view_items_changed (ClayImage        *image,
                                   ClayItemTreeView *item_view)
{
  GType  item_type;
  GList *items;

  item_type = CLAY_ITEM_TREE_VIEW_GET_CLASS (item_view)->item_type;

  items = clay_image_get_selected_items (item_view->priv->image, item_type);

  item_view->priv->block_selection_changed++;

  clay_container_view_set_selected (CLAY_CONTAINER_VIEW (item_view), items);

  item_view->priv->block_selection_changed--;

  clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (item_view)),
                          item_view);

  if (item_view->priv->options_box)
    gtk_widget_set_sensitive (item_view->priv->options_box, items != NULL);

  if (g_list_length (items) > 1)
    {
      gchar *str;

      str = g_strdup_printf (ngettext ("%d item selected", "%d items selected",
                                       g_list_length (items)),
                             g_list_length (items));
      gtk_label_set_text (GTK_LABEL (item_view->priv->multi_selection_label),
                          str);
      g_free (str);
      gtk_widget_set_opacity (item_view->priv->multi_selection_label, 1.0);
    }
  else
    {
      gtk_widget_set_opacity (item_view->priv->multi_selection_label, 0.0);
    }
}

static void
clay_item_tree_view_size_changed (ClayImage        *image,
                                  ClayItemTreeView *tree_view)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (tree_view);
  gint               view_size;
  gint               border_width;

  view_size = clay_container_view_get_view_size (view, &border_width);

  clay_container_view_set_view_size (view, view_size, border_width);
}

static void
clay_item_tree_view_name_edited (GtkCellRendererText *cell,
                                 const gchar         *path_str,
                                 const gchar         *new_name,
                                 ClayItemTreeView    *view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayViewRenderer *renderer;
      ClayItem         *item;
      const gchar      *old_name;
      GError           *error = NULL;

      renderer = clay_container_tree_store_get_renderer (CLAY_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

      item = CLAY_ITEM (renderer->viewable);

      old_name = clay_object_get_name (item);

      if (! old_name) old_name = "";
      if (! new_name) new_name = "";

      if (strcmp (old_name, new_name) &&
          clay_item_rename (item, new_name, &error))
        {
          clay_image_flush (clay_item_get_image (item));
        }
      else
        {
          gchar *name = clay_viewable_get_description (renderer->viewable, NULL);

          gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), &iter,
                              CLAY_CONTAINER_TREE_STORE_COLUMN_NAME, name,
                              -1);
          g_free (name);

          if (error)
            {
              clay_message_literal (view->priv->image->clay, G_OBJECT (view),
                                    CLAY_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
            }
        }

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}


/*  "Visible" callbacks  */

static void
clay_item_tree_view_visible_changed (ClayItem         *item,
                                     ClayItemTreeView *view)
{
  ClayContainerView     *container_view = CLAY_CONTAINER_VIEW (view);
  ClayContainerTreeView *tree_view      = CLAY_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = _clay_container_view_lookup (container_view, CLAY_VIEWABLE (item));

  if (iter)
    {
      ClayContainer *children;

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_visible,
                          clay_item_get_visible (item),
                          view->priv->model_column_viewable,
                          clay_item_get_visible (item) &&
                          ! clay_item_is_visible (item),
                          -1);

      children = clay_viewable_get_children (CLAY_VIEWABLE (item));

      if (children)
        clay_container_foreach (children,
                                (GFunc) clay_item_tree_view_visible_changed,
                                view);
    }
}

static void
clay_item_tree_view_eye_clicked (GtkCellRendererToggle *toggle,
                                 gchar                 *path_str,
                                 GdkModifierType        state,
                                 ClayItemTreeView      *view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  GtkTreePath           *path;
  GtkTreeIter            iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayContext      *context;
      ClayViewRenderer *renderer;
      ClayItem         *item;
      ClayImage        *image;
      gboolean          active;

      context = clay_container_view_get_context (CLAY_CONTAINER_VIEW (view));

      renderer = clay_container_tree_store_get_renderer (CLAY_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);
      g_object_get (toggle,
                    "active", &active,
                    NULL);

      item = CLAY_ITEM (renderer->viewable);
      g_object_unref (renderer);

      image = clay_item_get_image (item);

      if ((state & GDK_SHIFT_MASK) ||
          (state & GDK_MOD1_MASK))
        {
          clay_item_toggle_exclusive_visible (item, (state & GDK_MOD1_MASK), context);
        }
      else
        {
          ClayUndo *undo;
          gboolean  push_undo = TRUE;

          undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                               CLAY_UNDO_ITEM_VISIBILITY);

          if (undo && CLAY_ITEM_UNDO (undo)->item == item)
            push_undo = FALSE;

          if (! clay_item_set_visible (item, ! active, push_undo))
            clay_item_tree_view_blink_lock (view, item);

          if (!push_undo)
            clay_undo_refresh_preview (undo, context);
        }

      clay_image_flush (image);
    }

  gtk_tree_path_free (path);
}


/*  "Locked" callbacks  */

static void
clay_item_tree_view_lock_clicked (GtkCellRendererToggle *toggle,
                                  gchar                 *path_str,
                                  GdkModifierType        state,
                                  ClayItemTreeView      *view)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (CLAY_CONTAINER_TREE_VIEW (view)->model,
                               &iter, path))
    {
      ClayViewRenderer       *renderer;
      ClayContainerTreeStore *store;
      ClayItem               *item;
      GdkRectangle            rect;

      /* Update the lock state. */
      store = CLAY_CONTAINER_TREE_STORE (CLAY_CONTAINER_TREE_VIEW (view)->model);
      renderer = clay_container_tree_store_get_renderer (store, &iter);
      item = CLAY_ITEM (renderer->viewable);
      g_object_unref (renderer);
      clay_item_tree_view_update_lock_box (view, item, path);

      /* Change popover position. */
      gtk_tree_view_get_cell_area (CLAY_CONTAINER_TREE_VIEW (view)->view, path,
                                   gtk_tree_view_get_column (CLAY_CONTAINER_TREE_VIEW (view)->view, 1),
                                   &rect);
      gtk_tree_view_convert_bin_window_to_widget_coords (CLAY_CONTAINER_TREE_VIEW (view)->view,
                                                         rect.x, rect.y, &rect.x, &rect.y);
      gtk_popover_set_pointing_to (GTK_POPOVER (view->priv->lock_popover), &rect);

      gtk_popover_popup (GTK_POPOVER (view->priv->lock_popover));
    }
}


/*  "Color Tag" callbacks  */

static void
clay_item_tree_view_color_tag_changed (ClayItem         *item,
                                       ClayItemTreeView *view)
{
  ClayContainerView     *container_view = CLAY_CONTAINER_VIEW (view);
  ClayContainerTreeView *tree_view      = CLAY_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;

  iter = _clay_container_view_lookup (container_view, CLAY_VIEWABLE (item));

  if (iter)
    {
      ClayContainer *children;
      GeglColor     *color = gegl_color_new ("none");
      GdkRGBA        rgba;
      gboolean       has_color;

      has_color = clay_get_color_tag_color (clay_item_get_merged_color_tag (item),
                                            color,
                                            clay_item_get_color_tag (item) ==
                                            CLAY_COLOR_TAG_NONE);
      gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), &rgba);
      g_object_unref (color);

      gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,
                          view->priv->model_column_color_tag,
                          has_color ? &rgba : NULL,
                          -1);

      children = clay_viewable_get_children (CLAY_VIEWABLE (item));

      if (children)
        clay_container_foreach (children,
                                (GFunc) clay_item_tree_view_color_tag_changed,
                                view);
    }
}


/*  "Lock Content" callbacks  */

static void
clay_item_tree_view_lock_changed (ClayItem         *item,
                                  ClayItemTreeView *view)
{
  ClayContainerView     *container_view  = CLAY_CONTAINER_VIEW (view);
  ClayContainerTreeView *tree_view       = CLAY_CONTAINER_TREE_VIEW (view);
  GtkTreeIter           *iter;
  const gchar           *icon_name;
  gint                   n_locks;

  iter = _clay_container_view_lookup (container_view, CLAY_VIEWABLE (item));

  n_locks = clay_item_tree_view_get_n_locks (view, item, &icon_name);

  if (iter)
    gtk_tree_store_set (GTK_TREE_STORE (tree_view->model), iter,

                        view->priv->model_column_locked,
                        n_locks > 0,

                        view->priv->model_column_lock_icon,
                        icon_name,

                        -1);

  if (view->priv->lock_box_item == item)
    clay_item_tree_view_update_lock_box (view, item, NULL);
}

static gboolean
clay_item_tree_view_lock_button_release (GtkWidget        *widget,
                                         GdkEvent         *event,
                                         ClayItemTreeView *view)
{
  GdkEventButton  *bevent = (GdkEventButton *) event;
  LockToggle      *data;
  GdkModifierType  modifiers;

  data      = g_object_get_data (G_OBJECT (widget), "lock-data");
  modifiers = bevent->state & clay_get_all_modifiers_mask ();

  if (modifiers == GDK_SHIFT_MASK ||
      modifiers == GDK_MOD1_MASK)
    clay_item_toggle_exclusive (view->priv->lock_box_item,
                                data->is_locked,
                                data->lock,
                                data->can_lock,
                                NULL,
                                data->undo_push,
                                data->undo_exclusive_desc,
                                data->group_undo_type,
                                (modifiers == GDK_MOD1_MASK),
                                NULL);
  else
    return GDK_EVENT_PROPAGATE;

  return GDK_EVENT_STOP;
}

static void
clay_item_tree_view_lock_toggled (GtkWidget         *widget,
                                  ClayItemTreeView  *view)
{
  ClayImage   *image = view->priv->image;
  LockToggle  *data;
  ClayUndo    *undo;
  const gchar *undo_label;
  gboolean     push_undo = TRUE;
  gboolean     locked;

  locked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  data = g_object_get_data (G_OBJECT (widget), "lock-data");

  if (! data->can_lock (view->priv->lock_box_item) ||
      locked == data->is_locked (view->priv->lock_box_item))
    return;

  undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                       data->undo_type);

  if (undo && CLAY_ITEM_UNDO (undo)->item == view->priv->lock_box_item)
    push_undo = FALSE;

  if (push_undo)
    {
      if (locked)
        undo_label = data->undo_lock_desc;
      else
        undo_label = data->undo_unlock_desc;

      clay_image_undo_group_start (image, data->group_undo_type,
                                   undo_label);
    }

  /* TODO: maybe we should also have a modifier-interaction where we can
   * lock all same-level items (Shift-click for instance like eye click)
   * and maybe another modifier for all selected items at once.
   */
  data->lock (view->priv->lock_box_item, locked, push_undo);

  clay_image_flush (image);
  if (push_undo)
    clay_image_undo_group_end (image);
}

static gboolean
clay_item_tree_view_item_pre_clicked (ClayCellRendererViewable *cell,
                                      const gchar              *path_str,
                                      GdkModifierType           state,
                                      ClayItemTreeView         *item_view)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (item_view);
  GtkTreePath           *path;
  GtkTreeIter            iter;
  GdkModifierType        modifiers = (state & clay_get_all_modifiers_mask ());
  gboolean               handled   = FALSE;

  path = gtk_tree_path_new_from_string (path_str);

  /* Alt modifier at least and none other than Alt, Shift and Ctrl. */
  if ((modifiers & GDK_MOD1_MASK)                                          &&
      ! (modifiers & ~(GDK_MOD1_MASK | GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
      gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      ClayImage        *image    = clay_item_tree_view_get_image (item_view);
      ClayViewRenderer *renderer = NULL;

      renderer = clay_container_tree_store_get_renderer (CLAY_CONTAINER_TREE_STORE (tree_view->model),
                                                         &iter);

      if (renderer)
        {
          ClayItem       *item = CLAY_ITEM (renderer->viewable);
          ClayChannelOps  op   = clay_modifiers_to_channel_op (state);

          clay_item_to_selection (item, op,
                                  TRUE, FALSE, 0.0, 0.0);
          clay_image_flush (image);

          g_object_unref (renderer);

          /* Don't select the clicked layer */
          handled = TRUE;
        }
    }

  gtk_tree_path_free (path);

  return handled;
}

static void
clay_item_tree_view_update_lock_box (ClayItemTreeView *view,
                                     ClayItem         *item,
                                     GtkTreePath      *path)
{
  GList *list;

  for (list = view->priv->locks; list; list = list->next)
    {
      LockToggle *data = list->data;

      if (data->is_locked (item) !=
          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->toggle)))
        {
          g_signal_handlers_block_by_func (data->toggle,
                                           clay_item_tree_view_lock_toggled,
                                           view);
          g_signal_handlers_block_by_func (data->toggle,
                                           clay_item_tree_view_lock_button_release,
                                           view);

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->toggle),
                                        data->is_locked (item));

          g_signal_handlers_unblock_by_func (data->toggle,
                                             clay_item_tree_view_lock_toggled,
                                             view);
          g_signal_handlers_unblock_by_func (data->toggle,
                                           clay_item_tree_view_lock_button_release,
                                           view);
        }
      gtk_widget_set_sensitive (data->toggle, data->can_lock (item));
    }
  view->priv->lock_box_item = item;

  if (path)
    {
      if (view->priv->lock_box_path)
        gtk_tree_path_free (view->priv->lock_box_path);
      view->priv->lock_box_path = path;
    }
}

static gboolean
clay_item_tree_view_popover_button_press (GtkWidget        *widget,
                                          GdkEvent         *event,
                                          ClayItemTreeView *view)
{
  ClayContainerTreeView *tree_view    = CLAY_CONTAINER_TREE_VIEW (view);
  GdkEventButton        *bevent       = (GdkEventButton *) event;
  GdkEvent              *new_event;
  GtkTreeViewColumn     *column;
  GtkTreePath           *path;

  /* If we get to the popover signal handling, it means we didn't click
   * inside popover's buttons, which would have stopped the signal
   * first. So we were going to hide the popover anyway.
   * Nevertheless I do this by setting visibility FALSE instead of
   * gtk_popover_popdown() because the animation stuff is very buggy
   * (sometimes the popup stays displayed even though it doesn't react,
   * blocking the GUI for a second or so).
   * Moreover I directly retransfer the event as though it was sent to
   * the item tree view. This allows much faster GUI interaction, not
   * having to click twice, once to close the popup, then a second for
   * the actual action (like eye switching or lock on another item or
   * item selection).
   */
  gtk_widget_set_visible (widget, FALSE);

  if (gtk_tree_view_get_path_at_pos (tree_view->view,
                                     bevent->x, bevent->y,
                                     &path, &column, NULL, NULL))
    {
      /* Clicking on the lock cell from the currently displayed lock box
       * should only toggle-hide the lock box.
       */
      if (gtk_tree_path_compare (path, view->priv->lock_box_path) != 0 ||
          column != gtk_tree_view_get_column (CLAY_CONTAINER_TREE_VIEW (view)->view, 1))
        {
          /* Propagate the press event to the tree view. */
          new_event = gdk_event_copy (event);
          g_object_unref (new_event->any.window);
          new_event->any.window     = g_object_ref (gtk_widget_get_window (GTK_WIDGET (tree_view->view)));
          new_event->any.send_event = TRUE;
          gtk_main_do_event (new_event);

          /* Also immediately pass a release event at same position.
           * Without this, we get weird pointer as though a quick drag'n
           * drop occurred.
           */
          new_event->type = GDK_BUTTON_RELEASE;
          gtk_main_do_event (new_event);

          gdk_event_free (new_event);
        }
      gtk_tree_path_free (path);
    }

  return GDK_EVENT_STOP;
}


/*  GtkTreeView callbacks  */

static void
clay_item_tree_view_row_expanded (GtkTreeView      *tree_view,
                                  GtkTreeIter      *iter,
                                  GtkTreePath      *path,
                                  ClayItemTreeView *item_view)
{
  ClayItemTreeViewClass *item_view_class;
  GList                 *selected_items;
  GList                 *list;

  item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (item_view);

  selected_items = clay_image_get_selected_items (item_view->priv->image,
                                                  item_view_class->item_type);

  if (selected_items)
    {
      ClayContainerTreeStore *store;
      ClayViewRenderer       *renderer;
      ClayItem               *expanded_item;

      store = CLAY_CONTAINER_TREE_STORE (CLAY_CONTAINER_TREE_VIEW (item_view)->model);
      renderer = clay_container_tree_store_get_renderer (store, iter);
      expanded_item = CLAY_ITEM (renderer->viewable);
      g_object_unref (renderer);

      for (list = selected_items; list; list = list->next)
        {
          /*  don't select an item while it is being inserted  */
          if (! _clay_container_view_lookup (CLAY_CONTAINER_VIEW (item_view),
                                             list->data))
            return;

          /*  select items only if they were made visible by expanding
           *  their immediate parent. See bug #666561.
           */
          if (clay_item_get_parent (list->data) != expanded_item)
            return;
        }

      item_view->priv->block_selection_changed++;

      clay_container_view_set_selected (CLAY_CONTAINER_VIEW (item_view),
                                        selected_items);

      item_view->priv->block_selection_changed--;
    }
}


/*  Helper functions.  */

static gint
clay_item_tree_view_get_n_locks (ClayItemTreeView *view,
                                 ClayItem         *item,
                                 const gchar     **icon_name)
{
  GList *list;
  gint   n_locks = 0;

  *icon_name = CLAY_ICON_LOCK_MULTI;

  for (list = view->priv->locks; list; list = list->next)
    {
      LockToggle *data = list->data;

      n_locks += (gint) data->is_locked (item);
    }

  if (n_locks == 1)
    {
      for (list = view->priv->locks; list; list = list->next)
        {
          LockToggle *data = list->data;

          if (data->is_locked (item))
            {
              *icon_name = data->icon_name;
              break;
            }
        }
    }

  return n_locks;
}

static void
clay_item_tree_view_floating_selection_changed (ClayImage        *image,
                                                ClayItemTreeView *view)
{
  gtk_widget_set_sensitive (view->priv->search_button,
                            image && ! clay_image_get_floating_selection (image));
}

static void
clay_item_tree_view_item_links_changed (ClayImage        *image,
                                        GType             item_type,
                                        ClayItemTreeView *view)
{
  gtk_widget_set_sensitive (view->priv->search_button, image != NULL);

  _clay_item_tree_view_search_update_links (view, image, item_type);
}

static gboolean
clay_item_tree_view_start_interactive_search (GtkTreeView      *tree_view,
                                              ClayItemTreeView *view)
{
  _clay_item_tree_view_search_show (view);

  return FALSE;
}

static gboolean
clay_item_tree_view_search_clicked (GtkWidget        *button,
                                    GdkEventButton   *event,
                                    ClayItemTreeView *view)
{
  _clay_item_tree_view_search_show (view);

  return TRUE;
}

static gboolean
clay_item_tree_view_move_cursor (ClayItemTreeView *view,
                                 guint             keyval,
                                 GdkModifierType   modifiers)
{
  ClayItemTreeViewClass *klass;
  const gchar           *action_name = NULL;

  g_return_val_if_fail (CLAY_IS_ITEM_TREE_VIEW (view), FALSE);

  klass = CLAY_ITEM_TREE_VIEW_GET_CLASS (view);
  switch (keyval)
    {
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      if (klass->move_cursor_down_flat_action)
        action_name = klass->move_cursor_down_flat_action;
      else
        action_name = klass->move_cursor_down_action;
      break;
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      if (klass->move_cursor_up_flat_action)
        action_name = klass->move_cursor_up_flat_action;
      else
        action_name = klass->move_cursor_up_action;
      break;
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
      action_name = klass->move_cursor_down_action;
      break;
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
      action_name = klass->move_cursor_up_action;
      break;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
      action_name = klass->move_cursor_end_action;
      break;
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
      action_name = klass->move_cursor_start_action;
      break;
    default:
      break;
    }

  if (action_name != NULL)
    {
      GAction *action;

      action = g_action_map_lookup_action (G_ACTION_MAP (g_application_get_default ()),
                                           action_name);
      g_return_val_if_fail (action != NULL, FALSE);

      clay_action_activate (CLAY_ACTION (action));
    }

  return TRUE;
}

static void
clay_item_tree_view_blink_item_column (ClayItemTreeView *view,
                                       ClayItem         *item,
                                       gint              column)
{
  GtkTreeIter  *iter;
  GtkTreePath  *path;
  GdkRectangle  rect;

  g_return_if_fail (CLAY_IS_ITEM_TREE_VIEW (view));
  g_return_if_fail (CLAY_IS_ITEM (item));

  /* Find the item in the tree view. */
  iter = _clay_container_view_lookup (CLAY_CONTAINER_VIEW (view),
                                      CLAY_VIEWABLE (item));
  path = gtk_tree_model_get_path (CLAY_CONTAINER_TREE_VIEW (view)->model, iter);

  /* Scroll dockable to make sure the cell is showing. */
  gtk_tree_view_scroll_to_cell (CLAY_CONTAINER_TREE_VIEW (view)->view, path,
                                gtk_tree_view_get_column (CLAY_CONTAINER_TREE_VIEW (view)->view, column),
                                FALSE, 0.0, 0.0);

  /* Now blink the specified column. */
  gtk_tree_view_get_cell_area (CLAY_CONTAINER_TREE_VIEW (view)->view, path,
                               gtk_tree_view_get_column (CLAY_CONTAINER_TREE_VIEW (view)->view, column),
                               &rect);
  gtk_tree_view_convert_bin_window_to_widget_coords (CLAY_CONTAINER_TREE_VIEW (view)->view,
                                                     rect.x, rect.y, &rect.x, &rect.y);
  clay_widget_blink_rect (GTK_WIDGET (CLAY_CONTAINER_TREE_VIEW (view)->view), &rect);

  gtk_tree_path_free (path);
}

/* This will list the allowed base GType-s for a tree view.
 * Call it with CLAY_TYPE_ITEM as argument and let it do the recursion
 * for you.
 */
static GList *
clay_item_tree_view_get_allowed_types (GType gtype)
{
  GList *types = NULL;
  GType *item_types;
  guint  n_item_types;

  item_types = g_type_children (gtype, &n_item_types);
  for (gint i = 0; i < n_item_types; i++)
    if (G_TYPE_IS_ABSTRACT (item_types[i]))
      types = g_list_concat (types,
                             clay_item_tree_view_get_allowed_types (item_types[i]));
    else
      types = g_list_prepend (types, CLAYTYPE_TO_POINTER (item_types[i]));

  g_free (item_types);

  return types;
}
