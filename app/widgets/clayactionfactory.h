/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayactionfactory.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


typedef struct _ClayActionFactoryEntry ClayActionFactoryEntry;

struct _ClayActionFactoryEntry
{
  gchar                     *identifier;
  gchar                     *label;
  gchar                     *icon_name;
  ClayActionGroupSetupFunc   setup_func;
  ClayActionGroupUpdateFunc  update_func;

  GHashTable                *groups;
};


#define CLAY_TYPE_ACTION_FACTORY            (clay_action_factory_get_type ())
#define CLAY_ACTION_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ACTION_FACTORY, ClayActionFactory))
#define CLAY_ACTION_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ACTION_FACTORY, ClayActionFactoryClass))
#define CLAY_IS_ACTION_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ACTION_FACTORY))
#define CLAY_IS_ACTION_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ACTION_FACTORY))
#define CLAY_ACTION_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ACTION_FACTORY, ClayActionFactoryClass))


typedef struct _ClayActionFactoryClass  ClayActionFactoryClass;

struct _ClayActionFactory
{
  ClayObject  parent_instance;

  Clay       *clay;
  GList      *registered_groups;
};

struct _ClayActionFactoryClass
{
  ClayObjectClass  parent_class;
};


GType               clay_action_factory_get_type       (void);

ClayActionFactory * clay_action_factory_new            (Clay                      *clay);

void                clay_action_factory_group_register (ClayActionFactory         *factory,
                                                        const gchar               *identifier,
                                                        const gchar               *label,
                                                        const gchar               *icon_name,
                                                        ClayActionGroupSetupFunc   setup_func,
                                                        ClayActionGroupUpdateFunc  update_func);

ClayActionGroup *   clay_action_factory_get_group      (ClayActionFactory         *factory,
                                                        const gchar               *identifier,
                                                        gpointer                   user_data);
void                clay_action_factory_delete_group   (ClayActionFactory         *factory,
                                                        const gchar               *identifier,
                                                        gpointer                   user_data);
