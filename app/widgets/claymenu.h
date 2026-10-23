/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenu.h
 * Copyright (C) 2023 Jehan
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


#define CLAY_TYPE_MENU            (clay_menu_get_type ())
#define CLAY_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MENU, ClayMenu))
#define CLAY_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MENU, ClayMenuClass))
#define CLAY_IS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_MENU))
#define CLAY_IS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MENU))
#define CLAY_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MENU, ClayMenuClass))


typedef struct _ClayMenuPrivate ClayMenuPrivate;
typedef struct _ClayMenuClass   ClayMenuClass;

struct _ClayMenu
{
  GtkMenu           parent_instance;

  ClayMenuPrivate  *priv;
};

struct _ClayMenuClass
{
  GtkMenuClass  parent_class;
};


GType        clay_menu_get_type   (void);

GtkWidget  * clay_menu_new        (ClayUIManager *manager);

void         clay_menu_merge      (ClayMenu      *menu,
                                   ClayMenu      *menu2,
                                   gboolean       top);
