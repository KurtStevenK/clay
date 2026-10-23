/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolbar.c
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "core/clay.h"

#include "clayaction.h"
#include "clayenumaction.h"
#include "clayprocedureaction.h"
#include "clayradioaction.h"
#include "claytoggleaction.h"
#include "claytoolbar.h"
#include "claymenumodel.h"
#include "claymenushell.h"
#include "clayuimanager.h"

#define CLAY_TOOLBAR_ACTION_KEY "clay-toolbar-action"


/**
 * ClayToolbar:
 *
 * Our own toolbar widget.
 *
 * It can be initialized with a GMenuModel.
 *
 * TODO: in the future, we could also give optional GUI-customizability to use
 * it as a generic toolbar in Clay's main window (where people could add any
 * actions as buttons).
 */


/*  local function prototypes  */

static void   clay_toolbar_iface_init              (ClayMenuShellInterface  *iface);

static void   clay_toolbar_append                  (ClayMenuShell           *shell,
                                                    ClayMenuModel           *model);
static void   clay_toolbar_add_ui                  (ClayMenuShell           *shell,
                                                    const gchar            **paths,
                                                    const gchar             *action_name,
                                                    gboolean                 top);

static void   clay_toolbar_add_action              (ClayToolbar             *toolbar,
                                                    const gchar             *action_name,
                                                    gboolean                 top);

static void   clay_toolbar_toggle_item_toggled     (GtkToggleToolButton     *item,
                                                    ClayToggleAction        *action);

static void   clay_toolbar_toggle_action_toggled   (ClayToggleAction        *action,
                                                    GtkToggleToolButton     *item);
static void   clay_toolbar_action_notify_sensitive (ClayAction              *action,
                                                    const GParamSpec        *pspec,
                                                    GtkWidget               *item);
static void   clay_toolbar_action_notify_visible   (ClayAction              *action,
                                                    const GParamSpec        *pspec,
                                                    GtkWidget               *item);


G_DEFINE_TYPE_WITH_CODE (ClayToolbar, clay_toolbar, GTK_TYPE_TOOLBAR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_MENU_SHELL, clay_toolbar_iface_init))

#define parent_class clay_toolbar_parent_class


/* Class functions */

static void
clay_toolbar_class_init (ClayToolbarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = clay_menu_shell_get_property;
  object_class->set_property = clay_menu_shell_set_property;

  clay_menu_shell_install_properties (object_class);
}

static void
clay_toolbar_iface_init (ClayMenuShellInterface *iface)
{
  iface->append = clay_toolbar_append;
  iface->add_ui = clay_toolbar_add_ui;
}

static void
clay_toolbar_init (ClayToolbar *bar)
{
  clay_menu_shell_init (CLAY_MENU_SHELL (bar));
}

static void
clay_toolbar_append (ClayMenuShell *shell,
                     ClayMenuModel *model)
{
  ClayToolbar *toolbar = CLAY_TOOLBAR (shell);
  gint         n_items;

  g_return_if_fail (GTK_IS_CONTAINER (shell));

  n_items = g_menu_model_get_n_items (G_MENU_MODEL (model));
  for (gint i = 0; i < n_items; i++)
    {
      GMenuModel *subsection;
      GMenuModel *submenu;
      gchar      *label       = NULL;
      gchar      *action_name = NULL;

      subsection = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SECTION);
      submenu    = g_menu_model_get_item_link (G_MENU_MODEL (model), i, G_MENU_LINK_SUBMENU);
      g_menu_model_get_item_attribute (G_MENU_MODEL (model), i, G_MENU_ATTRIBUTE_LABEL, "s", &label);
      g_menu_model_get_item_attribute (G_MENU_MODEL (model), i, G_MENU_ATTRIBUTE_ACTION, "s", &action_name);

      if (subsection != NULL)
        {
          GtkToolItem *item;

          item = gtk_separator_tool_item_new ();
          gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
          gtk_widget_set_visible (GTK_WIDGET (item), TRUE);

          clay_toolbar_append (shell, CLAY_MENU_MODEL (subsection));

          item = gtk_separator_tool_item_new ();
          gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
          gtk_widget_set_visible (GTK_WIDGET (item), TRUE);
        }
      else if (submenu == NULL && action_name != NULL)
        {
          clay_toolbar_add_action (toolbar, action_name, FALSE);
        }
      else
        {
          if (submenu != NULL)
            g_warning ("%s: unexpected submenu. Only actions and sections are allowed in toolbar's root.",
                       G_STRFUNC);
          else if (label != NULL)
            g_warning ("%s: unexpected label '%s'. Only actions and sections are allowed in toolbar's root.",
                       G_STRFUNC, label);
          else
            g_warning ("%s: unexpected toolbar item. Only actions and sections are allowed in toolbar's root.",
                       G_STRFUNC);
        }

      g_free (label);
      g_free (action_name);
      g_clear_object (&submenu);
      g_clear_object (&subsection);
    }
}

static void
clay_toolbar_add_ui (ClayMenuShell  *shell,
                     const gchar   **paths,
                     const gchar    *action_name,
                     gboolean        top)
{
  ClayToolbar *toolbar = CLAY_TOOLBAR (shell);

  g_return_if_fail (paths != NULL);

  if (paths[0] != NULL)
    g_warning ("%s: unexpected path '%s'. Menus are not allowed in toolbar's root.",
               G_STRFUNC, paths[0]);
  else
    clay_toolbar_add_action (toolbar, action_name, top);
}


