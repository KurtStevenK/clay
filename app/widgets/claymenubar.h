/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenubar.h
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


#define CLAY_TYPE_MENU_BAR            (clay_menu_bar_get_type ())
#define CLAY_MENU_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MENU_BAR, ClayMenuBar))
#define CLAY_MENU_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MENU_BAR, ClayMenuBarClass))
#define CLAY_IS_MENU_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_MENU_BAR))
#define CLAY_IS_MENU_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MENU_BAR))
#define CLAY_MENU_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MENU_BAR, ClayMenuBarClass))


typedef struct _ClayMenuBarPrivate ClayMenuBarPrivate;
typedef struct _ClayMenuBarClass   ClayMenuBarClass;

struct _ClayMenuBar
{
  GtkMenuBar          parent_instance;

  ClayMenuBarPrivate *priv;
};

struct _ClayMenuBarClass
{
  GtkMenuBarClass  parent_class;
};


GType       clay_menu_bar_get_type (void);

GtkWidget * clay_menu_bar_new      (ClayMenuModel *model,
                                    ClayUIManager *manager);
