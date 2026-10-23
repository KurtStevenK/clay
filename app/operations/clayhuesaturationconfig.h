/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayhuesaturationconfig.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "clayoperationsettings.h"


#define CLAY_TYPE_HUE_SATURATION_CONFIG            (clay_hue_saturation_config_get_type ())
#define CLAY_HUE_SATURATION_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_HUE_SATURATION_CONFIG, ClayHueSaturationConfig))
#define CLAY_HUE_SATURATION_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_HUE_SATURATION_CONFIG, ClayHueSaturationConfigClass))
#define CLAY_IS_HUE_SATURATION_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_HUE_SATURATION_CONFIG))
#define CLAY_IS_HUE_SATURATION_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_HUE_SATURATION_CONFIG))
#define CLAY_HUE_SATURATION_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_HUE_SATURATION_CONFIG, ClayHueSaturationConfigClass))


typedef struct _ClayHueSaturationConfigClass ClayHueSaturationConfigClass;

struct _ClayHueSaturationConfig
{
  ClayOperationSettings  parent_instance;

  ClayHueRange           range;

  gdouble                hue[7];
  gdouble                saturation[7];
  gdouble                lightness[7];

  gdouble                overlap;
};

struct _ClayHueSaturationConfigClass
{
  ClayOperationSettingsClass  parent_class;
};


GType   clay_hue_saturation_config_get_type    (void);

void    clay_hue_saturation_config_reset_range (ClayHueSaturationConfig *config);
