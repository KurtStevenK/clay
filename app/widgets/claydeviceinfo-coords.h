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


gboolean clay_device_info_get_event_coords   (ClayDeviceInfo  *info,
                                              GdkWindow       *window,
                                              const GdkEvent  *event,
                                              ClayCoords      *coords);
void     clay_device_info_get_device_coords  (ClayDeviceInfo  *info,
                                              GdkWindow       *window,
                                              ClayCoords      *coords);

void     clay_device_info_get_time_coords    (ClayDeviceInfo  *info,
                                              GdkTimeCoord    *event,
                                              ClayCoords      *coords);

gboolean clay_device_info_get_event_state    (ClayDeviceInfo  *info,
                                              GdkWindow       *window,
                                              const GdkEvent  *event,
                                              GdkModifierType *state);
void     clay_device_info_get_device_state   (ClayDeviceInfo  *info,
                                              GdkWindow       *window,
                                              GdkModifierType *state);
