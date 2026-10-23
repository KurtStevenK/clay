/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycurvesconfig.h
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


#define CLAY_TYPE_CURVES_CONFIG            (clay_curves_config_get_type ())
#define CLAY_CURVES_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CURVES_CONFIG, ClayCurvesConfig))
#define CLAY_CURVES_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_CURVES_CONFIG, ClayCurvesConfigClass))
#define CLAY_IS_CURVES_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CURVES_CONFIG))
#define CLAY_IS_CURVES_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_CURVES_CONFIG))
#define CLAY_CURVES_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_CURVES_CONFIG, ClayCurvesConfigClass))


typedef struct _ClayCurvesConfigClass ClayCurvesConfigClass;

struct _ClayCurvesConfig
{
  ClayOperationSettings  parent_instance;

  ClayTRCType            trc;

  ClayHistogramChannel   channel;

  ClayCurve              *curve[5];
};

struct _ClayCurvesConfigClass
{
  ClayOperationSettingsClass  parent_class;
};


GType      clay_curves_config_get_type            (void);

GObject  * clay_curves_config_new_spline          (gint32             channel,
                                                   const gdouble     *points,
                                                   gint               n_points);
GObject *  clay_curves_config_new_explicit        (gint32             channel,
                                                   const gdouble     *samples,
                                                   gint               n_samples);

GObject  * clay_curves_config_new_spline_cruft    (gint32             channel,
                                                   const guint8      *points,
                                                   gint               n_points);
GObject *  clay_curves_config_new_explicit_cruft  (gint32             channel,
                                                   const guint8      *samples,
                                                   gint               n_samples);

void       clay_curves_config_reset_channel       (ClayCurvesConfig  *config);

gboolean   clay_curves_config_load_cruft          (ClayCurvesConfig  *config,
                                                   GInputStream      *input,
                                                   GError           **error);
gboolean   clay_curves_config_save_cruft          (ClayCurvesConfig  *config,
                                                   GOutputStream     *output,
                                                   GError           **error);

gboolean   clay_curves_config_load_acv            (ClayCurvesConfig  *config,
                                                   GInputStream      *input,
                                                   GError           **error);
