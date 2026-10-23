/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontrollerlist.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CONTROLLER_LIST            (clay_controller_list_get_type ())
#define CLAY_CONTROLLER_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTROLLER_LIST, ClayControllerList))
#define CLAY_CONTROLLER_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTROLLER_LIST, ClayControllerListClass))
#define CLAY_IS_CONTROLLER_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTROLLER_LIST))
#define CLAY_IS_CONTROLLER_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTROLLER_LIST))
#define CLAY_CONTROLLER_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTROLLER_LIST, ClayControllerListClass))


typedef struct _ClayControllerListClass ClayControllerListClass;

struct _ClayControllerList
{
  GtkBox                 parent_instance;

  ClayControllerManager *controller_manager;

  GtkWidget             *hbox;

  GtkWidget             *available_controllers;
  GtkWidget             *active_controllers;

  GtkWidget             *add_button;
  GtkWidget             *remove_button;
  GtkWidget             *edit_button;
  GtkWidget             *up_button;
  GtkWidget             *down_button;
};

struct _ClayControllerListClass
{
  GtkBoxClass   parent_class;
};


GType       clay_controller_list_get_type (void);

GtkWidget * clay_controller_list_new      (ClayControllerManager *controller_manager);
