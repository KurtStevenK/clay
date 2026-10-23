/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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

#include "clayobject.h"


#define CLAY_TYPE_HISTOGRAM            (clay_histogram_get_type ())
#define CLAY_HISTOGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_HISTOGRAM, ClayHistogram))
#define CLAY_HISTOGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_HISTOGRAM, ClayHistogramClass))
#define CLAY_IS_HISTOGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_HISTOGRAM))
#define CLAY_IS_HISTOGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_HISTOGRAM))
#define CLAY_HISTOGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_HISTOGRAM, ClayHistogramClass))


typedef struct _ClayHistogramPrivate ClayHistogramPrivate;
typedef struct _ClayHistogramClass   ClayHistogramClass;

struct _ClayHistogram
{
  ClayObject            parent_instance;

  ClayHistogramPrivate *priv;
};

struct _ClayHistogramClass
{
  ClayObjectClass  parent_class;
};


GType           clay_histogram_get_type        (void);

ClayHistogram * clay_histogram_new             (ClayTRCType           trc);

ClayHistogram * clay_histogram_duplicate       (ClayHistogram        *histogram);

void            clay_histogram_calculate       (ClayHistogram        *histogram,
                                                GeglBuffer           *buffer,
                                                const GeglRectangle  *buffer_rect,
                                                GeglBuffer           *mask,
                                                const GeglRectangle  *mask_rect);
ClayAsync     * clay_histogram_calculate_async (ClayHistogram        *histogram,
                                                GeglBuffer           *buffer,
                                                const GeglRectangle  *buffer_rect,
                                                GeglBuffer           *mask,
                                                const GeglRectangle  *mask_rect);

void            clay_histogram_clear_values    (ClayHistogram        *histogram,
                                                gint                  n_components);

gdouble         clay_histogram_get_maximum     (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel);
gdouble         clay_histogram_get_count       (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         clay_histogram_get_mean        (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         clay_histogram_get_median      (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         clay_histogram_get_std_dev     (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         clay_histogram_get_threshold   (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel,
                                                gint                  start,
                                                gint                  end);
gdouble         clay_histogram_get_value       (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel,
                                                gint                  bin);
gdouble         clay_histogram_get_component   (ClayHistogram        *histogram,
                                                gint                  component,
                                                gint                  bin);
gint            clay_histogram_n_components    (ClayHistogram        *histogram);
gint            clay_histogram_n_bins          (ClayHistogram        *histogram);
gboolean        clay_histogram_has_channel     (ClayHistogram        *histogram,
                                                ClayHistogramChannel  channel);
guint           clay_histogram_unique_colors   (ClayDrawable         *drawable);
