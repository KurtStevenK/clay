/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymeter.h
 * Copyright (C) 2017 Ell
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


#define CLAY_TYPE_METER            (clay_meter_get_type ())
#define CLAY_METER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_METER, ClayMeter))
#define CLAY_METER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_METER, ClayMeterClass))
#define CLAY_IS_METER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_METER))
#define CLAY_IS_METER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_METER))
#define CLAY_METER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_METER, ClayMeterClass))


typedef struct _ClayMeterPrivate ClayMeterPrivate;
typedef struct _ClayMeterClass   ClayMeterClass;

struct _ClayMeter
{
  GtkWidget         parent_instance;

  ClayMeterPrivate *priv;
};

struct _ClayMeterClass
{
  GtkWidgetClass  parent_class;
};


GType                   clay_meter_get_type                  (void);

GtkWidget             * clay_meter_new                       (Clay                  *clay,
                                                              gint                   n_values);

void                    clay_meter_set_size                  (ClayMeter             *meter,
                                                              gint                   size);
gint                    clay_meter_get_size                  (ClayMeter             *meter);

void                    clay_meter_set_refresh_rate          (ClayMeter             *meter,
                                                              gdouble                rate);
gdouble                 clay_meter_get_refresh_rate          (ClayMeter             *meter);

void                    clay_meter_set_range                 (ClayMeter             *meter,
                                                              gdouble                min,
                                                              gdouble                max);
gdouble                 clay_meter_get_range_min             (ClayMeter             *meter);
gdouble                 clay_meter_get_range_max             (ClayMeter             *meter);

void                    clay_meter_set_n_values              (ClayMeter             *meter,
                                                              gint                   n_values);
gint                    clay_meter_get_n_values              (ClayMeter             *meter);

void                    clay_meter_set_value_active          (ClayMeter             *meter,
                                                              gint                   value,
                                                              gboolean               active);
gboolean                clay_meter_get_value_active          (ClayMeter             *meter,
                                                              gint                   value);

void                    clay_meter_set_value_show_in_gauge    (ClayMeter            *meter,
                                                               gint                  value,
                                                               gboolean              show);
gboolean                clay_meter_get_value_show_in_gauge    (ClayMeter            *meter,
                                                               gint                  value);

void                    clay_meter_set_value_show_in_history (ClayMeter             *meter,
                                                              gint                   value,
                                                              gboolean               show);
gboolean                clay_meter_get_value_show_in_history (ClayMeter             *meter,
                                                              gint                   value);

void                    clay_meter_set_value_color           (ClayMeter             *meter,
                                                              gint                   value,
                                                              GeglColor             *color);
GeglColor             * clay_meter_get_value_color           (ClayMeter             *meter,
                                                              gint                   value);

void                    clay_meter_set_value_interpolation   (ClayMeter             *meter,
                                                              gint                   value,
                                                              ClayInterpolationType  interpolation);
ClayInterpolationType   clay_meter_get_value_interpolation   (ClayMeter             *meter,
                                                              gint                   value);

void                    clay_meter_set_history_visible       (ClayMeter             *meter,
                                                              gboolean               visible);
gboolean                clay_meter_get_history_visible       (ClayMeter             *meter);

void                    clay_meter_set_history_duration      (ClayMeter             *meter,
                                                              gdouble                duration);
gdouble                 clay_meter_get_history_duration      (ClayMeter             *meter);

void                    clay_meter_set_history_resolution    (ClayMeter             *meter,
                                                              gdouble                resolution);
gdouble                 clay_meter_get_history_resolution    (ClayMeter             *meter);

void                    clay_meter_clear_history             (ClayMeter             *meter);

void                    clay_meter_add_sample                (ClayMeter             *meter,
                                                              const gdouble         *sample);

void                    clay_meter_set_led_active            (ClayMeter             *meter,
                                                              gboolean               active);
gboolean                clay_meter_get_led_active            (ClayMeter             *meter);

void                    clay_meter_set_led_color             (ClayMeter             *meter,
                                                              GeglColor             *color);
GeglColor             * clay_meter_get_led_color             (ClayMeter             *meter);
