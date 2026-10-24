/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGuiConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include "config/claydisplayconfig.h"


#define CLAY_CONFIG_DEFAULT_THEME          "Photoshop"
#define CLAY_CONFIG_DEFAULT_ICON_THEME     "Default"


#define CLAY_TYPE_GUI_CONFIG            (clay_gui_config_get_type ())
#define CLAY_GUI_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GUI_CONFIG, ClayGuiConfig))
#define CLAY_GUI_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GUI_CONFIG, ClayGuiConfigClass))
#define CLAY_IS_GUI_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GUI_CONFIG))
#define CLAY_IS_GUI_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GUI_CONFIG))


typedef struct _ClayGuiConfigClass ClayGuiConfigClass;

struct _ClayGuiConfig
{
  ClayDisplayConfig    parent_instance;

  gboolean             edit_non_visible;
  gboolean             move_tool_changes_active;
  gint                 filter_tool_max_recent;
  gboolean             filter_tool_use_last_settings;
  gboolean             trust_dirty_flag;
  gboolean             save_device_status;
  gboolean             devices_share_tool;
  gboolean             save_session_info;
  gboolean             restore_session;
  gboolean             restore_monitor;
  gint                 startup_monitor;
  gboolean             save_tool_options;
  gboolean             save_accels;
  gboolean             restore_accels;
  gint                 last_opened_size;
  guint64              max_new_image_size;
  gboolean             toolbox_color_area;
  gboolean             toolbox_foo_area;
  gboolean             toolbox_image_area;
  gboolean             toolbox_wilber;
  gboolean             toolbox_groups;
  gchar               *theme_path;
  gchar               *theme;
  ClayThemeScheme      theme_scheme;
  gchar               *icon_theme_path;
  gchar               *icon_theme;
  gboolean             prefer_symbolic_icons;
  gdouble              font_relative_size;
  gboolean             override_icon_size;
  ClayIconSize         custom_icon_size;
  gboolean             viewables_follow_theme;
  gboolean             use_help;
  gboolean             show_help_button;
  gchar               *help_locales;
  gboolean             user_manual_online;
  gchar               *user_manual_online_uri;
  gint                 action_history_size;
  ClayWindowHint       dock_window_hint;
  ClayHandedness       cursor_handedness;
  gboolean             custom_title_bar;
  gboolean             show_welcome_dialog;

  /* experimental playground */
  gboolean             playground_npd_tool;
  gboolean             playground_seamless_clone_tool;
  gboolean             playground_paint_select_tool;
  gboolean             playground_use_list_box;

  /* saved in sessionrc */
  gboolean             hide_docks;
  gboolean             single_window_mode;
  gboolean             show_tabs;
  ClayPosition         tabs_position;
  gint                 last_tip_shown;

  /* Currently ignored since "gtk-can-change-accels" settings got deprecated in
   * GTK+ 3.10.
   */
  gboolean             can_change_accels;
};

struct _ClayGuiConfigClass
{
  ClayDisplayConfigClass  parent_class;
};


GType  clay_gui_config_get_type (void);
