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


void                clay_devices_init           (Clay       *clay);
void                clay_devices_exit           (Clay       *clay);

void                clay_devices_restore        (Clay       *clay);
void                clay_devices_save           (Clay       *clay,
                                                 gboolean    always_save);

gboolean            clay_devices_clear          (Clay       *clay,
                                                 GError    **error);

ClayDeviceManager * clay_devices_get_manager    (Clay       *clay);

GdkDevice         * clay_devices_get_from_event (Clay            *clay,
                                                 const GdkEvent  *event,
                                                 GdkDevice      **grab_device);

void                clay_devices_add_widget     (Clay       *clay,
                                                 GtkWidget  *widget);

gboolean            clay_devices_check_callback (GtkWidget  *widget,
                                                 GdkEvent   *event,
                                                 Clay       *clay);
gboolean            clay_devices_check_change   (Clay       *clay,
                                                 GdkDevice  *device);
