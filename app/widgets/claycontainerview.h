/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerview.h
 * Copyright (C) 2001-2025 Michael Natterer <mitch@gimp.org>
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


typedef enum
{
  CLAY_CONTAINER_VIEW_PROP_0,
  CLAY_CONTAINER_VIEW_PROP_CONTAINER,
  CLAY_CONTAINER_VIEW_PROP_CONTEXT,
  CLAY_CONTAINER_VIEW_PROP_SELECTION_MODE,
  CLAY_CONTAINER_VIEW_PROP_REORDERABLE,
  CLAY_CONTAINER_VIEW_PROP_VIEW_SIZE,
  CLAY_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH,
  CLAY_CONTAINER_VIEW_PROP_LAST = CLAY_CONTAINER_VIEW_PROP_VIEW_BORDER_WIDTH
} ClayContainerViewProp;


#define CLAY_TYPE_CONTAINER_VIEW (clay_container_view_get_type ())
G_DECLARE_INTERFACE (ClayContainerView,
                     clay_container_view,
                     CLAY, CONTAINER_VIEW,
                     GtkWidget)


struct _ClayContainerViewInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void     (* selection_changed)  (ClayContainerView *view);
  void     (* item_activated)     (ClayContainerView *view,
                                   ClayViewable      *object);

  /*  virtual functions  */
  void     (* set_container)      (ClayContainerView  *view,
                                   ClayContainer      *container);
  void     (* set_context)        (ClayContainerView  *view,
                                   ClayContext        *context);
  void     (* set_selection_mode) (ClayContainerView  *view,
                                   GtkSelectionMode    mode);
  void     (* set_view_size)      (ClayContainerView  *view);

  gboolean (* set_selected)       (ClayContainerView  *view,
                                   GList              *items);
  gint     (* get_selected)       (ClayContainerView  *view,
                                   GList             **items);

  /*  cruft  */
  gpointer (* insert_item)        (ClayContainerView *view,
                                   ClayViewable      *object,
                                   gpointer           parent_insert_data,
                                   gint               index);
  void     (* insert_items_after) (ClayContainerView *view);
  void     (* remove_item)        (ClayContainerView *view,
                                   ClayViewable      *object,
                                   gpointer           insert_data);
  void     (* reorder_item)       (ClayContainerView *view,
                                   ClayViewable      *object,
                                   gint               new_index,
                                   gpointer           insert_data);
  void     (* rename_item)        (ClayContainerView *view,
                                   ClayViewable      *object,
                                   gpointer           insert_data);
  void     (* expand_item)        (ClayContainerView *view,
                                   ClayViewable      *object,
                                   gpointer           insert_data);
  void     (* clear_items)        (ClayContainerView *view);

  /*  the destroy notifier for private->hash_table's values  */
  GDestroyNotify  insert_data_free;
  gboolean        model_is_tree;
  gboolean        use_list_model;
};


ClayContainer    * clay_container_view_get_container      (ClayContainerView  *view);
void               clay_container_view_set_container      (ClayContainerView  *view,
                                                           ClayContainer      *container);

ClayContext      * clay_container_view_get_context        (ClayContainerView  *view);
void               clay_container_view_set_context        (ClayContainerView  *view,
                                                           ClayContext        *context);

GtkSelectionMode   clay_container_view_get_selection_mode (ClayContainerView  *view);
void               clay_container_view_set_selection_mode (ClayContainerView  *view,
                                                           GtkSelectionMode    mode);

gint               clay_container_view_get_view_size      (ClayContainerView  *view,
                                                           gint               *view_border_width);
void               clay_container_view_set_view_size      (ClayContainerView  *view,
                                                           gint                view_size,
                                                           gint                view_border_width);

gboolean           clay_container_view_get_reorderable    (ClayContainerView  *view);
void               clay_container_view_set_reorderable    (ClayContainerView  *view,
                                                           gboolean            reorderable);

GtkWidget        * clay_container_view_get_dnd_widget     (ClayContainerView  *view);
void               clay_container_view_set_dnd_widget     (ClayContainerView  *view,
                                                           GtkWidget          *dnd_widget);

void               clay_container_view_enable_dnd         (ClayContainerView  *editor,
                                                           GtkButton          *button,
                                                           GType               child_type);

gboolean           clay_container_view_set_selected       (ClayContainerView  *view,
                                                           GList              *viewables);
gboolean           clay_container_view_set_1_selected     (ClayContainerView  *view,
                                                           ClayViewable       *viewable);

gint               clay_container_view_get_selected       (ClayContainerView  *view,
                                                           GList             **items);
ClayViewable     * clay_container_view_get_1_selected     (ClayContainerView  *view);

gboolean           clay_container_view_is_item_selected   (ClayContainerView  *view,
                                                           ClayViewable       *viewable);


/*  protected  */

void               _clay_container_view_selection_changed (ClayContainerView  *view);
void               _clay_container_view_item_activated    (ClayContainerView  *view,
                                                           ClayViewable       *item);

void               _clay_container_view_install_properties(GObjectClass       *klass);
void               _clay_container_view_set_property      (GObject            *object,
                                                           guint               property_id,
                                                           const GValue       *value,
                                                           GParamSpec         *pspec);
void               _clay_container_view_get_property      (GObject            *object,
                                                           guint               property_id,
                                                           GValue             *value,
                                                           GParamSpec         *pspec);
