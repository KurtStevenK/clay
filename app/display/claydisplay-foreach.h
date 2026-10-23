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


gboolean        clay_displays_dirty               (Clay      *clay);
ClayContainer * clay_displays_get_dirty_images    (Clay      *clay);
void            clay_displays_delete              (Clay      *clay);
void            clay_displays_close               (Clay      *clay);
void            clay_displays_reconnect           (Clay      *clay,
                                                   ClayImage *old,
                                                   ClayImage *new);

gint            clay_displays_get_num_visible     (Clay      *clay);

void            clay_displays_set_busy            (Clay      *clay);
void            clay_displays_unset_busy          (Clay      *clay);
gboolean        clay_displays_accept_focus_events (Clay      *clay);
