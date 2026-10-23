/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#include "clayview.h"


#define CLAY_TYPE_NAVIGATION_VIEW            (clay_navigation_view_get_type ())
#define CLAY_NAVIGATION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_NAVIGATION_VIEW, ClayNavigationView))
#define CLAY_NAVIGATION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_NAVIGATION_VIEW, ClayNavigationViewClass))
#define CLAY_IS_NAVIGATION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_NAVIGATION_VIEW))
#define CLAY_IS_NAVIGATION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_NAVIGATION_VIEW))
#define CLAY_NAVIGATION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_NAVIGATION_VIEW, ClayNavigationViewClass))


typedef struct _ClayNavigationViewClass  ClayNavigationViewClass;

struct _ClayNavigationViewClass
{
  ClayViewClass  parent_class;

  void (* marker_changed) (ClayNavigationView *view,
                           gdouble             center_x,
                           gdouble             center_y,
                           gdouble             width,
                           gdouble             height);
  void (* zoom)           (ClayNavigationView *view,
                           ClayZoomType        direction,
                           gdouble             delta);
  void (* scroll)         (ClayNavigationView *view,
                           GdkEventScroll     *sevent);
};


GType   clay_navigation_view_get_type     (void);

void    clay_navigation_view_set_marker   (ClayNavigationView *view,
                                           gdouble             center_x,
                                           gdouble             center_y,
                                           gdouble             width,
                                           gdouble             height,
                                           gboolean            flip_horizontally,
                                           gboolean            flip_vertically,
                                           gdouble             rotate_angle);
void    clay_navigation_view_set_canvas   (ClayNavigationView *view,
                                           gboolean            visible,
                                           gdouble             x,
                                           gdouble             y,
                                           gdouble             width,
                                           gdouble             height);
void    clay_navigation_view_set_motion_offset
                                          (ClayNavigationView *view,
                                           gint                motion_offset_x,
                                           gint                motion_offset_y);
void    clay_navigation_view_get_local_marker
                                          (ClayNavigationView *view,
                                           gint               *center_x,
                                           gint               *center_y,
                                           gint               *width,
                                           gint               *height);
void    clay_navigation_view_grab_pointer (ClayNavigationView *view,
                                           GdkEvent           *event);
