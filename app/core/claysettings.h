/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysettings.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include "clayviewable.h"


#define CLAY_TYPE_SETTINGS            (clay_settings_get_type ())
#define CLAY_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SETTINGS, ClaySettings))
#define CLAY_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_SETTINGS, ClaySettingsClass))
#define CLAY_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SETTINGS))
#define CLAY_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_SETTINGS))
#define CLAY_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_SETTINGS, ClaySettingsClass))


typedef struct _ClaySettingsClass ClaySettingsClass;

struct _ClaySettings
{
  ClayViewable  parent_instance;

  gint64        time;
};

struct _ClaySettingsClass
{
  ClayViewableClass  parent_class;
};


GType   clay_settings_get_type (void);

gint    clay_settings_compare  (ClaySettings *a,
                                ClaySettings *b);
