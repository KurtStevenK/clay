/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainertreestore.h
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


enum
{
  CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER,
  CLAY_CONTAINER_TREE_STORE_COLUMN_NAME,
  CLAY_CONTAINER_TREE_STORE_COLUMN_NAME_ATTRIBUTES,
  CLAY_CONTAINER_TREE_STORE_COLUMN_NAME_SENSITIVE,
  CLAY_CONTAINER_TREE_STORE_COLUMN_USER_DATA,
  CLAY_CONTAINER_TREE_STORE_N_COLUMNS
};


#define CLAY_TYPE_CONTAINER_TREE_STORE            (clay_container_tree_store_get_type ())
#define CLAY_CONTAINER_TREE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_TREE_STORE, ClayContainerTreeStore))
#define CLAY_CONTAINER_TREE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_TREE_STORE, ClayContainerTreeStoreClass))
#define CLAY_IS_CONTAINER_TREE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_TREE_STORE))
#define CLAY_IS_CONTAINER_TREE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_TREE_STORE))
#define CLAY_CONTAINER_TREE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_TREE_STORE, ClayContainerTreeStoreClass))


typedef struct _ClayContainerTreeStoreClass ClayContainerTreeStoreClass;

struct _ClayContainerTreeStore
{
  GtkTreeStore  parent_instance;
};

struct _ClayContainerTreeStoreClass
{
  GtkTreeStoreClass  parent_class;
};


GType          clay_container_tree_store_get_type      (void);

void           clay_container_tree_store_columns_init  (GType                  *types,
                                                        gint                   *n_types);
gint           clay_container_tree_store_columns_add   (GType                  *types,
                                                        gint                   *n_types,
                                                        GType                   type);

GtkTreeModel * clay_container_tree_store_new           (ClayContainerView      *container_view,
                                                        gint                    n_columns,
                                                        GType                  *types);

void       clay_container_tree_store_add_renderer_cell (ClayContainerTreeStore *store,
                                                        GtkCellRenderer        *cell,
                                                        gint                    column_number);
ClayViewRenderer *
               clay_container_tree_store_get_renderer  (ClayContainerTreeStore *store,
                                                        GtkTreeIter            *iter);

void           clay_container_tree_store_set_use_name  (ClayContainerTreeStore *store,
                                                        gboolean                use_name);
gboolean       clay_container_tree_store_get_use_name  (ClayContainerTreeStore *store);

void           clay_container_tree_store_set_context   (ClayContainerTreeStore *store,
                                                        ClayContext            *context);
GtkTreeIter *  clay_container_tree_store_insert_item   (ClayContainerTreeStore *store,
                                                        ClayViewable           *viewable,
                                                        GtkTreeIter            *parent,
                                                        gint                    index);
void           clay_container_tree_store_remove_item   (ClayContainerTreeStore *store,
                                                        ClayViewable           *viewable,
                                                        GtkTreeIter            *iter);
void           clay_container_tree_store_reorder_item  (ClayContainerTreeStore *store,
                                                        ClayViewable           *viewable,
                                                        gint                    new_index,
                                                        GtkTreeIter            *iter);
gboolean       clay_container_tree_store_rename_item   (ClayContainerTreeStore *store,
                                                        ClayViewable           *viewable,
                                                        GtkTreeIter            *iter);
void           clay_container_tree_store_clear_items   (ClayContainerTreeStore *store);
void           clay_container_tree_store_set_view_size (ClayContainerTreeStore *store);
