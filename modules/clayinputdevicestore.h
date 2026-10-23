/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayinputdevicestore.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_INPUT_DEVICE_STORE    (clay_input_device_store_get_type ())
#define CLAY_INPUT_DEVICE_STORE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_INPUT_DEVICE_STORE, ClayInputDeviceStore))
#define CLAY_IS_INPUT_DEVICE_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_INPUT_DEVICE_STORE))

typedef struct _ClayInputDeviceStore ClayInputDeviceStore;


void                   clay_input_device_store_register_types  (GTypeModule           *module);

GType                  clay_input_device_store_get_type        (void);

ClayInputDeviceStore * clay_input_device_store_new             (void);
gchar                * clay_input_device_store_get_device_file (ClayInputDeviceStore  *store,
                                                                const gchar           *udi);
GError               * clay_input_device_store_get_error       (ClayInputDeviceStore  *store);
