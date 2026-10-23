/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitemtreeview.h
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

#pragma once

#include "claycontainertreeview.h"


typedef ClayItem * (* ClayNewItemFunc)  (ClayImage *image);

typedef gboolean   (* ClayIsLockedFunc) (ClayItem  *item);
typedef gboolean   (* ClayCanLockFunc)  (ClayItem  *item);
typedef void       (* ClaySetLockFunc)  (ClayItem  *item,
                                         gboolean   lock,
                                         gboolean   push_undo);
typedef ClayUndo * (*ClayUndoLockPush)  (ClayImage     *image,
                                         const gchar   *undo_desc,
                                         ClayItem      *item);


#define CLAY_TYPE_ITEM_TREE_VIEW            (clay_item_tree_view_get_type ())
#define CLAY_ITEM_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ITEM_TREE_VIEW, ClayItemTreeView))
#define CLAY_ITEM_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ITEM_TREE_VIEW, ClayItemTreeViewClass))
#define CLAY_IS_ITEM_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ITEM_TREE_VIEW))
#define CLAY_IS_ITEM_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ITEM_TREE_VIEW))
#define CLAY_ITEM_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ITEM_TREE_VIEW, ClayItemTreeViewClass))


typedef struct _ClayItemTreeViewClass   ClayItemTreeViewClass;
typedef struct _ClayItemTreeViewPrivate ClayItemTreeViewPrivate;
typedef struct _ClayItemTreeViewSearch  ClayItemTreeViewSearch;

struct _ClayItemTreeView
{
  ClayContainerTreeView    parent_instance;

  ClayItemTreeViewPrivate *priv;
  ClayItemTreeViewSearch  *search;
};

struct _ClayItemTreeViewClass
{
  ClayContainerTreeViewClass  parent_class;

  /*  signals  */
  void (* set_image) (ClayItemTreeView *view,
                      ClayImage        *image);

  GType                 item_type;
  const gchar          *signal_name;

  /*  virtual functions for manipulating the image's item tree  */
  ClayNewItemFunc       new_item;

  /*  action names  */
  const gchar          *action_group;
  const gchar          *activate_action;
  const gchar          *new_action;
  const gchar          *new_default_action;
  const gchar          *raise_action;
  const gchar          *raise_top_action;
  const gchar          *lower_action;
  const gchar          *lower_bottom_action;
  const gchar          *duplicate_action;
  const gchar          *delete_action;

  const gchar          *move_cursor_up_action;
  const gchar          *move_cursor_down_action;
  const gchar          *move_cursor_up_flat_action;
  const gchar          *move_cursor_down_flat_action;
  const gchar          *move_cursor_start_action;
  const gchar          *move_cursor_end_action;

  /*  lock content button appearance  */
  const gchar          *lock_content_icon_name;
  const gchar          *lock_content_tooltip;
  const gchar          *lock_content_help_id;

  /* lock position (translation and transformation) button appearance */
  const gchar          *lock_position_icon_name;
  const gchar          *lock_position_tooltip;
  const gchar          *lock_position_help_id;

  /* lock visibility button appearance */
  const gchar          *lock_visibility_icon_name;
  const gchar          *lock_visibility_tooltip;
  const gchar          *lock_visibility_help_id;
};


GType       clay_item_tree_view_get_type          (void);

GtkWidget * clay_item_tree_view_new               (GType             view_type,
                                                   gint              view_size,
                                                   gint              view_border_width,
                                                   gboolean          multiple_selection,
                                                   ClayImage        *image,
                                                   ClayMenuFactory  *menu_facotry,
                                                   const gchar      *menu_identifier,
                                                   const gchar      *ui_identifier);

void        clay_item_tree_view_set_image         (ClayItemTreeView *view,
                                                   ClayImage        *image);
ClayImage * clay_item_tree_view_get_image         (ClayItemTreeView *view);

void        clay_item_tree_view_add_options       (ClayItemTreeView *view,
                                                   const gchar      *label,
                                                   GtkWidget        *options);
void        clay_item_tree_view_add_lock          (ClayItemTreeView *view,
                                                   const gchar      *icon_name,
                                                   ClayIsLockedFunc  is_locked,
                                                   ClayCanLockFunc   can_lock,
                                                   ClaySetLockFunc   lock,
                                                   ClayUndoLockPush  undo_push,
                                                   const gchar      *signal_name,
                                                   ClayUndoType      undo_type,
                                                   ClayUndoType      group_undo_type,
                                                   const gchar      *undo_lock_label,
                                                   const gchar      *undo_unlock_label,
                                                   const gchar      *undo_exclusive_desc,
                                                   const gchar      *tooltip,
                                                   const gchar      *help_id);
void        clay_item_tree_view_blink_lock        (ClayItemTreeView *view,
                                                   ClayItem         *item);
void        clay_item_tree_view_blink_item        (ClayItemTreeView *view,
                                                   ClayItem         *item);

GtkWidget * clay_item_tree_view_get_new_button    (ClayItemTreeView *view);
GtkWidget * clay_item_tree_view_get_delete_button (ClayItemTreeView *view);

gint        clay_item_tree_view_get_drop_index    (ClayItemTreeView *view,
                                                   ClayViewable     *dest_viewable,
                                                   GtkTreeViewDropPosition drop_pos,
                                                   ClayViewable    **parent);
