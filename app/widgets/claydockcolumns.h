/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockcolumns.h
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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


#define CLAY_TYPE_DOCK_COLUMNS            (clay_dock_columns_get_type ())
#define CLAY_DOCK_COLUMNS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DOCK_COLUMNS, ClayDockColumns))
#define CLAY_DOCK_COLUMNS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DOCK_COLUMNS, ClayDockColumnsClass))
#define CLAY_IS_DOCK_COLUMNS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DOCK_COLUMNS))
#define CLAY_IS_DOCK_COLUMNS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DOCK_COLUMNS))
#define CLAY_DOCK_COLUMNS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DOCK_COLUMNS, ClayDockColumnsClass))


typedef struct _ClayDockColumnsClass    ClayDockColumnsClass;
typedef struct _ClayDockColumnsPrivate  ClayDockColumnsPrivate;

/**
 * ClayDockColumns:
 *
 * A widget containing ClayDocks so that dockables are arranged in
 * columns.
 */
struct _ClayDockColumns
{
  GtkBox parent_instance;

  ClayDockColumnsPrivate *p;
};

struct _ClayDockColumnsClass
{
  GtkBoxClass parent_class;

  void (* dock_added)   (ClayDockColumns *dock_columns,
                         ClayDock        *dock);
  void (* dock_removed) (ClayDockColumns *dock_columns,
                         ClayDock        *dock);
};


GType               clay_dock_columns_get_type           (void);
GtkWidget         * clay_dock_columns_new                (ClayContext       *context,
                                                          ClayDialogFactory *dialog_factory,
                                                          ClayUIManager     *ui_manager);
void                clay_dock_columns_add_dock           (ClayDockColumns   *dock_columns,
                                                          ClayDock          *dock,
                                                          gint               index);
void                clay_dock_columns_prepare_dockbook   (ClayDockColumns   *dock_columns,
                                                          gint               dock_index,
                                                          GtkWidget        **dockbook_p);
void                clay_dock_columns_remove_dock        (ClayDockColumns   *dock_columns,
                                                          ClayDock          *dock);
GList             * clay_dock_columns_get_docks          (ClayDockColumns   *dock_columns);
ClayContext       * clay_dock_columns_get_context        (ClayDockColumns   *dock_columns);
void                clay_dock_columns_set_context        (ClayDockColumns   *dock_columns,
                                                          ClayContext       *context);
ClayDialogFactory * clay_dock_columns_get_dialog_factory (ClayDockColumns   *dock_columns);
ClayUIManager     * clay_dock_columns_get_ui_manager     (ClayDockColumns   *dock_columns);
