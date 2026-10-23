/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libclaywidgets/claywidgets.h"

#include "menus-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claylist.h"
#include "core/claytoolinfo.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayuimanager.h"

#include "tool-options-menu.h"


/*  local function prototypes  */

static void   tool_options_menu_update         (ClayUIManager *manager,
                                                gpointer       update_data,
                                                const gchar   *ui_path);
static void   tool_options_menu_update_after   (ClayUIManager *manager,
                                                gpointer       update_data,
                                                const gchar   *ui_path);
static void   tool_options_menu_update_presets (ClayUIManager *manager,
                                                const gchar   *menu_path,
                                                const gchar   *which_action,
                                                ClayContainer *presets);


/*  public functions  */

void
tool_options_menu_setup (ClayUIManager *manager,
                         const gchar   *ui_path)
{
  g_signal_connect (manager, "update",
                    G_CALLBACK (tool_options_menu_update),
                    (gpointer) ui_path);
  g_signal_connect_after (manager, "update",
                          G_CALLBACK (tool_options_menu_update_after),
                          (gpointer) ui_path);

  tool_options_menu_update_after (manager, NULL, ui_path);
}


/*  private functions  */

static void
tool_options_menu_update (ClayUIManager *manager,
                          gpointer       update_data,
                          const gchar   *ui_path)
{
  /* no-op. */
}

static void
tool_options_menu_update_after (ClayUIManager *manager,
                                gpointer       update_data,
                                const gchar   *ui_path)
{
  ClayContext  *context;
  ClayToolInfo *tool_info;

  context   = clay_get_user_context (manager->clay);
  tool_info = clay_context_get_tool (context);

  if (! tool_info->presets)
    return;

  tool_options_menu_update_presets (manager, "Save Tool Preset",
                                    "save", tool_info->presets);

  tool_options_menu_update_presets (manager, "Restore Tool Preset",
                                    "restore", tool_info->presets);

  tool_options_menu_update_presets (manager, "Edit Tool Preset",
                                    "edit", tool_info->presets);

  tool_options_menu_update_presets (manager, "Delete Tool Preset",
                                    "delete", tool_info->presets);
}

static void
tool_options_menu_update_presets (ClayUIManager *manager,
                                  const gchar   *menu_path,
                                  const gchar   *which_action,
                                  ClayContainer *presets)
{
  gchar *action_prefix;
  gint   n_children;
  gint   i;

  action_prefix = g_strdup_printf ("tool-options-%s-preset-", which_action);
  clay_ui_manager_remove_uis (manager, action_prefix);

  n_children = clay_container_get_n_children (presets);
  for (i = 0; i < n_children; i++)
    {
      gchar *action_name;
      gchar *path;

      action_name = g_strdup_printf ("%s%03d", action_prefix, i);
      path        = g_strdup_printf ("/Tool Options Menu/%s", menu_path);

      clay_ui_manager_add_ui (manager, path, action_name, FALSE);

      g_free (action_name);
      g_free (path);
    }

  g_free (action_prefix);
}
