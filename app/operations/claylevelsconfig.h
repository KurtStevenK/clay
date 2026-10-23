/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylevelsconfig.h
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


#define CLAY_TYPE_LEVELS_CONFIG            (clay_levels_config_get_type ())
#define CLAY_LEVELS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LEVELS_CONFIG, ClayLevelsConfig))
#define CLAY_LEVELS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_LEVELS_CONFIG, ClayLevelsConfigClass))
#define CLAY_IS_LEVELS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LEVELS_CONFIG))
#define CLAY_IS_LEVELS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_LEVELS_CONFIG))
#define CLAY_LEVELS_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_LEVELS_CONFIG, ClayLevelsConfigClass))


typedef struct _ClayLevelsConfigClass ClayLevelsConfigClass;

struct _ClayLevelsConfig
{
  ClayOperationSettings  parent_instance;

  ClayTRCType            trc;

  ClayHistogramChannel   channel;

  gdouble                low_input[5];
  gdouble                high_input[5];

  gboolean               clamp_input;

  gdouble                gamma[5];

  gdouble                low_output[5];
  gdouble                high_output[5];

  gboolean               clamp_output;
};

struct _ClayLevelsConfigClass
{
  ClayOperationSettingsClass  parent_class;
};


GType      clay_levels_config_get_type         (void);

void       clay_levels_config_reset_channel    (ClayLevelsConfig      *config);

void       clay_levels_config_stretch          (ClayLevelsConfig      *config,
                                                ClayHistogram         *histogram,
                                                gboolean               is_color);
void       clay_levels_config_stretch_channel  (ClayLevelsConfig      *config,
                                                ClayHistogram         *histogram,
                                                ClayHistogramChannel   channel);
void       clay_levels_config_adjust_by_colors (ClayLevelsConfig      *config,
                                                ClayHistogramChannel   channel,
                                                const Babl            *target_space,
                                                GeglColor             *black,
                                                GeglColor             *gray,
                                                GeglColor             *white);

ClayCurvesConfig *
           clay_levels_config_to_curves_config (ClayLevelsConfig      *config);

gboolean   clay_levels_config_load_cruft       (ClayLevelsConfig      *config,
                                                GInputStream          *input,
                                                GError               **error);
gboolean   clay_levels_config_save_cruft       (ClayLevelsConfig      *config,
                                                GOutputStream         *output,
                                                GError               **error);

gboolean   clay_levels_config_load_alv         (ClayLevelsConfig      *config,
                                                GInputStream          *input,
                                                GError               **error);
