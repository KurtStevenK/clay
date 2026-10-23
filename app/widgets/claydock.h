/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydock.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_DOCK            (clay_dock_get_type ())
#define CLAY_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DOCK, ClayDock))
#define CLAY_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DOCK, ClayDockClass))
#define CLAY_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DOCK))
#define CLAY_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DOCK))
#define CLAY_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DOCK, ClayDockClass))


/* String used to separate dockables, e.g. "Tool Options, Layers" */
#define CLAY_DOCK_DOCKABLE_SEPARATOR C_("dock", ", ")

/* String used to separate books (GtkNotebooks) within a dock,
   e.g. "Tool Options, Layers - Brushes"
 */
#define CLAY_DOCK_BOOK_SEPARATOR C_("dock", " - ")

/* String used to separate dock columns,
   e.g. "Tool Options, Layers - Brushes | Gradients"
 */
#define CLAY_DOCK_COLUMN_SEPARATOR C_("dock", " | ")


typedef struct _ClayDockClass    ClayDockClass;
typedef struct _ClayDockPrivate  ClayDockPrivate;

/**
 * ClayDock:
 *
 * Contains a column of ClayDockbooks.
 */
struct _ClayDock
{
  GtkBox           parent_instance;

  ClayDockPrivate *p;
};

struct _ClayDockClass
{
  GtkBoxClass  parent_class;

  /*  virtual functions  */
  gchar * (* get_description)         (ClayDock       *dock,
                                       gboolean        complete);
  void    (* set_host_geometry_hints) (ClayDock       *dock,
                                       GtkWindow      *window);

  /*  signals  */
  void    (* book_added)              (ClayDock       *dock,
                                       ClayDockbook   *dockbook);
  void    (* book_removed)            (ClayDock       *dock,
                                       ClayDockbook   *dockbook);
  void    (* description_invalidated) (ClayDock       *dock);
  void    (* geometry_invalidated)    (ClayDock       *dock);
};


GType               clay_dock_get_type                (void);

gchar             * clay_dock_get_description         (ClayDock       *dock,
                                                       gboolean        complete);
void                clay_dock_set_host_geometry_hints (ClayDock       *dock,
                                                       GtkWindow      *window);
void                clay_dock_invalidate_geometry     (ClayDock       *dock);
void                clay_dock_update_with_context     (ClayDock       *dock,
                                                       ClayContext    *context);
ClayContext       * clay_dock_get_context             (ClayDock       *dock);
ClayDialogFactory * clay_dock_get_dialog_factory      (ClayDock       *dock);
ClayUIManager     * clay_dock_get_ui_manager          (ClayDock       *dock);
GList             * clay_dock_get_dockbooks           (ClayDock       *dock);
gint                clay_dock_get_n_dockables         (ClayDock       *dock);
GtkWidget         * clay_dock_get_main_vbox           (ClayDock       *dock);
GtkWidget         * clay_dock_get_vbox                (ClayDock       *dock);

void                clay_dock_set_id                  (ClayDock       *dock,
                                                       gint            ID);
gint                clay_dock_get_id                  (ClayDock       *dock);

void                clay_dock_add_book                (ClayDock       *dock,
                                                       ClayDockbook   *dockbook,
                                                       gint            index);
void                clay_dock_remove_book             (ClayDock       *dock,
                                                       ClayDockbook   *dockbook);
