/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayuimanager.c
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayxmlparser.h"

#include "core/clay.h"

#include "clayaction.h"
#include "clayactiongroup.h"
#include "clayradioaction.h"
#include "clayhelp.h"
#include "clayhelp-ids.h"
#include "claymenu.h"
#include "claymenumodel.h"
#include "claymenushell.h"
#include "claytoggleaction.h"
#include "clayuimanager.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_NAME,
  PROP_CLAY
};

enum
{
  UPDATE,
  SHOW_TOOLTIP,
  HIDE_TOOLTIP,
  UI_ADDED,
  UI_REMOVED,
  LAST_SIGNAL
};

typedef struct
{
  gchar    *path;
  gchar    *action_name;
  gboolean  top;
} ClayUIManagerMenuItem;


static void       clay_ui_manager_constructed          (GObject        *object);
static void       clay_ui_manager_dispose              (GObject        *object);
static void       clay_ui_manager_finalize             (GObject        *object);
static void       clay_ui_manager_set_property         (GObject        *object,
                                                        guint           prop_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void       clay_ui_manager_get_property         (GObject        *object,
                                                        guint           prop_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void       clay_ui_manager_real_update          (ClayUIManager  *manager,
                                                        gpointer        update_data);
static ClayUIManagerUIEntry *
                  clay_ui_manager_entry_get            (ClayUIManager  *manager,
                                                        const gchar    *ui_path);
static ClayUIManagerUIEntry *
                  clay_ui_manager_entry_ensure         (ClayUIManager  *manager,
                                                        const gchar    *path);

static void       clay_ui_manager_store_builder_path   (GMenuModel           *model,
                                                        GMenuModel           *original);
static GtkBuilder * clay_ui_manager_load_builder       (const gchar          *filename,
                                                        const gchar          *root);
static void       clay_ui_manager_parse_start_element  (GMarkupParseContext  *context,
                                                        const gchar          *element_name,
                                                        const gchar         **attribute_names,
                                                        const gchar         **attribute_values,
                                                        gpointer              user_data,
                                                        GError              **error);
static void       clay_ui_manager_parse_end_element    (GMarkupParseContext  *context,
                                                        const gchar          *element_name,
                                                        gpointer              user_data,
                                                        GError              **error);
static void       clay_ui_manager_parse_ui_text        (GMarkupParseContext  *context,
                                                        const gchar          *text,
                                                        gsize                 text_len,
                                                        gpointer              user_data,
                                                        GError              **error);

static void       clay_ui_manager_delete_popdown_data  (GtkWidget      *widget,
                                                        ClayUIManager  *manager);

static void       clay_ui_manager_menu_item_free       (ClayUIManagerMenuItem *item);

static void       clay_ui_manager_popup_hidden         (GtkMenuShell          *popup,
                                                        gpointer               user_data);
static gboolean   clay_ui_manager_popup_destroy        (GtkWidget             *popup);

static void       clay_ui_manager_image_action_added   (ClayActionGroup       *group,
                                                        gchar                 *action_name,
                                                        ClayUIManager         *manager);
static void       clay_ui_manager_image_action_removed (ClayActionGroup       *group,
                                                        gchar                 *action_name,
                                                        ClayUIManager         *manager);
static void       clay_ui_manager_image_accels_changed (ClayAction            *action,
                                                        const gchar          **accels,
                                                        ClayUIManager         *manager);


G_DEFINE_TYPE (ClayUIManager, clay_ui_manager, CLAY_TYPE_OBJECT)

#define parent_class clay_ui_manager_parent_class

static guint manager_signals[LAST_SIGNAL] = { 0 };


static void
clay_ui_manager_class_init (ClayUIManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed    = clay_ui_manager_constructed;
  object_class->dispose        = clay_ui_manager_dispose;
  object_class->finalize       = clay_ui_manager_finalize;
  object_class->set_property   = clay_ui_manager_set_property;
  object_class->get_property   = clay_ui_manager_get_property;

  klass->update                = clay_ui_manager_real_update;

  manager_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayUIManagerClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  manager_signals[SHOW_TOOLTIP] =
    g_signal_new ("show-tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayUIManagerClass, show_tooltip),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  manager_signals[HIDE_TOOLTIP] =
    g_signal_new ("hide-tooltip",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayUIManagerClass, hide_tooltip),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0,
                  G_TYPE_NONE);
  manager_signals[UI_ADDED] =
    g_signal_new ("ui-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayUIManagerClass, ui_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 3,
                  G_TYPE_STRING,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN);
  manager_signals[UI_REMOVED] =
    g_signal_new ("ui-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayUIManagerClass, ui_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_STRING);

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  klass->managers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, NULL);
}

static void
clay_ui_manager_init (ClayUIManager *manager)
{
  manager->name               = NULL;
  manager->clay               = NULL;
  manager->action_groups      = NULL;
  manager->store_action_paths = FALSE;
}

static void
clay_ui_manager_constructed (GObject *object)
{
  ClayUIManager *manager = CLAY_UI_MANAGER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (manager->name)
    {
      ClayUIManagerClass *manager_class;
      GList              *list;

      manager_class = CLAY_UI_MANAGER_GET_CLASS (object);

      list = g_hash_table_lookup (manager_class->managers, manager->name);

      list = g_list_append (list, manager);

      g_hash_table_replace (manager_class->managers,
                            g_strdup (manager->name), list);
    }
}

static void
clay_ui_manager_dispose (GObject *object)
{
  ClayUIManager *manager = CLAY_UI_MANAGER (object);

  if (manager->name)
    {
      ClayUIManagerClass *manager_class;
      GList              *list;

      manager_class = CLAY_UI_MANAGER_GET_CLASS (object);

      list = g_hash_table_lookup (manager_class->managers, manager->name);

      if (list)
        {
          list = g_list_remove (list, manager);

          if (list)
            g_hash_table_replace (manager_class->managers,
                                  g_strdup (manager->name), list);
          else
            g_hash_table_remove (manager_class->managers, manager->name);
        }
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_ui_manager_finalize (GObject *object)
{
  ClayUIManager *manager = CLAY_UI_MANAGER (object);
  GList         *list;

  for (list = manager->registered_uis; list; list = g_list_next (list))
    {
      ClayUIManagerUIEntry *entry = list->data;

      g_free (entry->ui_path);
      g_free (entry->basename);
      g_clear_object (&entry->builder);

      g_slice_free (ClayUIManagerUIEntry, entry);
    }

  g_clear_pointer (&manager->registered_uis, g_list_free);
  g_clear_pointer (&manager->name,           g_free);
  g_list_free_full (manager->action_groups,  g_object_unref);

  g_list_free_full (manager->ui_items,
                    (GDestroyNotify) clay_ui_manager_menu_item_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_ui_manager_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayUIManager *manager = CLAY_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_set_str (&manager->name, g_value_get_string (value));
      break;

    case PROP_CLAY:
      manager->clay = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_ui_manager_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayUIManager *manager = CLAY_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, manager->name);
      break;

    case PROP_CLAY:
      g_value_set_object (value, manager->clay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_ui_manager_real_update (ClayUIManager *manager,
                             gpointer       update_data)
{
  GList *list;

  for (list = clay_ui_manager_get_action_groups (manager);
       list;
       list = g_list_next (list))
    {
      clay_action_group_update (list->data, update_data);
    }
}

/**
 * clay_ui_manager_new:
 * @clay: the @Clay instance this ui manager belongs to
 * @name: the UI manager's name.
 *
 * Creates a new #ClayUIManager object.
 *
 * Returns: the new #ClayUIManager
 */
ClayUIManager *
clay_ui_manager_new (Clay        *clay,
                     const gchar *name)
{
  ClayUIManager *manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  manager = g_object_new (CLAY_TYPE_UI_MANAGER,
                          "name", name,
                          "clay", clay,
                          NULL);

  return manager;
}

GList *
clay_ui_managers_from_name (const gchar *name)
{
  ClayUIManagerClass *manager_class;
  GList              *list;

  g_return_val_if_fail (name != NULL, NULL);

  manager_class = g_type_class_ref (CLAY_TYPE_UI_MANAGER);

  list = g_hash_table_lookup (manager_class->managers, name);

  g_type_class_unref (manager_class);

  return list;
}

void
clay_ui_manager_update (ClayUIManager *manager,
                        gpointer       update_data)
{
  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));

  g_signal_emit (manager, manager_signals[UPDATE], 0, update_data);
}

void
clay_ui_manager_add_action_group (ClayUIManager   *manager,
                                  ClayActionGroup *group)
{
  if (! g_list_find (manager->action_groups, group))
    manager->action_groups = g_list_prepend (manager->action_groups, g_object_ref (group));

  /* Special-case the <Image> UI Manager which should be unique and represent
   * global application actions.
   */
  if (g_strcmp0 (manager->name, "<Image>") == 0)
    {
      gchar **actions = g_action_group_list_actions (G_ACTION_GROUP (group));

      for (int i = 0; i < g_strv_length (actions); i++)
        clay_ui_manager_image_action_added (group, actions[i], manager);

      g_signal_connect_object (group, "action-added",
                               G_CALLBACK (clay_ui_manager_image_action_added),
                               manager, 0);
      g_signal_connect_object (group, "action-removed",
                               G_CALLBACK (clay_ui_manager_image_action_removed),
                               manager, 0);

      g_strfreev (actions);
    }
}

ClayActionGroup *
clay_ui_manager_get_action_group (ClayUIManager *manager,
                                  const gchar   *name)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (list = clay_ui_manager_get_action_groups (manager);
       list;
       list = g_list_next (list))
    {
      ClayActionGroup *group = list->data;

      if (! strcmp (name, clay_action_group_get_name (group)))
        return group;
    }

  return NULL;
}

GList *
clay_ui_manager_get_action_groups (ClayUIManager *manager)
{
  return manager->action_groups;
}

ClayMenuModel *
clay_ui_manager_get_model (ClayUIManager *manager,
                           const gchar   *path)
{
  ClayUIManagerUIEntry *entry;
  ClayMenuModel        *model;
  GMenuModel           *gmodel;
  ClayMenuModel        *submodel;
  gchar                *root;
  gchar                *submenus;

  root     = g_strdup (path);
  submenus = strstr (root + 1, "/");
  if (submenus != NULL)
    {
      *submenus = '\0';
      if (*(++submenus) == '\0')
        submenus = NULL;
    }

  entry         = clay_ui_manager_entry_ensure (manager, path);
  g_return_val_if_fail (entry != NULL, NULL);

  if (entry->builder == NULL)
    {
      const gchar *menus_path_override = g_getenv ("CLAY_TESTING_MENUS_PATH");
      gchar       *full_basename;
      gchar       *filename;

      full_basename = g_strconcat (entry->basename, ".ui", NULL);

      /* In order for test cases to be able to run without Clay being
       * installed yet, allow them to override the menus directory to the
       * menus dir in the source root
       */
      if (menus_path_override)
        {
          GList *paths = clay_parse_search_path (menus_path_override, 2, FALSE, NULL);
          GList *list;

          for (list = paths; list; list = g_list_next (list))
            {
              filename = g_build_filename (list->data, full_basename, NULL);

              if (! list->next ||
                  g_file_test (filename, G_FILE_TEST_EXISTS))
                break;

              g_clear_pointer (&filename, g_free);
            }

          g_list_free_full (paths, g_free);
        }
      else
        {
          filename = g_build_filename (clay_data_directory (), "menus", full_basename, NULL);
        }

      g_return_val_if_fail (filename != NULL, NULL);

      if (manager->clay->be_verbose)
        g_print ("Loading menu '%s' for %s\n",
                 clay_filename_to_utf8 (filename), entry->ui_path);

      /* The model is owned by the builder which I have to keep around. */
      entry->builder = clay_ui_manager_load_builder (filename, root);

      g_free (filename);
      g_free (full_basename);
    }

  gmodel = G_MENU_MODEL (gtk_builder_get_object (entry->builder, root));

  g_return_val_if_fail (G_IS_MENU (gmodel), NULL);

  model = clay_menu_model_new (manager, gmodel);

  submodel = clay_menu_model_get_submodel (model, submenus);

  g_object_unref (model);
  g_free (root);

  return submodel;
}

void
clay_ui_manager_remove_ui (ClayUIManager *manager,
                           const gchar   *action_name)
{
  GList *iter = manager->ui_items;

  g_return_if_fail (action_name != NULL);

  for (; iter != NULL; iter = iter->next)
    {
      ClayUIManagerMenuItem *item = iter->data;

      if (g_strcmp0 (item->action_name, action_name) == 0)
        {
          g_signal_emit (manager, manager_signals[UI_REMOVED], 0,
                         item->path, item->action_name);
          manager->ui_items = g_list_delete_link (manager->ui_items, iter);
          clay_ui_manager_menu_item_free (item);
          break;
        }
    }
}

void
clay_ui_manager_remove_uis (ClayUIManager *manager,
                            const gchar   *action_name_prefix)
{
  GList *iter = manager->ui_items;

  while (iter != NULL)
    {
      ClayUIManagerMenuItem *item         = iter->data;
      GList                 *current_iter = iter;

      /* Increment nearly in case we delete the list item. */
      iter = iter->next;

      if (action_name_prefix == NULL ||
          g_str_has_prefix (item->action_name, action_name_prefix))
        {
          g_signal_emit (manager, manager_signals[UI_REMOVED], 0,
                         item->path, item->action_name);
          manager->ui_items = g_list_delete_link (manager->ui_items,
                                                  current_iter);
          clay_ui_manager_menu_item_free (item);
        }
    }
}

void
clay_ui_manager_add_ui (ClayUIManager *manager,
                        const gchar   *path,
                        const gchar   *action_name,
                        gboolean       top)
{
  ClayUIManagerMenuItem *item;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  g_return_if_fail (path != NULL);
  g_return_if_fail (action_name != NULL);

  item = g_slice_new0 (ClayUIManagerMenuItem);
  item->path        = g_strdup (path);
  item->action_name = g_strdup (action_name);
  item->top         = top;

  manager->ui_items = g_list_prepend (manager->ui_items, item);

  g_signal_emit (manager, manager_signals[UI_ADDED], 0,
                 path, action_name, top);
}

void
clay_ui_manager_foreach_ui (ClayUIManager      *manager,
                            ClayUIMenuCallback  callback,
                            gpointer            user_data)
{
  for (GList *iter = g_list_last (manager->ui_items); iter; iter = iter->prev)
    {
      ClayUIManagerMenuItem *item = iter->data;

      callback (manager, item->path, item->action_name, item->top, user_data);
    }
}

ClayAction *
clay_ui_manager_find_action (ClayUIManager *manager,
                             const gchar   *group_name,
                             const gchar   *action_name)
{
  ClayActionGroup *group;
  ClayAction      *action = NULL;

  g_return_val_if_fail (CLAY_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);

  if (g_strcmp0 (group_name, "app") == 0)
    {
      GApplication *app = manager->clay->app;

      action = (ClayAction *) g_action_map_lookup_action (G_ACTION_MAP (app), action_name);
    }
  else if (group_name)
    {
      group = clay_ui_manager_get_action_group (manager, group_name);

      if (group)
        action = clay_action_group_get_action (group, action_name);
    }
  else
    {
      GList       *list;
      const gchar *dot;

      dot = strstr (action_name, ".");

      if (dot == NULL)
        {
          /* No group specified. */
          for (list = clay_ui_manager_get_action_groups (manager);
               list;
               list = g_list_next (list))
            {
              group = list->data;

              action = clay_action_group_get_action (group, action_name);

              if (action)
                break;
            }
        }
      else
        {
          gchar *gname;

          gname = g_strndup (action_name, dot - action_name);

          action = clay_ui_manager_find_action (manager, gname, dot + 1);
          g_free (gname);
        }
    }

  return action;
}

gboolean
clay_ui_manager_activate_action (ClayUIManager *manager,
                                 const gchar   *group_name,
                                 const gchar   *action_name)
{
  ClayAction *action;

  g_return_val_if_fail (CLAY_IS_UI_MANAGER (manager), FALSE);
  g_return_val_if_fail (action_name != NULL, FALSE);

  action = clay_ui_manager_find_action (manager, group_name, action_name);

  if (action)
    clay_action_activate (action);

  return (action != NULL);
}

gboolean
clay_ui_manager_toggle_action (ClayUIManager *manager,
                               const gchar   *group_name,
                               const gchar   *action_name,
                               gboolean       active)
{
  ClayAction *action;

  g_return_val_if_fail (CLAY_IS_UI_MANAGER (manager), FALSE);
  g_return_val_if_fail (action_name != NULL, FALSE);

  action = clay_ui_manager_find_action (manager, group_name, action_name);

  if (CLAY_IS_TOGGLE_ACTION (action))
    clay_toggle_action_set_active (CLAY_TOGGLE_ACTION (action),
                                   active ? TRUE : FALSE);

  return CLAY_IS_TOGGLE_ACTION (action);
}

void
clay_ui_manager_ui_register (ClayUIManager          *manager,
                             const gchar            *ui_path,
                             const gchar            *basename,
                             ClayUIManagerSetupFunc  setup_func)
{
  ClayUIManagerUIEntry *entry;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (basename != NULL);
  g_return_if_fail (clay_ui_manager_entry_get (manager, ui_path) == NULL);

  entry = g_slice_new0 (ClayUIManagerUIEntry);

  entry->ui_path    = g_strdup (ui_path);
  entry->basename   = g_strdup (basename);
  entry->setup_func = setup_func;
  entry->setup_done = FALSE;
  entry->builder    = NULL;

  manager->registered_uis = g_list_prepend (manager->registered_uis, entry);
}

void
clay_ui_manager_ui_popup_at_widget (ClayUIManager  *manager,
                                    const gchar    *ui_path,
                                    ClayUIManager  *child_ui_manager,
                                    const gchar    *child_ui_path,
                                    GtkWidget      *widget,
                                    GdkGravity      widget_anchor,
                                    GdkGravity      menu_anchor,
                                    const GdkEvent *trigger_event,
                                    GDestroyNotify  popdown_func,
                                    gpointer        popdown_data)
{
  ClayMenuModel *model;
  GtkWidget     *menu;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  menu = clay_menu_new (manager);
  gtk_menu_attach_to_widget (GTK_MENU (menu), widget, NULL);

  model = clay_ui_manager_get_model (manager, ui_path);
  g_return_if_fail (model != NULL);
  clay_menu_shell_fill (CLAY_MENU_SHELL (menu), model, TRUE);
  g_object_unref (model);

  if (! menu)
    return;

  if (child_ui_manager != NULL && child_ui_path != NULL)
    {
      ClayMenuModel *child_model;
      GtkWidget     *child_menu;

      /* TODO GMenu: the "icon" attribute set in the .ui file should be visible. */
      child_model = clay_ui_manager_get_model (child_ui_manager, child_ui_path);
      child_menu  = clay_menu_new (child_ui_manager);
      clay_menu_shell_fill (CLAY_MENU_SHELL (child_menu), child_model, FALSE);
      g_object_unref (child_model);

      clay_menu_merge (CLAY_MENU (menu), CLAY_MENU (child_menu), TRUE);
    }

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (menu, "selection-done",
                        G_CALLBACK (clay_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup_at_widget (GTK_MENU (menu), widget,
                            widget_anchor,
                            menu_anchor,
                            trigger_event);
  g_signal_connect (menu, "hide",
                    G_CALLBACK (clay_ui_manager_popup_hidden),
                    NULL);
}

void
clay_ui_manager_ui_popup_at_pointer (ClayUIManager  *manager,
                                     const gchar    *ui_path,
                                     GtkWidget      *attached_widget,
                                     const GdkEvent *trigger_event,
                                     GDestroyNotify  popdown_func,
                                     gpointer        popdown_data)
{
  ClayMenuModel *model;
  GtkWidget     *menu;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  model = clay_ui_manager_get_model (manager, ui_path);
  menu  = clay_menu_new (manager);
  gtk_menu_attach_to_widget (GTK_MENU (menu), attached_widget, NULL);
  clay_menu_shell_fill (CLAY_MENU_SHELL (menu), model, TRUE);
  g_object_unref (model);

  if (! menu)
    return;

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (menu, "selection-done",
                        G_CALLBACK (clay_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup_at_pointer (GTK_MENU (menu), trigger_event);
  g_signal_connect (menu, "hide",
                    G_CALLBACK (clay_ui_manager_popup_hidden),
                    NULL);
}

void
clay_ui_manager_ui_popup_at_rect (ClayUIManager      *manager,
                                  const gchar        *ui_path,
                                  GtkWidget          *attached_widget,
                                  GdkWindow          *window,
                                  const GdkRectangle *rect,
                                  GdkGravity          rect_anchor,
                                  GdkGravity          menu_anchor,
                                  const GdkEvent     *trigger_event,
                                  GDestroyNotify      popdown_func,
                                  gpointer            popdown_data)
{
  ClayMenuModel *model;
  GtkWidget     *menu;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  model = clay_ui_manager_get_model (manager, ui_path);
  menu  = clay_menu_new (manager);
  gtk_menu_attach_to_widget (GTK_MENU (menu), attached_widget, NULL);
  clay_menu_shell_fill (CLAY_MENU_SHELL (menu), model, TRUE);
  g_object_unref (model);

  if (! menu)
    return;

  if (popdown_func && popdown_data)
    {
      g_object_set_data_full (G_OBJECT (manager), "popdown-data",
                              popdown_data, popdown_func);
      g_signal_connect (menu, "selection-done",
                        G_CALLBACK (clay_ui_manager_delete_popdown_data),
                        manager);
    }

  gtk_menu_popup_at_rect (GTK_MENU (menu), window,
                          rect, rect_anchor, menu_anchor,
                          trigger_event);
  g_signal_connect (menu, "hide",
                    G_CALLBACK (clay_ui_manager_popup_hidden),
                    NULL);
}


/*  private functions  */

static ClayUIManagerUIEntry *
clay_ui_manager_entry_get (ClayUIManager *manager,
                           const gchar   *ui_path)
{
  GList *list;
  gchar *path;

  path = g_strdup (ui_path);

  if (strlen (path) > 1)
    {
      gchar *p = strchr (path + 1, '/');

      if (p)
        *p = '\0';
    }

  for (list = manager->registered_uis; list; list = g_list_next (list))
    {
      ClayUIManagerUIEntry *entry = list->data;

      if (! strcmp (entry->ui_path, path))
        {
          g_free (path);

          return entry;
        }
    }

  g_free (path);

  return NULL;
}

static ClayUIManagerUIEntry *
clay_ui_manager_entry_ensure (ClayUIManager *manager,
                              const gchar   *path)
{
  ClayUIManagerUIEntry *entry;

  entry = clay_ui_manager_entry_get (manager, path);

  if (! entry)
    {
      g_warning ("%s: no entry registered for \"%s\"", G_STRFUNC, path);
      return NULL;
    }

  if (entry->setup_func && ! entry->setup_done)
    {
      entry->setup_func (manager, entry->ui_path);
      entry->setup_done = TRUE;
    }

  return entry;
}

static void
clay_ui_manager_store_builder_path (GMenuModel *model,
                                    GMenuModel *original)
{
  gint n_items = 0;

  g_return_if_fail (g_menu_model_get_n_items (model) == g_menu_model_get_n_items (original));

  n_items = (model != NULL ? g_menu_model_get_n_items (model) : 0);
  for (gint i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      GMenuModel *en_submodel = NULL;

      subsection = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SUBMENU);
      if (subsection != NULL)
        {
          en_submodel = g_menu_model_get_item_link (G_MENU_MODEL (original), i, G_MENU_LINK_SECTION);

          clay_ui_manager_store_builder_path (subsection, en_submodel);
        }
      else if (submenu != NULL)
        {
          gchar *label = NULL;

          en_submodel = g_menu_model_get_item_link (G_MENU_MODEL (original),
                                                    i, G_MENU_LINK_SUBMENU);
          clay_ui_manager_store_builder_path (submenu, en_submodel);

          g_menu_model_get_item_attribute (G_MENU_MODEL (model), i,
                                           G_MENU_ATTRIBUTE_LABEL, "s", &label);
          if (label != NULL)
            {
              gchar *en_label = NULL;

              g_menu_model_get_item_attribute (G_MENU_MODEL (original), i,
                                               G_MENU_ATTRIBUTE_LABEL, "s", &en_label);

              g_object_set_data_full (G_OBJECT (submenu), "clay-ui-manager-menu-model-en-label",
                                      en_label, (GDestroyNotify) g_free);

              /* also keep the translated label so helper widgets (e.g. the
                 action search popup `/`) can show a localized menu path */
              g_object_set_data_full (G_OBJECT (submenu), "clay-ui-manager-menu-model-label",
                                      g_strdup (label), (GDestroyNotify) g_free);
            }

          g_free (label);
        }

      g_clear_object (&submenu);
      g_clear_object (&subsection);
      g_clear_object (&en_submodel);
    }
}

static GtkBuilder *
clay_ui_manager_load_builder (const gchar *filename,
                              const gchar *root)
{
  const GMarkupParser markup_parser =
    {
      clay_ui_manager_parse_start_element,
      clay_ui_manager_parse_end_element,
      clay_ui_manager_parse_ui_text,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };

  GtkBuilder    *builder;
  GtkBuilder    *en_builder;
  gchar         *contents;
  ClayXmlParser *xml_parser;
  GString       *new_xml;
  GMenuModel    *model;
  GMenuModel    *en_model;
  GError        *error = NULL;

  /* First load the localized version. */
  if (! g_file_get_contents (filename, &contents, NULL, &error))
    {
      g_warning ("%s: failed to read \"%s\"", G_STRFUNC, filename);
      g_clear_error (&error);
      return NULL;
    }
  builder = gtk_builder_new_from_string (contents, -1);

  /* Now transform the source XML as non-translatable and load it again. */
  new_xml = g_string_new (NULL);
  xml_parser = clay_xml_parser_new (&markup_parser, new_xml);
  clay_xml_parser_parse_buffer (xml_parser, contents, -1, &error);
  clay_xml_parser_free (xml_parser);

  g_free (contents);
  contents = g_string_free (new_xml, FALSE);

  if (error != NULL)
    {
      g_warning ("%s: error parsing XML file \"%s\"", G_STRFUNC, filename);
      g_clear_error (&error);
      g_free (contents);
      return builder;
    }
  en_builder = gtk_builder_new_from_string (contents, -1);
  g_free (contents);

  model    = G_MENU_MODEL (gtk_builder_get_object (builder, root));
  en_model = G_MENU_MODEL (gtk_builder_get_object (en_builder, root));
  clay_ui_manager_store_builder_path (model, en_model);

  g_clear_object (&en_builder);

  return builder;
}

static void
clay_ui_manager_parse_start_element (GMarkupParseContext *context,
                                     const gchar         *element_name,
                                     const gchar        **attribute_names,
                                     const gchar        **attribute_values,
                                     gpointer             user_data,
                                     GError             **error)
{
  GString *new_xml = user_data;

  g_string_append_printf (new_xml, "<%s", element_name);
  for (gint i = 0; attribute_names[i] != NULL; i++)
    {
      if (g_strcmp0 (attribute_names[i], "translatable") == 0 ||
          g_strcmp0 (attribute_names[i], "context") == 0)
        continue;

      g_string_append_printf (new_xml, " %s=\"%s\"",
                              attribute_names[i], attribute_values[i]);
    }
  g_string_append_printf (new_xml, ">");
}

static void
clay_ui_manager_parse_end_element (GMarkupParseContext *context,
                                   const gchar         *element_name,
                                   gpointer             user_data,
                                   GError             **error)
{
  GString *new_xml = user_data;

  g_string_append_printf (new_xml, "</%s>", element_name);
}

static void
clay_ui_manager_parse_ui_text (GMarkupParseContext *context,
                               const gchar         *text,
                               gsize                text_len,
                               gpointer             user_data,
                               GError             **error)
{
  GString *new_xml   = user_data;
  gchar   *unchanged = g_markup_escape_text (text, text_len);

  g_string_append (new_xml, unchanged);
  g_free (unchanged);
}

static void
clay_ui_manager_delete_popdown_data (GtkWidget     *widget,
                                     ClayUIManager *manager)
{
  g_signal_handlers_disconnect_by_func (widget,
                                        clay_ui_manager_delete_popdown_data,
                                        manager);
  g_object_set_data (G_OBJECT (manager), "popdown-data", NULL);
}

static void
clay_ui_manager_menu_item_free (ClayUIManagerMenuItem *item)
{
  g_free (item->path);
  g_free (item->action_name);

  g_slice_free (ClayUIManagerMenuItem, item);
}

static void
clay_ui_manager_popup_hidden (GtkMenuShell *popup,
                              gpointer      user_data)
{
  /* Destroying the popup would happen after we finish all other events related
   * to this popup. Otherwise the action which might have been activated will
   * not run because it happens after hiding.
   */
  g_idle_add ((GSourceFunc) clay_ui_manager_popup_destroy, g_object_ref (popup));
}

static gboolean
clay_ui_manager_popup_destroy (GtkWidget *popup)
{
  gtk_menu_detach (GTK_MENU (popup));
  g_object_unref (popup);

  return G_SOURCE_REMOVE;
}

static void
clay_ui_manager_image_action_added (ClayActionGroup *group,
                                    gchar           *action_name,
                                    ClayUIManager   *manager)
{
  ClayAction   *action;
  const gchar **accels;

  g_return_if_fail (CLAY_IS_UI_MANAGER (manager));
  /* The action should not already exist in the application. */
  g_return_if_fail (g_action_map_lookup_action (G_ACTION_MAP (manager->clay->app), action_name) == NULL);

  action = clay_action_group_get_action (group, action_name);
  g_return_if_fail (action != NULL);

  g_action_map_add_action (G_ACTION_MAP (manager->clay->app), G_ACTION (action));
  g_signal_connect_object (action, "accels-changed",
                           G_CALLBACK (clay_ui_manager_image_accels_changed),
                           manager, 0);

  accels = clay_action_get_accels (action);
  if (accels && g_strv_length ((gchar **) accels) > 0)
    clay_ui_manager_image_accels_changed (action, clay_action_get_accels (action), manager);
}

static void
clay_ui_manager_image_action_removed (ClayActionGroup *group,
                                      gchar           *action_name,
                                      ClayUIManager   *manager)
{
  GAction *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (manager->clay->app), action_name);

  if (action != NULL)
    {
      g_action_map_remove_action (G_ACTION_MAP (manager->clay->app), action_name);
      g_signal_handlers_disconnect_by_func (action,
                                            G_CALLBACK (clay_ui_manager_image_accels_changed),
                                            manager);
    }
}

static void
clay_ui_manager_image_accels_changed (ClayAction     *action,
                                      const gchar   **accels,
                                      ClayUIManager  *manager)
{
  gchar *detailed_action_name;

  if (CLAY_IS_RADIO_ACTION (action))
    {
      gint value;

      g_object_get ((GObject *) action,
                    "value", &value,
                    NULL);
      detailed_action_name = g_strdup_printf ("app.%s(%i)",
                                               g_action_get_name (G_ACTION (action)),
                                               value);
    }
  else
    {
      detailed_action_name = g_strdup_printf ("app.%s",
                                               g_action_get_name (G_ACTION (action)));
    }

  gtk_application_set_accels_for_action (GTK_APPLICATION (manager->clay->app),
                                         detailed_action_name,
                                         accels ? accels : (const gchar *[]) { NULL });
  g_free (detailed_action_name);
}
