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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydisplayconfig.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"

#include "widgets/clayactiongroup.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "dialogs-actions.h"
#include "windows-commands.h"

#include "clay-intl.h"


void
windows_hide_docks_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  Clay     *clay;
  gboolean  active;
  return_if_no_clay (clay, data);

  active = g_variant_get_boolean (value);

  if (active != CLAY_GUI_CONFIG (clay->config)->hide_docks)
    g_object_set (clay->config,
                  "hide-docks", active,
                  NULL);
}

void
windows_use_single_window_mode_cmd_callback (ClayAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  Clay     *clay;
  gboolean  active;
  return_if_no_clay (clay, data);

  active = g_variant_get_boolean (value);

  if (active != CLAY_GUI_CONFIG (clay->config)->single_window_mode)
    g_object_set (clay->config,
                  "single-window-mode", active,
                  NULL);
}

void
windows_show_tabs_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  Clay     *clay;
  gboolean  active;
  return_if_no_clay (clay, data);

  active = g_variant_get_boolean (value);

  if (active != CLAY_GUI_CONFIG (clay->config)->show_tabs)
    g_object_set (clay->config,
                  "show-tabs", active,
                  NULL);
}


void
windows_set_tabs_position_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  Clay         *clay;
  ClayPosition  position;
  return_if_no_clay (clay, data);

  position = (ClayPosition) g_variant_get_int32 (value);

  if (position != CLAY_GUI_CONFIG (clay->config)->tabs_position)
    g_object_set (clay->config,
                  "tabs-position", position,
                  NULL);
}

void
windows_show_display_next_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayDisplay      *display;
  Clay             *clay;
  gint              index;
  ClayDisplayShell *shell;
  return_if_no_display (display, data);
  return_if_no_clay (clay, data);

  index = clay_container_get_child_index (clay->displays,
                                          CLAY_OBJECT (display));
  index++;

  if (index >= clay_container_get_n_children (clay->displays))
    index = 0;

  display = CLAY_DISPLAY (clay_container_get_child_by_index (clay->displays,
                                                             index));
  shell = clay_display_get_shell (display);
  clay_display_shell_present (shell);

  /* fix cycling shortcut not working due to nothing focused to catch the key press */
  gtk_widget_grab_focus (shell->canvas);
}

void
windows_show_display_previous_cmd_callback (ClayAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  ClayDisplay      *display;
  Clay             *clay;
  gint              index;
  ClayDisplayShell *shell;
  return_if_no_display (display, data);
  return_if_no_clay (clay, data);

  index = clay_container_get_child_index (clay->displays,
                                          CLAY_OBJECT (display));
  index--;

  if (index < 0)
    index = clay_container_get_n_children (clay->displays) - 1;

  display = CLAY_DISPLAY (clay_container_get_child_by_index (clay->displays,
                                                             index));
  shell = clay_display_get_shell (display);
  clay_display_shell_present (shell);

  /* fix cycling shortcut not working due to nothing focused to catch the key press */
  gtk_widget_grab_focus (shell->canvas);
}

void
windows_show_display_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplay *display = g_object_get_data (G_OBJECT (action), "display");

  clay_display_shell_present (clay_display_get_shell (display));
}

void
windows_show_dock_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GtkWindow *dock_window = g_object_get_data (G_OBJECT (action), "dock-window");

  gtk_window_present (dock_window);
}

void
windows_open_recent_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClaySessionInfo        *info;
  ClayDialogFactoryEntry *entry;
  Clay                   *clay;
  GtkWidget              *widget;
  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  info  = g_object_get_data (G_OBJECT (action), "info");
  entry = clay_session_info_get_factory_entry (info);

  if (entry && strcmp ("clay-toolbox-window", entry->identifier) == 0 &&
      dialogs_actions_toolbox_exists (clay))
    {
      clay_message (clay,
                    G_OBJECT (action_data_get_widget (data)),
                    CLAY_MESSAGE_WARNING,
                    _("The chosen recent dock contains a toolbox. Please "
                      "close the currently open toolbox and try again."));
      return;
    }

  g_object_ref (info);

  clay_container_remove (global_recent_docks, CLAY_OBJECT (info));

  clay_dialog_factory_add_session_info (clay_dialog_factory_get_singleton (),
                                        info);

  clay_session_info_restore (info, clay_dialog_factory_get_singleton (),
                             clay_widget_get_monitor (widget));

  g_object_unref (info);
}
