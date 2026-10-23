/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayDBusService
 * Copyright (C) 2007, 2008 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2013       Michael Natterer <mitch@gimp.org>
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

#include "claydbusservice-generated.h"


#define CLAY_DBUS_SERVICE_NAME   "xyz.kainzmayer.ClayDesign.UI"
#define CLAY_DBUS_SERVICE_PATH   "/xyz/kainzmayer/ClayDesign/UI"
#define CLAY_DBUS_INTERFACE_NAME "xyz.kainzmayer.ClayDesign.UI"
#define CLAY_DBUS_INTERFACE_PATH "/xyz/kainzmayer/ClayDesign/UI"


#define CLAY_TYPE_DBUS_SERVICE            (clay_dbus_service_get_type ())
#define CLAY_DBUS_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DBUS_SERVICE, ClayDBusService))
#define CLAY_DBUS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DBUS_SERVICE, ClayDBusServiceClass))
#define CLAY_IS_DBUS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DBUS_SERVICE))
#define CLAY_IS_DBUS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DBUS_SERVICE))
#define CLAY_DBUS_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DBUS_SERVICE, ClayDBusServiceClass))


typedef struct _ClayDBusService      ClayDBusService;
typedef struct _ClayDBusServiceClass ClayDBusServiceClass;

struct _ClayDBusService
{
  ClayDBusServiceUISkeleton  parent_instance;

  Clay     *clay;
  GQueue   *queue;
  GSource  *source;
  gboolean  timeout_source;
};

struct _ClayDBusServiceClass
{
  ClayDBusServiceUISkeletonClass  parent_class;
};


GType     clay_dbus_service_get_type (void);

GObject * clay_dbus_service_new      (Clay *clay);
