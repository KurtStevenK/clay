/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockbook.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_DOCKBOOK            (clay_dockbook_get_type ())
#define CLAY_DOCKBOOK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DOCKBOOK, ClayDockbook))
#define CLAY_DOCKBOOK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DOCKBOOK, ClayDockbookClass))
#define CLAY_IS_DOCKBOOK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DOCKBOOK))
#define CLAY_IS_DOCKBOOK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DOCKBOOK))
#define CLAY_DOCKBOOK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DOCKBOOK, ClayDockbookClass))


typedef void (* ClayDockbookDragCallback) (GdkDragContext *context,
                                           gboolean        begin,
                                           gpointer        data);


typedef struct _ClayDockbookClass    ClayDockbookClass;
typedef struct _ClayDockbookPrivate  ClayDockbookPrivate;

/**
 * ClayDockbook:
 *
 * Holds ClayDockables which are presented on different tabs using
 * GtkNotebook.
 */
struct _ClayDockbook
{
  GtkNotebook parent_instance;

  ClayDockbookPrivate *p;
};

struct _ClayDockbookClass
{
  GtkNotebookClass parent_class;

  void (* dockable_added)     (ClayDockbook *dockbook,
                               ClayDockable *dockable);
  void (* dockable_removed)   (ClayDockbook *dockbook,
                               ClayDockable *dockable);
  void (* dockable_reordered) (ClayDockbook *dockbook,
                               ClayDockable *dockable);
};


GType           clay_dockbook_get_type                (void);
GtkWidget     * clay_dockbook_new                     (ClayMenuFactory          *menu_factory);

void            clay_dockbook_set_dock                (ClayDockbook             *dockbook,
                                                       ClayDock                 *dock);
ClayDock      * clay_dockbook_get_dock                (ClayDockbook             *dockbook);

GtkWidget     * clay_dockbook_add_from_dialog_factory (ClayDockbook             *dockbook,
                                                       const gchar              *identifiers);

void            clay_dockbook_update_with_context     (ClayDockbook             *dockbook,
                                                       ClayContext              *context);
GtkWidget    *  clay_dockbook_create_tab_widget       (ClayDockbook             *dockbook,
                                                       ClayDockable             *dockable);
void            clay_dockbook_set_drag_handler        (ClayDockbook             *dockbook,
                                                       ClayPanedBox             *drag_handler);

void            clay_dockbook_add_drag_callback       (ClayDockbookDragCallback  callback,
                                                       gpointer                  data);
void            clay_dockbook_remove_drag_callback    (ClayDockbookDragCallback  callback,
                                                       gpointer                  data);
