/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayitemtree.h
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

#pragma once

#include "clayobject.h"


#define CLAY_TYPE_ITEM_TREE            (clay_item_tree_get_type ())
#define CLAY_ITEM_TREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ITEM_TREE, ClayItemTree))
#define CLAY_ITEM_TREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ITEM_TREE, ClayItemTreeClass))
#define CLAY_IS_ITEM_TREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ITEM_TREE))
#define CLAY_IS_ITEM_TREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ITEM_TREE))


typedef struct _ClayItemTreeClass ClayItemTreeClass;

struct _ClayItemTree
{
  ClayObject     parent_instance;

  ClayContainer *container;
};

struct _ClayItemTreeClass
{
  ClayObjectClass  parent_class;
};


GType          clay_item_tree_get_type         (void);
ClayItemTree * clay_item_tree_new              (ClayImage     *image,
                                                GType          container_type,
                                                GType          item_type);

ClayItem     * clay_item_tree_get_active_item  (ClayItemTree  *tree);
void           clay_item_tree_set_active_item  (ClayItemTree  *tree,
                                                ClayItem      *item);

GList        * clay_item_tree_get_selected_items (ClayItemTree  *tree);
void           clay_item_tree_set_selected_items (ClayItemTree  *tree,
                                                  GList         *items);

ClayItem     * clay_item_tree_get_item_by_name (ClayItemTree  *tree,
                                                const gchar   *name);

gboolean       clay_item_tree_get_insert_pos   (ClayItemTree  *tree,
                                                ClayItem      *item,
                                                ClayItem     **parent,
                                                gint          *position);

void           clay_item_tree_add_item         (ClayItemTree  *tree,
                                                ClayItem      *item,
                                                ClayItem      *parent,
                                                gint           position);
GList        * clay_item_tree_remove_item      (ClayItemTree  *tree,
                                                ClayItem      *item,
                                                GList         *new_selected);

gboolean       clay_item_tree_reorder_item     (ClayItemTree  *tree,
                                                ClayItem      *item,
                                                ClayItem      *new_parent,
                                                gint           new_index,
                                                gboolean       push_undo,
                                                const gchar   *undo_desc);

void           clay_item_tree_rename_item      (ClayItemTree  *tree,
                                                ClayItem      *item,
                                                const gchar   *new_name,
                                                gboolean       push_undo,
                                                const gchar   *undo_desc);
