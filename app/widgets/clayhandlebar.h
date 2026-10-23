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


#define CLAY_TYPE_HANDLE_BAR            (clay_handle_bar_get_type ())
#define CLAY_HANDLE_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_HANDLE_BAR, ClayHandleBar))
#define CLAY_HANDLE_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_HANDLE_BAR, ClayHandleBarClass))
#define CLAY_IS_HANDLE_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_HANDLE_BAR))
#define CLAY_IS_HANDLE_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_HANDLE_BAR))
#define CLAY_HANDLE_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_HANDLE_BAR, ClayHandleBarClass))


typedef struct _ClayHandleBarClass  ClayHandleBarClass;

struct _ClayHandleBar
{
  GtkEventBox     parent_class;

  GtkOrientation  orientation;

  GtkAdjustment  *slider_adj[3];
  gboolean        limits_set;
  gdouble         lower;
  gdouble         upper;

  gint            slider_pos[3];
  gint            active_slider;
};

struct _ClayHandleBarClass
{
  GtkEventBoxClass   parent_class;
};


GType       clay_handle_bar_get_type       (void);

GtkWidget * clay_handle_bar_new            (GtkOrientation  orientation);

void        clay_handle_bar_set_adjustment (ClayHandleBar  *bar,
                                            gint            handle_no,
                                            GtkAdjustment  *adjustment);

void        clay_handle_bar_set_limits     (ClayHandleBar  *bar,
                                            gdouble         lower,
                                            gdouble         upper);
void        clay_handle_bar_unset_limits   (ClayHandleBar  *bar);
gboolean    clay_handle_bar_get_limits     (ClayHandleBar  *bar,
                                            gdouble        *lower,
                                            gdouble        *upper);

void        clay_handle_bar_connect_events (ClayHandleBar  *bar,
                                            GtkWidget      *event_source);
