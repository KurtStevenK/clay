/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenufactory.h
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

#include "core/clayobject.h"


typedef struct _ClayMenuFactoryEntry ClayMenuFactoryEntry;

struct _ClayMenuFactoryEntry
{
  gchar        *identifier;
  GList        *action_groups;
  GList        *managed_uis;

  GHashTable   *managers;
};


#define CLAY_TYPE_MENU_FACTORY            (clay_menu_factory_get_type ())
#define CLAY_MENU_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MENU_FACTORY, ClayMenuFactory))
#define CLAY_MENU_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MENU_FACTORY, ClayMenuFactoryClass))
#define CLAY_IS_MENU_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MENU_FACTORY))
#define CLAY_IS_MENU_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MENU_FACTORY))
#define CLAY_MENU_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MENU_FACTORY, ClayMenuFactoryClass))


typedef struct _ClayMenuFactoryPrivate  ClayMenuFactoryPrivate;
typedef struct _ClayMenuFactoryClass    ClayMenuFactoryClass;

struct _ClayMenuFactory
{
  ClayObject              parent_instance;

  ClayMenuFactoryPrivate *p;
};

struct _ClayMenuFactoryClass
{
  ClayObjectClass  parent_class;
};


GType             clay_menu_factory_get_type             (void);
ClayMenuFactory * clay_menu_factory_new                  (Clay              *clay,
                                                          ClayActionFactory *action_factory);
void              clay_menu_factory_manager_register     (ClayMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          const gchar       *first_group,
                                                          ...)  G_GNUC_NULL_TERMINATED;
GList           * clay_menu_factory_get_registered_menus (ClayMenuFactory   *factory);
ClayUIManager   * clay_menu_factory_get_manager          (ClayMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          gpointer           callback_data);
void              clay_menu_factory_delete_manager       (ClayMenuFactory   *factory,
                                                          const gchar       *identifier,
                                                          gpointer           callback_data);
