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

#pragma once


void       tool_manager_init                       (Clay             *clay);
void       tool_manager_exit                       (Clay             *clay);

ClayTool * tool_manager_get_active                 (Clay             *clay);

void       tool_manager_push_tool                  (Clay             *clay,
                                                    ClayTool         *tool);
void       tool_manager_pop_tool                   (Clay             *clay);

void       tool_manager_swap_tools                 (Clay             *clay,
                                                    gboolean          block_initialization);

gboolean   tool_manager_initialize_active          (Clay             *clay,
                                                    ClayDisplay      *display);
void       tool_manager_control_active             (Clay             *clay,
                                                    ClayToolAction    action,
                                                    ClayDisplay      *display);
void       tool_manager_button_press_active        (Clay             *clay,
                                                    const ClayCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    ClayButtonPressType press_type,
                                                    ClayDisplay      *display);
void       tool_manager_button_release_active      (Clay             *clay,
                                                    const ClayCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    ClayDisplay      *display);
void       tool_manager_motion_active              (Clay             *clay,
                                                    const ClayCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    ClayDisplay      *display);
gboolean   tool_manager_key_press_active           (Clay             *clay,
                                                    GdkEventKey      *kevent,
                                                    ClayDisplay      *display);
gboolean   tool_manager_key_release_active         (Clay             *clay,
                                                    GdkEventKey      *kevent,
                                                    ClayDisplay      *display);

void       tool_manager_focus_display_active       (Clay             *clay,
                                                    ClayDisplay      *display);
void       tool_manager_modifier_state_active      (Clay             *clay,
                                                    GdkModifierType   state,
                                                    ClayDisplay      *display);

void     tool_manager_active_modifier_state_active (Clay             *clay,
                                                    GdkModifierType   state,
                                                    ClayDisplay      *display);

void       tool_manager_oper_update_active         (Clay             *clay,
                                                    const ClayCoords *coords,
                                                    GdkModifierType   state,
                                                    gboolean          proximity,
                                                    ClayDisplay      *display);
void       tool_manager_cursor_update_active       (Clay             *clay,
                                                    const ClayCoords *coords,
                                                    GdkModifierType   state,
                                                    ClayDisplay      *display);

const gchar   * tool_manager_can_undo_active       (Clay             *clay,
                                                    ClayDisplay      *display);
const gchar   * tool_manager_can_redo_active       (Clay             *clay,
                                                    ClayDisplay      *display);
gboolean        tool_manager_undo_active           (Clay             *clay,
                                                    ClayDisplay      *display);
gboolean        tool_manager_redo_active           (Clay             *clay,
                                                    ClayDisplay      *display);

ClayUIManager * tool_manager_get_popup_active      (Clay             *clay,
                                                    const ClayCoords *coords,
                                                    GdkModifierType   state,
                                                    ClayDisplay      *display,
                                                    const gchar     **ui_path);
