/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayuimanager.h
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


typedef void (* ClayUIMenuCallback)  (ClayUIManager *manager,
                                      const gchar   *path,
                                      const gchar   *action_name,
                                      gboolean       top,
                                      gpointer       user_data);


typedef struct _ClayUIManagerUIEntry ClayUIManagerUIEntry;

struct _ClayUIManagerUIEntry
{
  gchar                  *ui_path;
  gchar                  *basename;
  ClayUIManagerSetupFunc  setup_func;
  gboolean                setup_done;
  GtkBuilder             *builder;
};


#define CLAY_TYPE_UI_MANAGER              (clay_ui_manager_get_type ())
#define CLAY_UI_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_UI_MANAGER, ClayUIManager))
#define CLAY_UI_MANAGER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), CLAY_TYPE_UI_MANAGER, ClayUIManagerClass))
#define CLAY_IS_UI_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_UI_MANAGER))
#define CLAY_IS_UI_MANAGER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), CLAY_TYPE_UI_MANAGER))
#define CLAY_UI_MANAGER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), CLAY_TYPE_UI_MANAGER, ClayUIManagerClass))


typedef struct _ClayUIManagerClass ClayUIManagerClass;

/**
 * Among other things, is responsible for updating menu bars. A more
 * appropriate name would perhaps be ClayMenubarManager.
 */
struct _ClayUIManager
{
  ClayObject    parent_instance;

  gchar        *name;
  Clay         *clay;
  GList        *registered_uis;

  GList        *ui_items;
  GList        *action_groups;

  /* Special property which should be set to TRUE only for the singleton UI
   * manager of the top menu bar.
   */
  gboolean      store_action_paths;
};

struct _ClayUIManagerClass
{
  ClayObjectClass    parent_class;

  GHashTable        *managers;

  void (* update)       (ClayUIManager *manager,
                         gpointer       update_data);
  void (* show_tooltip) (ClayUIManager *manager,
                         const gchar   *tooltip);
  void (* hide_tooltip) (ClayUIManager *manager);
  void (* ui_added)     (ClayUIManager *manager,
                         const gchar   *path,
                         const gchar   *action_name,
                         gboolean       top);
  void (* ui_removed)   (ClayUIManager *manager,
                         const gchar   *path,
                         const gchar   *action_name);
};


GType           clay_ui_manager_get_type    (void);

ClayUIManager * clay_ui_manager_new         (Clay                   *clay,
                                             const gchar            *name);

GList         * clay_ui_managers_from_name  (const gchar            *name);

void            clay_ui_manager_update      (ClayUIManager          *manager,
                                             gpointer                update_data);

void            clay_ui_manager_add_action_group   (ClayUIManager   *manager,
                                                    ClayActionGroup *group);
ClayActionGroup * clay_ui_manager_get_action_group (ClayUIManager   *manager,
                                                    const gchar     *name);
GList         * clay_ui_manager_get_action_groups  (ClayUIManager   *manager);

ClayMenuModel * clay_ui_manager_get_model       (ClayUIManager      *manager,
                                                 const gchar        *path);

void            clay_ui_manager_remove_ui       (ClayUIManager      *manager,
                                                 const gchar        *action_name);
void            clay_ui_manager_remove_uis      (ClayUIManager      *manager,
                                                 const gchar        *action_name_prefix);
void            clay_ui_manager_add_ui          (ClayUIManager      *manager,
                                                 const gchar        *path,
                                                 const gchar        *action_name,
                                                 gboolean            top);
void            clay_ui_manager_foreach_ui      (ClayUIManager      *manager,
                                                 ClayUIMenuCallback  callback,
                                                 gpointer            user_data);

ClayAction    * clay_ui_manager_find_action     (ClayUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name);
gboolean        clay_ui_manager_activate_action (ClayUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name);
gboolean        clay_ui_manager_toggle_action   (ClayUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name,
                                                 gboolean            active);

void            clay_ui_manager_ui_register (ClayUIManager          *manager,
                                             const gchar            *ui_path,
                                             const gchar            *basename,
                                             ClayUIManagerSetupFunc  setup_func);

void            clay_ui_manager_ui_popup_at_widget
                                            (ClayUIManager          *manager,
                                             const gchar            *ui_path,
                                             ClayUIManager          *child_ui_manager,
                                             const gchar            *child_ui_path,
                                             GtkWidget              *widget,
                                             GdkGravity              widget_anchor,
                                             GdkGravity              menu_anchor,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            clay_ui_manager_ui_popup_at_pointer
                                            (ClayUIManager          *manager,
                                             const gchar            *ui_path,
                                             GtkWidget              *attached_widget,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            clay_ui_manager_ui_popup_at_rect
                                            (ClayUIManager           *manager,
                                             const gchar             *ui_path,
                                             GtkWidget               *attached_widget,
                                             GdkWindow               *window,
                                             const GdkRectangle      *rect,
                                             GdkGravity               rect_anchor,
                                             GdkGravity               menu_anchor,
                                             const GdkEvent          *trigger_event,
                                             GDestroyNotify           popdown_func,
                                             gpointer                 popdown_data);
