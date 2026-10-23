/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolgui.h
 * Copyright (C) 2013  Michael Natterer <mitch@gimp.org>
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

#include "core/clayobject.h"


#define CLAY_TYPE_TOOL_GUI (clay_tool_gui_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayToolGui,
                          clay_tool_gui,
                          CLAY, TOOL_GUI,
                          ClayObject)


struct _ClayToolGuiClass
{
  ClayObjectClass  parent_class;

  void (* response) (ClayToolGui *gui,
                     gint         response_id);

};


ClayToolGui * clay_tool_gui_new                    (ClayToolInfo     *tool_info,
                                                    const gchar      *title,
                                                    const gchar      *description,
                                                    const gchar      *icon_name,
                                                    const gchar      *help_id,
                                                    GdkMonitor       *monitor,
                                                    gboolean          overlay,
                                                    ...) G_GNUC_NULL_TERMINATED;

void          clay_tool_gui_set_title              (ClayToolGui      *gui,
                                                    const gchar      *title);
void          clay_tool_gui_set_description        (ClayToolGui      *gui,
                                                    const gchar      *description);
void          clay_tool_gui_set_icon_name          (ClayToolGui      *gui,
                                                    const gchar      *icon_name);
void          clay_tool_gui_set_help_id            (ClayToolGui      *gui,
                                                    const gchar      *help_id);

void          clay_tool_gui_set_shell              (ClayToolGui      *gui,
                                                    ClayDisplayShell *shell);
void          clay_tool_gui_set_viewables          (ClayToolGui      *gui,
                                                    GList            *viewables);
void          clay_tool_gui_set_viewable           (ClayToolGui      *gui,
                                                    ClayViewable     *viewable);

GtkWidget   * clay_tool_gui_get_dialog             (ClayToolGui      *gui);
GtkWidget   * clay_tool_gui_get_vbox               (ClayToolGui      *gui);

gboolean      clay_tool_gui_get_visible            (ClayToolGui      *gui);
void          clay_tool_gui_show                   (ClayToolGui      *gui);
void          clay_tool_gui_hide                   (ClayToolGui      *gui);

void          clay_tool_gui_set_overlay            (ClayToolGui      *gui,
                                                    GdkMonitor       *monitor,
                                                    gboolean          overlay);
gboolean      clay_tool_gui_get_overlay            (ClayToolGui      *gui);

void          clay_tool_gui_set_auto_overlay       (ClayToolGui      *gui,
                                                    gboolean          auto_overlay);
gboolean      clay_tool_gui_get_auto_overlay       (ClayToolGui      *gui);

void          clay_tool_gui_set_focus_on_map       (ClayToolGui      *gui,
                                                    gboolean          focus_on_map);
gboolean      clay_tool_gui_get_focus_on_map       (ClayToolGui      *gui);


void          clay_tool_gui_add_buttons_valist     (ClayToolGui      *gui,
                                                    va_list           args);
void          clay_tool_gui_add_button             (ClayToolGui      *gui,
                                                    const gchar      *button_text,
                                                    gint              response_id);
void          clay_tool_gui_set_default_response   (ClayToolGui      *gui,
                                                    gint              response_id);
void          clay_tool_gui_set_response_sensitive (ClayToolGui      *gui,
                                                    gint              response_id,
                                                    gboolean          sensitive);
void    clay_tool_gui_set_alternative_button_order (ClayToolGui      *gui,
                                                    ...);