/* Public functions */

GtkWidget *
clay_toolbar_new (ClayMenuModel *model,
                  ClayUIManager *manager)
{
  g_return_val_if_fail (CLAY_IS_UI_MANAGER (manager) &&
                        G_IS_MENU_MODEL (model), NULL);

  return g_object_new (CLAY_TYPE_TOOLBAR,
                       "model",   model,
                       "manager", manager,
                       NULL);
}


/* Private functions */

static void
clay_toolbar_add_action (ClayToolbar *toolbar,
                         const gchar *action_name,
                         gboolean     top)
{
  ClayUIManager *manager;
  ClayAction    *action;
  const gchar   *action_label;
  GtkToolItem   *item;
  gboolean       visible;

  g_return_if_fail (CLAY_IS_TOOLBAR (toolbar));

  manager = clay_menu_shell_get_manager (CLAY_MENU_SHELL (toolbar));
  action  = clay_ui_manager_find_action (manager, NULL, action_name);

  g_return_if_fail (CLAY_IS_ACTION (action));

  action_label = clay_action_get_short_label (CLAY_ACTION (action));
  g_return_if_fail (action_label != NULL);

  if (CLAY_IS_TOGGLE_ACTION (action))
    {
      if (CLAY_IS_RADIO_ACTION (action))
        {
          GtkToolItem *sibling = NULL;
          GSList      *group = NULL;

          if (top)
            sibling = gtk_toolbar_get_nth_item (GTK_TOOLBAR (toolbar), 0);
          else if (gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) > 0)
            sibling = gtk_toolbar_get_nth_item (GTK_TOOLBAR (toolbar),
                                                gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar)) - 1);

          if (sibling != NULL && GTK_IS_RADIO_TOOL_BUTTON (sibling))
            {
              ClayAction *sibling_action;

              sibling_action = g_object_get_data (G_OBJECT (sibling), CLAY_TOOLBAR_ACTION_KEY);

              if (sibling_action != NULL                &&
                  CLAY_IS_RADIO_ACTION (sibling_action) &&
                  clay_radio_action_get_group (CLAY_RADIO_ACTION (sibling_action)) == clay_radio_action_get_group (CLAY_RADIO_ACTION (action)))
                group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (sibling));
            }

          item = gtk_radio_tool_button_new (group);
        }
      else
        {
          item = gtk_toggle_tool_button_new ();
        }

      gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), action_label);
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (item),
                                         clay_toggle_action_get_active (CLAY_TOGGLE_ACTION (action)));

      g_signal_connect (item, "toggled",
                        G_CALLBACK (clay_toolbar_toggle_item_toggled),
                        action);
      g_signal_connect_object (action, "toggled",
                               G_CALLBACK (clay_toolbar_toggle_action_toggled),
                               item, 0);
    }
  else
    {
      item = gtk_tool_button_new (NULL, action_label);

      g_signal_connect_swapped (item, "clicked",
                                G_CALLBACK (clay_action_activate),
                                action);
    }

  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item),
                                 clay_action_get_icon_name (CLAY_ACTION (action)));

  g_object_set_data (G_OBJECT (item), CLAY_TOOLBAR_ACTION_KEY, action);

  clay_action_set_proxy (CLAY_ACTION (action), GTK_WIDGET (item));

  gtk_widget_set_sensitive (GTK_WIDGET (item),
                            clay_action_is_sensitive (CLAY_ACTION (action), NULL));
  g_signal_connect_object (action, "notify::sensitive",
                           G_CALLBACK (clay_toolbar_action_notify_sensitive),
                           item, 0);

  if (top)
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, 0);
  else
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

  visible = clay_action_is_visible (CLAY_ACTION (action));
  gtk_widget_set_visible (GTK_WIDGET (item), visible);
  g_signal_connect_object (action, "notify::visible",
                           G_CALLBACK (clay_toolbar_action_notify_visible),
                           item, 0);
}

static void
clay_toolbar_toggle_item_toggled (GtkToggleToolButton *item,
                                  ClayToggleAction    *action)
{
  gboolean active = gtk_toggle_tool_button_get_active (item);

  g_signal_handlers_block_by_func (action,
                                   G_CALLBACK (clay_toolbar_toggle_action_toggled),
                                   item);
  clay_toggle_action_set_active (action, active);
  g_signal_handlers_unblock_by_func (action,
                                     G_CALLBACK (clay_toolbar_toggle_action_toggled),
                                     item);
}

static void
clay_toolbar_toggle_action_toggled (ClayToggleAction    *action,
                                    GtkToggleToolButton *item)
{
  gboolean active = clay_toggle_action_get_active (action);

  g_signal_handlers_block_by_func (item,
                                   G_CALLBACK (clay_toolbar_toggle_item_toggled),
                                   action);
  gtk_toggle_tool_button_set_active (item, active);
  g_signal_handlers_unblock_by_func (item,
                                   G_CALLBACK (clay_toolbar_toggle_item_toggled),
                                   action);
}

static void
clay_toolbar_action_notify_sensitive (ClayAction       *action,
                                      const GParamSpec *pspec,
                                      GtkWidget        *item)
{
  gtk_widget_set_sensitive (item, clay_action_is_sensitive (action, NULL));
}

static void
clay_toolbar_action_notify_visible (ClayAction       *action,
                                    const GParamSpec *pspec,
                                    GtkWidget        *item)
{
  gtk_widget_set_visible (item, clay_action_is_visible (action));
}
