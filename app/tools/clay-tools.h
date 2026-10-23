/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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


void       clay_tools_init        (Clay              *clay);
void       clay_tools_exit        (Clay              *clay);

void       clay_tools_restore     (Clay              *clay);
void       clay_tools_save        (Clay              *clay,
                                   gboolean           save_tool_options,
                                   gboolean           always_save);

gboolean   clay_tools_clear       (Clay              *clay,
                                   GError           **error);

gboolean   clay_tools_serialize   (Clay              *clay,
                                   ClayContainer     *container,
                                   ClayConfigWriter  *writer);
gboolean   clay_tools_deserialize (Clay              *clay,
                                   ClayContainer     *container,
                                   GScanner          *scanner);

void       clay_tools_reset       (Clay              *clay,
                                   ClayContainer     *container,
                                   gboolean           user_toolrc);
