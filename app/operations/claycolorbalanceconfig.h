/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycolorbalanceconfig.h
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


#define CLAY_TYPE_COLOR_BALANCE_CONFIG            (clay_color_balance_config_get_type ())
#define CLAY_COLOR_BALANCE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_BALANCE_CONFIG, ClayColorBalanceConfig))
#define CLAY_COLOR_BALANCE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_COLOR_BALANCE_CONFIG, ClayColorBalanceConfigClass))
#define CLAY_IS_COLOR_BALANCE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_BALANCE_CONFIG))
#define CLAY_IS_COLOR_BALANCE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_COLOR_BALANCE_CONFIG))
#define CLAY_COLOR_BALANCE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_COLOR_BALANCE_CONFIG, ClayColorBalanceConfigClass))


typedef struct _ClayColorBalanceConfigClass ClayColorBalanceConfigClass;

struct _ClayColorBalanceConfig
{
  ClayOperationSettings  parent_instance;

  ClayTransferMode       range;

  gdouble                cyan_red[3];
  gdouble                magenta_green[3];
  gdouble                yellow_blue[3];

  gboolean               preserve_luminosity;
};

struct _ClayColorBalanceConfigClass
{
  ClayOperationSettingsClass  parent_class;
};


GType   clay_color_balance_config_get_type    (void);

void    clay_color_balance_config_reset_range (ClayColorBalanceConfig *config);
