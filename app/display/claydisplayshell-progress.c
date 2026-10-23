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

#include "core/clayprogress.h"

#include "widgets/claywidgets-utils.h"

#include "claydisplayshell.h"
#include "claydisplayshell-progress.h"
#include "claystatusbar.h"


static ClayProgress *
clay_display_shell_progress_start (ClayProgress *progress,
                                   gboolean      cancellable,
                                   const gchar  *message)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  return clay_progress_start (CLAY_PROGRESS (statusbar), cancellable,
                              "%s", message);
}

static void
clay_display_shell_progress_end (ClayProgress *progress)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  clay_progress_end (CLAY_PROGRESS (statusbar));
}

static gboolean
clay_display_shell_progress_is_active (ClayProgress *progress)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  return clay_progress_is_active (CLAY_PROGRESS (statusbar));
}

static void
clay_display_shell_progress_set_text (ClayProgress *progress,
                                      const gchar  *message)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  clay_progress_set_text_literal (CLAY_PROGRESS (statusbar), message);
}

static void
clay_display_shell_progress_set_value (ClayProgress *progress,
                                       gdouble       percentage)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  clay_progress_set_value (CLAY_PROGRESS (statusbar), percentage);
}

static gdouble
clay_display_shell_progress_get_value (ClayProgress *progress)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  return clay_progress_get_value (CLAY_PROGRESS (statusbar));
}

static void
clay_display_shell_progress_pulse (ClayProgress *progress)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  clay_progress_pulse (CLAY_PROGRESS (statusbar));
}

static GBytes *
clay_display_shell_progress_get_window_handle (ClayProgress *progress)
{
  ClayDisplayShell *shell  = CLAY_DISPLAY_SHELL (progress);
  GBytes           *handle = NULL;

  if (shell->window_handle)
    handle = g_bytes_ref (shell->window_handle);

  return handle;
}

static gboolean
clay_display_shell_progress_message (ClayProgress        *progress,
                                     Clay                *clay,
                                     ClayMessageSeverity  severity,
                                     const gchar         *domain,
                                     const gchar         *message)
{
  ClayDisplayShell *shell     = CLAY_DISPLAY_SHELL (progress);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

  switch (severity)
    {
    case CLAY_MESSAGE_ERROR:
    case CLAY_MESSAGE_BUG_WARNING:
    case CLAY_MESSAGE_BUG_CRITICAL:
      /* error messages are never handled here */
      break;

    case CLAY_MESSAGE_WARNING:
      /* warning messages go to the statusbar, if it's visible */
      if (! clay_statusbar_get_visible (statusbar))
        break;
      else
        return clay_progress_message (CLAY_PROGRESS (statusbar), clay,
                                      severity, domain, message);

    case CLAY_MESSAGE_INFO:
      /* info messages go to the statusbar;
       * if they are not handled there, they are swallowed
       */
      clay_progress_message (CLAY_PROGRESS (statusbar), clay,
                             severity, domain, message);
      return TRUE;
    }

  return FALSE;
}

void
clay_display_shell_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start             = clay_display_shell_progress_start;
  iface->end               = clay_display_shell_progress_end;
  iface->is_active         = clay_display_shell_progress_is_active;
  iface->set_text          = clay_display_shell_progress_set_text;
  iface->set_value         = clay_display_shell_progress_set_value;
  iface->get_value         = clay_display_shell_progress_get_value;
  iface->pulse             = clay_display_shell_progress_pulse;
  iface->get_window_handle = clay_display_shell_progress_get_window_handle;
  iface->message           = clay_display_shell_progress_message;
}
