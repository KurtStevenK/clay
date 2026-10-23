/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytool-progress.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "tools-types.h"

#include "core/clayprogress.h"

#include "widgets/claywidgets-utils.h"

#include "display/claycanvasprogress.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-items.h"
#include "display/claydisplayshell-transform.h"

#include "claytool.h"
#include "claytool-progress.h"


/*  local function prototypes  */

static ClayProgress * clay_tool_progress_start     (ClayProgress        *progress,
                                                    gboolean             cancelable,
                                                    const gchar         *message);
static void           clay_tool_progress_end       (ClayProgress        *progress);
static gboolean       clay_tool_progress_is_active (ClayProgress        *progress);
static void           clay_tool_progress_set_text  (ClayProgress        *progress,
                                                    const gchar         *message);
static void           clay_tool_progress_set_value (ClayProgress        *progress,
                                                    gdouble              percentage);
static gdouble        clay_tool_progress_get_value (ClayProgress        *progress);
static void           clay_tool_progress_pulse     (ClayProgress        *progress);
static gboolean       clay_tool_progress_message   (ClayProgress        *progress,
                                                    Clay                *clay,
                                                    ClayMessageSeverity  severity,
                                                    const gchar         *domain,
                                                    const gchar         *message);


/*  public functions  */

void
clay_tool_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start     = clay_tool_progress_start;
  iface->end       = clay_tool_progress_end;
  iface->is_active = clay_tool_progress_is_active;
  iface->set_text  = clay_tool_progress_set_text;
  iface->set_value = clay_tool_progress_set_value;
  iface->get_value = clay_tool_progress_get_value;
  iface->pulse     = clay_tool_progress_pulse;
  iface->message   = clay_tool_progress_message;
}


/*  private functions  */

static gboolean
clay_tool_progress_button_press (GtkWidget            *widget,
                                 const GdkEventButton *bevent,
                                 ClayTool             *tool)
{
  if (tool->progress_cancelable          &&
      bevent->type   == GDK_BUTTON_PRESS &&
      bevent->button == 1)
    {
      GtkWidget        *event_widget;
      ClayDisplayShell *shell;

      event_widget = gtk_get_event_widget ((GdkEvent *) bevent);
      shell        = clay_display_get_shell (tool->progress_display);

      if (shell->canvas == event_widget)
        {
          gint x, y;

          clay_display_shell_unzoom_xy (shell, bevent->x, bevent->y,
                                        &x, &y, FALSE);

          if (clay_canvas_item_hit (tool->progress, x, y))
            {
              clay_progress_cancel (CLAY_PROGRESS (tool));
            }
        }
    }

  return TRUE;
}

static gboolean
clay_tool_progress_key_press (GtkWidget         *widget,
                              const GdkEventKey *kevent,
                              ClayTool          *tool)
{
  if (tool->progress_cancelable &&
      kevent->keyval == GDK_KEY_Escape)
    {
      clay_progress_cancel (CLAY_PROGRESS (tool));
    }

  return TRUE;
}

static ClayProgress *
clay_tool_progress_start (ClayProgress *progress,
                          gboolean      cancelable,
                          const gchar  *message)
{
  ClayTool         *tool = CLAY_TOOL (progress);
  ClayDisplayShell *shell;
  gint              x, y;

  g_return_val_if_fail (CLAY_IS_DISPLAY (tool->display), NULL);
  g_return_val_if_fail (tool->progress == NULL, NULL);

  shell = clay_display_get_shell (tool->display);

  x = shell->disp_width  / 2;
  y = shell->disp_height / 2;

  clay_display_shell_unzoom_xy (shell, x, y, &x, &y, FALSE);

  tool->progress = clay_canvas_progress_new (shell,
                                             CLAY_HANDLE_ANCHOR_CENTER,
                                             x, y);
  clay_display_shell_add_unrotated_item (shell, tool->progress);
  g_object_unref (tool->progress);

  clay_progress_start (CLAY_PROGRESS (tool->progress), FALSE,
                       "%s", message);

  tool->progress_display = tool->display;

  tool->progress_grab_widget = gtk_invisible_new ();
  gtk_widget_set_visible (tool->progress_grab_widget, TRUE);
  gtk_grab_add (tool->progress_grab_widget);

  g_signal_connect (tool->progress_grab_widget, "button-press-event",
                    G_CALLBACK (clay_tool_progress_button_press),
                    tool);
  g_signal_connect (tool->progress_grab_widget, "key-press-event",
                    G_CALLBACK (clay_tool_progress_key_press),
                    tool);

  tool->progress_cancelable = cancelable;

  return progress;
}

static void
clay_tool_progress_end (ClayProgress *progress)
{
  ClayTool *tool = CLAY_TOOL (progress);

  if (tool->progress)
    {
      ClayDisplayShell *shell = clay_display_get_shell (tool->progress_display);

      clay_progress_end (CLAY_PROGRESS (tool->progress));
      clay_display_shell_remove_unrotated_item (shell, tool->progress);

      gtk_grab_remove (tool->progress_grab_widget);
      gtk_widget_destroy (tool->progress_grab_widget);

      tool->progress             = NULL;
      tool->progress_display     = NULL;
      tool->progress_grab_widget = NULL;
      tool->progress_cancelable  = FALSE;
    }
}

static gboolean
clay_tool_progress_is_active (ClayProgress *progress)
{
  ClayTool *tool = CLAY_TOOL (progress);

  return tool->progress != NULL;
}

static void
clay_tool_progress_set_text (ClayProgress *progress,
                             const gchar  *message)
{
  ClayTool *tool = CLAY_TOOL (progress);

  if (tool->progress)
    clay_progress_set_text_literal (CLAY_PROGRESS (tool->progress), message);
}

static void
clay_tool_progress_set_value (ClayProgress *progress,
                              gdouble       percentage)
{
  ClayTool *tool = CLAY_TOOL (progress);

  if (tool->progress)
    clay_progress_set_value (CLAY_PROGRESS (tool->progress), percentage);
}

static gdouble
clay_tool_progress_get_value (ClayProgress *progress)
{
  ClayTool *tool = CLAY_TOOL (progress);

  if (tool->progress)
    return clay_progress_get_value (CLAY_PROGRESS (tool->progress));

  return 0.0;
}

static void
clay_tool_progress_pulse (ClayProgress *progress)
{
}

static gboolean
clay_tool_progress_message (ClayProgress        *progress,
                            Clay                *clay,
                            ClayMessageSeverity  severity,
                            const gchar         *domain,
                            const gchar         *message)
{
  return FALSE;
}
