/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydevicestatus.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "clayeditor.h"


#define CLAY_TYPE_DEVICE_STATUS            (clay_device_status_get_type ())
#define CLAY_DEVICE_STATUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DEVICE_STATUS, ClayDeviceStatus))
#define CLAY_DEVICE_STATUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DEVICE_STATUS, ClayDeviceStatusClass))
#define CLAY_IS_DEVICE_STATUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DEVICE_STATUS))
#define CLAY_IS_DEVICE_STATUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DEVICE_STATUS))
#define CLAY_DEVICE_STATUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DEVICE_STATUS, ClayDeviceStatusClass))


typedef struct _ClayDeviceStatusEntry ClayDeviceStatusEntry;
typedef struct _ClayDeviceStatusClass ClayDeviceStatusClass;

struct _ClayDeviceStatus
{
  ClayEditor      parent_instance;

  Clay           *clay;
  ClayDeviceInfo *current_device;

  GList          *devices;

  GtkWidget      *vbox;

  GtkWidget      *save_button;
  GtkWidget      *edit_button;
};

struct _ClayDeviceStatusClass
{
  ClayEditorClass  parent_class;
};


GType       clay_device_status_get_type (void);

GtkWidget * clay_device_status_new      (Clay *clay);
