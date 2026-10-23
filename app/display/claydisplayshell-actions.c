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

#include "display-types.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "menus/menus.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayuimanager.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-actions.h"
#include "clayimagewindow.h"


void
clay_display_shell_set_action_sensitive (ClayDisplayShell *shell,
                                         const gchar      *action,
                                         gboolean          sensitive)
{
  ClayImageWindow *window;
  ClayContext     *context;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = clay_display_shell_get_window (shell);

  if (window && clay_image_window_get_active_shell (window) == shell)
    {
      ClayUIManager   *manager = menus_get_image_manager_singleton (shell->display->clay);
      ClayActionGroup *action_group;

      action_group = clay_ui_manager_get_action_group (manager, "view");

      if (action_group)
        clay_action_group_set_action_sensitive (action_group, action, sensitive, NULL);
    }

  context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (context))
    {
      ClayActionGroup *action_group;

      action_group = clay_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        clay_action_group_set_action_sensitive (action_group, action, sensitive, NULL);
    }
}

void
clay_display_shell_set_action_active (ClayDisplayShell *shell,
                                      const gchar      *action,
                                      gboolean          active)
{
  ClayImageWindow *window;
  ClayContext     *context;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = clay_display_shell_get_window (shell);

  if (window && clay_image_window_get_active_shell (window) == shell)
    {
      ClayUIManager   *manager = menus_get_image_manager_singleton (shell->display->clay);
      ClayActionGroup *action_group;

      action_group = clay_ui_manager_get_action_group (manager, "view");

      if (action_group)
        clay_action_group_set_action_active (action_group, action, active);
    }

  context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (context))
    {
      ClayActionGroup *action_group;

      action_group = clay_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        clay_action_group_set_action_active (action_group, action, active);
    }
}

void
clay_display_shell_set_action_color (ClayDisplayShell *shell,
                                     const gchar      *action,
                                     GeglColor        *color)
{
  ClayImageWindow *window;
  ClayContext     *context;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (action != NULL);

  window = clay_display_shell_get_window (shell);

  if (window && clay_image_window_get_active_shell (window) == shell)
    {
      ClayUIManager   *manager = menus_get_image_manager_singleton (shell->display->clay);
      ClayActionGroup *action_group;

      action_group = clay_ui_manager_get_action_group (manager, "view");

      if (action_group)
        clay_action_group_set_action_color (action_group, action, color, FALSE);
    }

  context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (context))
    {
      ClayActionGroup *action_group;

      action_group = clay_ui_manager_get_action_group (shell->popup_manager,
                                                       "view");

      if (action_group)
        clay_action_group_set_action_color (action_group, action, color, FALSE);
    }
}
