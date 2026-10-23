/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydevicemanager.h
 * Copyright (C) 2011 Michael Natterer
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

#include "core/claylist.h"


#define CLAY_TYPE_DEVICE_MANAGER            (clay_device_manager_get_type ())
#define CLAY_DEVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DEVICE_MANAGER, ClayDeviceManager))
#define CLAY_DEVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DEVICE_MANAGER, ClayDeviceManagerClass))
#define CLAY_IS_DEVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DEVICE_MANAGER))
#define CLAY_IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DEVICE_MANAGER))
#define CLAY_DEVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DEVICE_MANAGER, ClayDeviceManagerClass))


typedef struct _ClayDeviceManagerPrivate ClayDeviceManagerPrivate;
typedef struct _ClayDeviceManagerClass   ClayDeviceManagerClass;

struct _ClayDeviceManager
{
  ClayList                  parent_instance;

  ClayDeviceManagerPrivate *priv;
};

struct _ClayDeviceManagerClass
{
  ClayListClass  parent_class;
};


GType               clay_device_manager_get_type           (void);

ClayDeviceManager * clay_device_manager_new                (Clay *clay);

ClayDeviceInfo    * clay_device_manager_get_current_device (ClayDeviceManager *manager);
void                clay_device_manager_set_current_device (ClayDeviceManager *manager,
                                                            ClayDeviceInfo    *info);

void                clay_device_manager_reset              (ClayDeviceManager *manager);

void                clay_device_manager_reconfigure_pads   (ClayDeviceManager *manager);
