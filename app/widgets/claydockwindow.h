/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockwindow.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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

#include "widgets/claywindow.h"


#define CLAY_TYPE_DOCK_WINDOW            (clay_dock_window_get_type ())
#define CLAY_DOCK_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DOCK_WINDOW, ClayDockWindow))
#define CLAY_DOCK_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DOCK_WINDOW, ClayDockWindowClass))
#define CLAY_IS_DOCK_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DOCK_WINDOW))
#define CLAY_IS_DOCK_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DOCK_WINDOW))
#define CLAY_DOCK_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DOCK_WINDOW, ClayDockWindowClass))


typedef struct _ClayDockWindowClass    ClayDockWindowClass;
typedef struct _ClayDockWindowPrivate  ClayDockWindowPrivate;

/**
 * ClayDockWindow:
 *
 * A top-level window containing ClayDocks.
 */
struct _ClayDockWindow
{
  ClayWindow  parent_instance;

  ClayDockWindowPrivate *p;
};

struct _ClayDockWindowClass
{
  ClayWindowClass  parent_class;
};


GType               clay_dock_window_get_type               (void);
GtkWidget         * clay_dock_window_new                    (const gchar       *role,
                                                             const gchar       *ui_manager_name,
                                                             gboolean           allow_dockbook_absence,
                                                             ClayDialogFactory *factory,
                                                             ClayContext       *context);
gint                clay_dock_window_get_id                 (ClayDockWindow    *dock_window);
void                clay_dock_window_add_dock               (ClayDockWindow    *dock_window,
                                                             ClayDock          *dock,
                                                             gint               index);
void                clay_dock_window_remove_dock            (ClayDockWindow    *dock_window,
                                                             ClayDock          *dock);
ClayContext       * clay_dock_window_get_context            (ClayDockWindow    *dock);
gboolean            clay_dock_window_get_auto_follow_active (ClayDockWindow    *menu_dock);
void                clay_dock_window_set_auto_follow_active (ClayDockWindow    *menu_dock,
                                                             gboolean           show);
gboolean            clay_dock_window_get_show_image_menu    (ClayDockWindow    *menu_dock);
void                clay_dock_window_set_show_image_menu    (ClayDockWindow    *menu_dock,
                                                             gboolean           show);
void                clay_dock_window_setup                  (ClayDockWindow    *dock_window,
                                                             ClayDockWindow    *template);
gboolean            clay_dock_window_has_toolbox            (ClayDockWindow    *dock_window);

ClayDockWindow    * clay_dock_window_from_dock              (ClayDock          *dock);
