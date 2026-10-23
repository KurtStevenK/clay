/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymodifiersmanager.h
 * Copyright (C) 2022 Jehan
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


#define CLAY_TYPE_MODIFIERS_MANAGER            (clay_modifiers_manager_get_type ())
#define CLAY_MODIFIERS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MODIFIERS_MANAGER, ClayModifiersManager))
#define CLAY_MODIFIERS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MODIFIERS_MANAGER, ClayModifiersManagerClass))
#define CLAY_IS_MODIFIERS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MODIFIERS_MANAGER))
#define CLAY_IS_MODIFIERS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MODIFIERS_MANAGER))
#define CLAY_MODIFIERS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MODIFIERS_MANAGER, ClayModifiersManagerClass))


typedef struct _ClayModifiersManagerPrivate  ClayModifiersManagerPrivate;
typedef struct _ClayModifiersManagerClass    ClayModifiersManagerClass;

/**
 * ClayModifiersManager:
 *
 * Contains modifiers configuration for canvas interaction.
 */
struct _ClayModifiersManager
{
  GObject                      parent_instance;

  ClayModifiersManagerPrivate *p;
};

struct _ClayModifiersManagerClass
{
  GObjectClass                 parent_class;
};


GType                  clay_modifiers_manager_get_type      (void);

ClayModifiersManager * clay_modifiers_manager_new           (void);

ClayModifierAction     clay_modifiers_manager_get_action    (ClayModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers,
                                                             const gchar         **action_desc);

/* Protected functions: only use them from ClayModifiersEditor */

GList                * clay_modifiers_manager_get_modifiers (ClayModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button);

void                   clay_modifiers_manager_set           (ClayModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers,
                                                             ClayModifierAction    action,
                                                             const gchar          *action_desc);
void                   clay_modifiers_manager_remove        (ClayModifiersManager *manager,
                                                             GdkDevice            *device,
                                                             guint                 button,
                                                             GdkModifierType       modifiers);
void                   clay_modifiers_manager_clear         (ClayModifiersManager *manager);
