/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


#define CLAY_TYPE_HISTOGRAM_VIEW            (clay_histogram_view_get_type ())
#define CLAY_HISTOGRAM_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_HISTOGRAM_VIEW, ClayHistogramView))
#define CLAY_HISTOGRAM_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_HISTOGRAM_VIEW, ClayHistogramViewClass))
#define CLAY_IS_HISTOGRAM_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_HISTOGRAM_VIEW))
#define CLAY_IS_HISTOGRAM_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_HISTOGRAM_VIEW))
#define CLAY_HISTOGRAM_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_HISTOGRAM_VIEW, ClayHistogramViewClass))


typedef struct _ClayHistogramViewClass  ClayHistogramViewClass;

struct _ClayHistogramView
{
  GtkDrawingArea         parent_instance;

  ClayHistogram         *histogram;
  ClayHistogram         *bg_histogram;
  ClayHistogramChannel   channel;
  ClayHistogramScale     scale;
  gint                   n_bins;
  gint                   start;
  gint                   end;

  gint                   border_width;
  gint                   subdivisions;

  GdkSeat               *grab_seat;
};

struct _ClayHistogramViewClass
{
  GtkDrawingAreaClass  parent_class;

  void (* range_changed) (ClayHistogramView *view,
                          gint               start,
                          gint               end);
};


GType           clay_histogram_view_get_type       (void);

GtkWidget     * clay_histogram_view_new            (gboolean             range);

void            clay_histogram_view_set_histogram  (ClayHistogramView   *view,
                                                    ClayHistogram       *histogram);
ClayHistogram * clay_histogram_view_get_histogram  (ClayHistogramView   *view);

void            clay_histogram_view_set_background (ClayHistogramView   *view,
                                                    ClayHistogram       *histogram);
ClayHistogram * clay_histogram_view_get_background (ClayHistogramView   *view);

void            clay_histogram_view_set_channel    (ClayHistogramView   *view,
                                                    ClayHistogramChannel channel);
ClayHistogramChannel
                clay_histogram_view_get_channel    (ClayHistogramView   *view);

void            clay_histogram_view_set_scale      (ClayHistogramView   *view,
                                                    ClayHistogramScale   scale);
ClayHistogramScale
                clay_histogram_view_get_scale      (ClayHistogramView   *view);

void            clay_histogram_view_set_range      (ClayHistogramView   *view,
                                                    gint                 start,
                                                    gint                 end);
void            clay_histogram_view_get_range      (ClayHistogramView   *view,
                                                    gint                *start,
                                                    gint                *end);
