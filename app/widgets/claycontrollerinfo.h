/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycontrollerinfo.h
 * Copyright (C) 2004-2005 Michael Natterer <mitch@gimp.org>
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

#include "core/clayviewable.h"


typedef gboolean (* ClayControllerEventSnooper) (ClayControllerInfo        *info,
                                                 ClayController            *controller,
                                                 const ClayControllerEvent *event,
                                                 gpointer                   user_data);


#define CLAY_TYPE_CONTROLLER_INFO            (clay_controller_info_get_type ())
#define CLAY_CONTROLLER_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTROLLER_INFO, ClayControllerInfo))
#define CLAY_CONTROLLER_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTROLLER_INFO, ClayControllerInfoClass))
#define CLAY_IS_CONTROLLER_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTROLLER_INFO))
#define CLAY_IS_CONTROLLER_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTROLLER_INFO))
#define CLAY_CONTROLLER_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTROLLER_INFO, ClayControllerInfoClass))


typedef struct _ClayControllerInfoClass ClayControllerInfoClass;

struct _ClayControllerInfo
{
  ClayViewable                parent_instance;

  gboolean                    enabled;
  gboolean                    debug_events;

  ClayController             *controller;
  GHashTable                 *mapping;

  ClayControllerEventSnooper  snooper;
  gpointer                    snooper_data;
};

struct _ClayControllerInfoClass
{
  ClayViewableClass  parent_class;

  gboolean (* event_mapped) (ClayControllerInfo        *info,
                             ClayController            *controller,
                             const ClayControllerEvent *event,
                             const gchar               *action_name);
};


GType    clay_controller_info_get_type          (void);

ClayControllerInfo * clay_controller_info_new   (GType                       type);

void     clay_controller_info_set_enabled       (ClayControllerInfo         *info,
                                                 gboolean                    enabled);
gboolean clay_controller_info_get_enabled       (ClayControllerInfo         *info);

void     clay_controller_info_set_event_snooper (ClayControllerInfo         *info,
                                                 ClayControllerEventSnooper  snooper,
                                                 gpointer                    snooper_data);
