/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationsettings.h
 * Copyright (C) 2020 Ell
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "core/claysettings.h"


#define CLAY_TYPE_OPERATION_SETTINGS            (clay_operation_settings_get_type ())
#define CLAY_OPERATION_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_SETTINGS, ClayOperationSettings))
#define CLAY_OPERATION_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_SETTINGS, ClayOperationSettingsClass))
#define CLAY_IS_OPERATION_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_SETTINGS))
#define CLAY_IS_OPERATION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_SETTINGS))
#define CLAY_OPERATION_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_SETTINGS, ClayOperationSettingsClass))


typedef struct _ClayOperationSettingsClass ClayOperationSettingsClass;

struct _ClayOperationSettings
{
  ClaySettings         parent_instance;

  ClayTransformResize  clip;
  ClayFilterRegion     region;
  ClayLayerMode        mode;
  gdouble              opacity;
};

struct _ClayOperationSettingsClass
{
  ClaySettingsClass  parent_class;
};


GType      clay_operation_settings_get_type              (void);

void       clay_operation_settings_sync_drawable_filter  (ClayOperationSettings *settings,
                                                          ClayDrawableFilter    *filter);


/*  protected  */

gboolean   clay_operation_settings_config_serialize_base (ClayConfig            *config,
                                                          ClayConfigWriter      *writer,
                                                          gpointer               data);
gboolean   clay_operation_settings_config_equal_base     (ClayConfig            *a,
                                                          ClayConfig            *b);
void       clay_operation_settings_config_reset_base     (ClayConfig            *config);
gboolean   clay_operation_settings_config_copy_base      (ClayConfig            *src,
                                                          ClayConfig            *dest,
                                                          GParamFlags            flags);
