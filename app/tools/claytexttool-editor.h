/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@gimp.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@gimp.org>
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


void       clay_text_tool_editor_init             (ClayTextTool        *text_tool);
void       clay_text_tool_editor_finalize         (ClayTextTool        *text_tool);

void       clay_text_tool_editor_start            (ClayTextTool        *text_tool);
void       clay_text_tool_editor_position         (ClayTextTool        *text_tool);
void       clay_text_tool_editor_halt             (ClayTextTool        *text_tool);

void       clay_text_tool_editor_button_press     (ClayTextTool        *text_tool,
                                                   gdouble              x,
                                                   gdouble              y,
                                                   ClayButtonPressType  press_type);
void       clay_text_tool_editor_button_release   (ClayTextTool        *text_tool);
void       clay_text_tool_editor_motion           (ClayTextTool        *text_tool,
                                                   gdouble              x,
                                                   gdouble              y);
gboolean   clay_text_tool_editor_key_press        (ClayTextTool        *text_tool,
                                                   GdkEventKey         *kevent);
gboolean   clay_text_tool_editor_key_release      (ClayTextTool        *text_tool,
                                                   GdkEventKey         *kevent);

void       clay_text_tool_reset_im_context        (ClayTextTool        *text_tool);
void       clay_text_tool_abort_im_context        (ClayTextTool        *text_tool);

gboolean   clay_text_tool_editor_get_cursor_rect  (ClayTextTool        *text_tool,
                                                   gboolean             overwrite,
                                                   PangoRectangle      *cursor_rect,
                                                   ClayVector2         *corners);
void       clay_text_tool_editor_update_im_cursor (ClayTextTool        *text_tool);
