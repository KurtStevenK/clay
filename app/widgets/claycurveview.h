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

#include "clayhistogramview.h"


#define CLAY_TYPE_CURVE_VIEW            (clay_curve_view_get_type ())
#define CLAY_CURVE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CURVE_VIEW, ClayCurveView))
#define CLAY_CURVE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CURVE_VIEW, ClayCurveViewClass))
#define CLAY_IS_CURVE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CURVE_VIEW))
#define CLAY_IS_CURVE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CURVE_VIEW))
#define CLAY_CURVE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CURVE_VIEW, ClayCurveViewClass))


typedef struct _ClayCurveViewClass  ClayCurveViewClass;

struct _ClayCurveView
{
  ClayHistogramView  parent_instance;

  Clay              *clay; /* only needed for copy & paste */

  ClayCurve         *curve;
  GeglColor         *curve_color;

  GList             *bg_curves;

  gboolean           draw_base_line;
  gint               grid_rows;
  gint               grid_columns;

  gint               selected;
  gdouble            offset_x;
  gdouble            offset_y;
  ClayCurvePointType point_type;
  gdouble            last_x;
  gdouble            last_y;
  gdouble            leftmost;
  gdouble            rightmost;
  gboolean           grabbed;
  ClayCurve         *orig_curve;

  GdkCursorType      cursor_type;

  gdouble            xpos;

  PangoLayout       *layout;

  gdouble            range_x_min;
  gdouble            range_x_max;
  gdouble            range_y_min;
  gdouble            range_y_max;

  gdouble            cursor_x;
  gdouble            cursor_y;
  PangoLayout       *cursor_layout;
  PangoRectangle     cursor_rect;

  gchar             *x_axis_label;
  gchar             *y_axis_label;
};

struct _ClayCurveViewClass
{
  ClayHistogramViewClass  parent_class;

  /* signals */
  void (* selection_changed) (ClayCurveView *view);

  void (* cut_clipboard)     (ClayCurveView *view);
  void (* copy_clipboard)    (ClayCurveView *view);
  void (* paste_clipboard)   (ClayCurveView *view);
};


GType       clay_curve_view_get_type          (void);

GtkWidget * clay_curve_view_new               (void);

void        clay_curve_view_set_curve         (ClayCurveView *view,
                                               ClayCurve     *curve,
                                               GeglColor     *color);
ClayCurve * clay_curve_view_get_curve         (ClayCurveView *view);

void        clay_curve_view_add_background    (ClayCurveView *view,
                                               ClayCurve     *curve,
                                               GeglColor     *color);
void        clay_curve_view_remove_background (ClayCurveView *view,
                                               ClayCurve     *curve);

void   clay_curve_view_remove_all_backgrounds (ClayCurveView *view);

void        clay_curve_view_set_selected      (ClayCurveView *view,
                                               gint           selected);
gint        clay_curve_view_get_selected      (ClayCurveView *view);

void        clay_curve_view_set_range_x       (ClayCurveView *view,
                                               gdouble        min,
                                               gdouble        max);
void        clay_curve_view_set_range_y       (ClayCurveView *view,
                                               gdouble        min,
                                               gdouble        max);
void        clay_curve_view_set_xpos          (ClayCurveView *view,
                                               gdouble        x);

void        clay_curve_view_set_x_axis_label  (ClayCurveView *view,
                                               const gchar   *label);
void        clay_curve_view_set_y_axis_label  (ClayCurveView *view,
                                               const gchar   *label);
