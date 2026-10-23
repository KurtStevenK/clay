/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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


void      clay_data_factories_init        (Clay               *clay);
void      clay_data_factories_add_builtin (Clay               *clay);
void      clay_data_factories_clear       (Clay               *clay);
void      clay_data_factories_exit        (Clay               *clay);

gboolean  clay_data_factories_wait        (Clay               *clay);

gint64    clay_data_factories_get_memsize (Clay   *clay,
                                           gint64 *gui_size);
void      clay_data_factories_data_clean  (Clay   *clay);

void      clay_data_factories_load        (Clay               *clay,
                                           ClayInitStatusFunc  status_callback);
void      clay_data_factories_save        (Clay               *clay);
