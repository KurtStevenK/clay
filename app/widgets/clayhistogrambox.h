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


#define CLAY_TYPE_HISTOGRAM_BOX            (clay_histogram_box_get_type ())
#define CLAY_HISTOGRAM_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_HISTOGRAM_BOX, ClayHistogramBox))
#define CLAY_HISTOGRAM_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_HISTOGRAM_BOX, ClayHistogramBoxClass))
#define CLAY_IS_HISTOGRAM_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_HISTOGRAM_BOX))
#define CLAY_IS_HISTOGRAM_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_HISTOGRAM_BOX))
#define CLAY_HISTOGRAM_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_HISTOGRAM_BOX, ClayHistogramBoxClass))


typedef struct _ClayHistogramBoxClass ClayHistogramBoxClass;

struct _ClayHistogramBox
{
  GtkBox             parent_instance;

  ClayHistogramView *view;
  GtkWidget         *color_bar;
  GtkWidget         *slider_bar;

  gint               n_bins;

  GtkAdjustment     *low_adj;
  GtkAdjustment     *high_adj;

  GtkWidget         *low_spinbutton;
  GtkWidget         *high_spinbutton;
};

struct _ClayHistogramBoxClass
{
  GtkBoxClass  parent_class;
};


GType       clay_histogram_box_get_type    (void);

GtkWidget * clay_histogram_box_new         (void);
void        clay_histogram_box_set_channel (ClayHistogramBox     *box,
                                            ClayHistogramChannel  channel);
