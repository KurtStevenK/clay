/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenubar.c
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
#include "claymenu.h"
#include "claymenubar.h"
#include "claymenumodel.h"
#include "claymenushell.h"
#include "clayradioaction.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"


/**
 * ClayMenuBar:
 *
 * Our own menu bar widget.
 *
 * We cannot use the simpler gtk_application_set_menubar() because it lacks
 * tooltip support and unfortunately GTK does not plan to implement this:
 * https://gitlab.gnome.org/GNOME/gtk/-/issues/785
 * This is why we need to implement our own ClayMenuBar subclass.
 */

struct _ClayMenuBarPrivate
{
  GTree *menus;
};


/*  local function prototypes  */

static void   clay_menu_bar_iface_init              (ClayMenuShellInterface  *iface);

static void   clay_menu_bar_dispose                 (GObject                 *object);

static void   clay_menu_bar_append                  (ClayMenuShell           *shell,
                                                     ClayMenuModel           *model);
static void   clay_menu_bar_add_ui                  (ClayMenuShell           *shell,
                                                     const gchar            **paths,
                                                     const gchar             *action_name,
                                                     gboolean                 top);


G_DEFINE_TYPE_WITH_CODE (ClayMenuBar, clay_menu_bar, GTK_TYPE_MENU_BAR,
                         G_ADD_PRIVATE (ClayMenuBar)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_MENU_SHELL, clay_menu_bar_iface_init))

#define parent_class clay_menu_bar_parent_class


/*  Class functions  */

static void
clay_menu_bar_class_init (ClayMenuBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = clay_menu_bar_dispose;
  object_class->get_property = clay_menu_shell_get_property;
  object_class->set_property = clay_menu_shell_set_property;

  clay_menu_shell_install_properties (object_class);
}

static void
clay_menu_bar_iface_init (ClayMenuShellInterface *iface)
{
  iface->append = clay_menu_bar_append;
  iface->add_ui = clay_menu_bar_add_ui;
}

static void
clay_menu_bar_init (ClayMenuBar *bar)
{
  bar->priv = clay_menu_bar_get_instance_private (bar);

  bar->priv->menus = g_tree_new_full ((GCompareDataFunc) g_strcmp0, NULL,
                                      g_free, NULL);

  clay_menu_shell_init (CLAY_MENU_SHELL (bar));
}

static void
clay_menu_bar_dispose (GObject *object)
{
  ClayMenuBar *bar = CLAY_MENU_BAR (object);

  g_clear_pointer (&bar->priv->menus, g_tree_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_menu_bar_append (ClayMenuShell *shell,
                      ClayMenuModel *model)
{
  ClayMenuBar   *bar     = CLAY_MENU_BAR (shell);
  ClayUIManager *manager = clay_menu_shell_get_manager (CLAY_MENU_SHELL (shell));
  gint           n_items;

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

      if (submenu != NULL)
        {
          GtkWidget *subcontainer;
          GtkWidget *item;

          g_return_if_fail (label != NULL);

          /* I don't show the item on purpose because
           * clay_menu_append() will show the parent item if any of
           * the added actions are visible.
           */
          item = gtk_menu_item_new_with_mnemonic (label);
          gtk_container_add (GTK_CONTAINER (shell), item);

          subcontainer = clay_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), subcontainer);
          clay_menu_shell_append (CLAY_MENU_SHELL (subcontainer), CLAY_MENU_MODEL (submenu));
          gtk_widget_set_visible (subcontainer, TRUE);

          g_tree_insert (bar->priv->menus,
                         clay_utils_make_canonical_menu_label (label),
                         subcontainer);
        }
      else
        {
          if (subsection != NULL)
            g_warning ("%s: unexpected subsection. Only menus are allowed in menu bar's root.",
                       G_STRFUNC);
          if (action_name == NULL)
            g_warning ("%s: missing action name. Only menus are allowed in menu bar's root.",
                       G_STRFUNC);
          else
            g_warning ("%s: unexpected action '%s'. Only menus are allowed in menu bar's root.",
                       G_STRFUNC, action_name);
        }
      g_free (label);
      g_free (action_name);
      g_clear_object (&submenu);
      g_clear_object (&subsection);
    }
}

static void
clay_menu_bar_add_ui (ClayMenuShell  *shell,
                      const gchar   **paths,
                      const gchar    *action_name,
                      gboolean        top)
{
  ClayMenuBar   *bar     = CLAY_MENU_BAR (shell);
  ClayUIManager *manager = clay_menu_shell_get_manager (CLAY_MENU_SHELL (shell));

  g_return_if_fail (paths != NULL);

  if (paths[0] == NULL)
    {
      g_warning ("%s: unexpected action '%s'. Only menus are allowed in menu bar's root.",
                 G_STRFUNC, action_name);
    }
  else
    {
      GtkWidget *menu = NULL;

      menu = g_tree_lookup (bar->priv->menus, paths[0]);

      if (menu == NULL)
        {
          GtkWidget *item;

          item = gtk_menu_item_new_with_mnemonic (paths[0]);
          gtk_container_add (GTK_CONTAINER (shell), item);

          menu = clay_menu_new (manager);
          gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
          gtk_widget_set_visible (menu, TRUE);

          g_tree_insert (bar->priv->menus, g_strdup (paths[0]), menu);
        }

      CLAY_MENU_SHELL_GET_INTERFACE (menu)->add_ui (CLAY_MENU_SHELL (menu),
                                                    paths + 1, action_name, top);
    }
}


/* Public functions */

GtkWidget *
clay_menu_bar_new (ClayMenuModel *model,
                   ClayUIManager *manager)
{
  g_return_val_if_fail (CLAY_IS_UI_MANAGER (manager) &&
                        G_IS_MENU_MODEL (model), NULL);

  return g_object_new (CLAY_TYPE_MENU_BAR,
                       "model",   model,
                       "manager", manager,
                       NULL);
}
