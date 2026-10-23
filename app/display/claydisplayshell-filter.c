/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1999 Manish Singh
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

#include "display-types.h"

#include "claydisplayshell.h"
#include "claydisplayshell-expose.h"
#include "claydisplayshell-filter.h"
#include "claydisplayshell-profile.h"
#include "claydisplayshell-render.h"


/*  local function prototypes  */

static void   clay_display_shell_filter_changed (ClayColorDisplayStack *stack,
                                                 ClayDisplayShell      *shell);


/*  public functions  */

void
clay_display_shell_filter_set (ClayDisplayShell      *shell,
                               ClayColorDisplayStack *stack)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (stack == NULL || CLAY_IS_COLOR_DISPLAY_STACK (stack));

  if (stack == shell->filter_stack)
    return;

  if (shell->filter_stack)
    {
      g_signal_handlers_disconnect_by_func (shell->filter_stack,
                                            clay_display_shell_filter_changed,
                                            shell);
    }

  g_set_object (&shell->filter_stack, stack);

  if (shell->filter_stack)
    {
      g_signal_connect (shell->filter_stack, "changed",
                        G_CALLBACK (clay_display_shell_filter_changed),
                        shell);
    }

  clay_display_shell_filter_changed (NULL, shell);
}

gboolean
clay_display_shell_has_filter (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  if (shell->filter_stack)
    {
      GList *filters;
      GList *iter;

      filters = clay_color_display_stack_get_filters (shell->filter_stack);

      for (iter = filters; iter; iter = g_list_next (iter))
        {
          if (clay_color_display_get_enabled (CLAY_COLOR_DISPLAY (iter->data)))
            return TRUE;
        }
    }

  return FALSE;
}


/*  private functions  */

static gboolean
clay_display_shell_filter_changed_idle (gpointer data)
{
  ClayDisplayShell *shell = data;

  clay_display_shell_profile_update (shell);
  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);

  shell->filter_idle_id = 0;

  return FALSE;
}

static void
clay_display_shell_filter_changed (ClayColorDisplayStack *stack,
                                   ClayDisplayShell      *shell)
{
  if (shell->filter_idle_id)
    g_source_remove (shell->filter_idle_id);

  shell->filter_idle_id =
    g_idle_add_full (G_PRIORITY_LOW,
                     clay_display_shell_filter_changed_idle,
                     shell, NULL);
}
