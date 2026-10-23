/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenushell.h
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


#define CLAY_TYPE_MENU_SHELL               (clay_menu_shell_get_type ())
#define CLAY_MENU_SHELL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MENU_SHELL, ClayMenuShell))
#define CLAY_IS_MENU_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MENU_SHELL))
#define CLAY_MENU_SHELL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), CLAY_TYPE_MENU_SHELL, ClayMenuShellInterface))


enum
{
  CLAY_MENU_SHELL_PROP_0,
  CLAY_MENU_SHELL_PROP_MANAGER,
  CLAY_MENU_SHELL_PROP_MODEL,

  CLAY_MENU_SHELL_PROP_LAST = CLAY_MENU_SHELL_PROP_MODEL,
};

typedef struct _ClayMenuShellInterface ClayMenuShellInterface;

struct _ClayMenuShellInterface
{
  GTypeInterface base_interface;

  /* Signals */

  void          (* model_deleted) (ClayMenuShell  *shell);

  /* Virtual functions. */

  void          (* append)        (ClayMenuShell  *shell,
                                   ClayMenuModel  *model);
  void          (* add_ui)        (ClayMenuShell  *shell,
                                   const gchar   **paths,
                                   const gchar    *action_name,
                                   gboolean        top);
  void          (* remove_ui)     (ClayMenuShell  *shell,
                                   const gchar   **paths,
                                   const gchar    *action_name);
};


GType           clay_menu_shell_get_type            (void);

void            clay_menu_shell_fill                (ClayMenuShell *shell,
                                                     ClayMenuModel *model,
                                                     gboolean       drop_top_submenu);


/* Protected functions. */

void            clay_menu_shell_append              (ClayMenuShell *shell,
                                                     ClayMenuModel *model);

void            clay_menu_shell_init                (ClayMenuShell *shell);
void            clay_menu_shell_install_properties  (GObjectClass  *klass);
void            clay_menu_shell_get_property        (GObject       *object,
                                                     guint          property_id,
                                                     GValue        *value,
                                                     GParamSpec    *pspec);
void            clay_menu_shell_set_property        (GObject       *object,
                                                     guint          property_id,
                                                     const GValue  *value,
                                                     GParamSpec    *pspec);

ClayUIManager * clay_menu_shell_get_manager         (ClayMenuShell *shell);
