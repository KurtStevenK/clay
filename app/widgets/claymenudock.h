/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenudock.h
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

#include "claydock.h"


#define CLAY_TYPE_MENU_DOCK            (clay_menu_dock_get_type ())
#define CLAY_MENU_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MENU_DOCK, ClayMenuDock))
#define CLAY_MENU_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MENU_DOCK, ClayMenuDockClass))
#define CLAY_IS_MENU_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MENU_DOCK))
#define CLAY_IS_MENU_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MENU_DOCK))
#define CLAY_MENU_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MENU_DOCK, ClayMenuDockClass))

typedef struct _ClayMenuDockPrivate ClayMenuDockPrivate;
typedef struct _ClayMenuDockClass   ClayMenuDockClass;

struct _ClayMenuDock
{
  ClayDock             parent_instance;

  ClayMenuDockPrivate *p;
};

struct _ClayMenuDockClass
{
  ClayDockClass  parent_class;
};


GType       clay_menu_dock_get_type (void);

GtkWidget * clay_menu_dock_new      (void);
