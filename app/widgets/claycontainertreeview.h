/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainertreeview.h
 * Copyright (C) 2003-2004 Michael Natterer <mitch@gimp.org>
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

#include "claycontainerbox.h"


#define CLAY_TYPE_CONTAINER_TREE_VIEW            (clay_container_tree_view_get_type ())
#define CLAY_CONTAINER_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_TREE_VIEW, ClayContainerTreeView))
#define CLAY_CONTAINER_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_TREE_VIEW, ClayContainerTreeViewClass))
#define CLAY_IS_CONTAINER_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_TREE_VIEW))
#define CLAY_IS_CONTAINER_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_TREE_VIEW))
#define CLAY_CONTAINER_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_TREE_VIEW, ClayContainerTreeViewClass))


typedef struct _ClayContainerTreeViewClass   ClayContainerTreeViewClass;
typedef struct _ClayContainerTreeViewPrivate ClayContainerTreeViewPrivate;

struct _ClayContainerTreeView
{
  ClayContainerBox              parent_instance;

  GtkTreeModel                 *model;
  gint                          n_model_columns;
  GType                         model_columns[16];

  GtkTreeView                  *view;

  GtkTreeViewColumn            *main_column;
  GtkCellRenderer              *renderer_cell;

  Clay                         *dnd_clay; /* eek */

  ClayContainerTreeViewPrivate *priv;
};

struct _ClayContainerTreeViewClass
{
  ClayContainerBoxClass  parent_class;

  /* signals */

  void     (* edit_name)      (ClayContainerTreeView   *tree_view);

  /* virtual functions */

  gboolean (* drop_possible)  (ClayContainerTreeView   *tree_view,
                               ClayDndType              src_type,
                               GList                   *src_viewables,
                               ClayViewable            *dest_viewable,
                               GtkTreePath             *drop_path,
                               GtkTreeViewDropPosition  drop_pos,
                               GtkTreeViewDropPosition *return_drop_pos,
                               GdkDragAction           *return_drag_action);
  void     (* drop_viewables) (ClayContainerTreeView   *tree_view,
                               GList                   *src_viewables,
                               ClayViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_color)     (ClayContainerTreeView   *tree_view,
                               GeglColor               *src_color,
                               ClayViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_uri_list)  (ClayContainerTreeView   *tree_view,
                               GList                   *uri_list,
                               ClayViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_svg)       (ClayContainerTreeView   *tree_view,
                               const gchar             *svg_data,
                               gsize                    svg_data_length,
                               ClayViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_component) (ClayContainerTreeView   *tree_view,
                               ClayImage               *image,
                               ClayChannelType          component,
                               ClayViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
  void     (* drop_pixbuf)    (ClayContainerTreeView   *tree_view,
                               GdkPixbuf               *pixbuf,
                               ClayViewable            *dest_viewable,
                               GtkTreeViewDropPosition  drop_pos);
};


GType       clay_container_tree_view_get_type (void);

GtkWidget * clay_container_tree_view_new      (ClayContainer *container,
                                               ClayContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);

GtkCellRenderer *
            clay_container_tree_view_get_name_cell
                                              (ClayContainerTreeView *tree_view);

void        clay_container_tree_view_set_main_column_title
                                              (ClayContainerTreeView *tree_view,
                                               const gchar           *title);

gint        clay_container_tree_view_get_main_column_position
                                              (ClayContainerTreeView *tree_view);

void        clay_container_tree_view_add_toggle_cell
                                              (ClayContainerTreeView *tree_view,
                                               GtkCellRenderer       *cell);

void        clay_container_tree_view_add_renderer_cell
                                              (ClayContainerTreeView *tree_view,
                                               GtkCellRenderer       *cell,
                                               gint                   column_number);

void        clay_container_tree_view_set_dnd_drop_to_empty
                                              (ClayContainerTreeView *tree_view,
                                               gboolean               dnd_drop_to_emtpy);
void        clay_container_tree_view_connect_name_edited
                                              (ClayContainerTreeView *tree_view,
                                               GCallback              callback,
                                               gpointer               data);
gboolean    clay_container_tree_view_name_edited
                                              (GtkCellRendererText   *cell,
                                               const gchar           *path_str,
                                               const gchar           *new_name,
                                               ClayContainerTreeView *tree_view);
