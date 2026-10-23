/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycircle.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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


#define CLAY_TYPE_CIRCLE (clay_circle_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCircle,
                          clay_circle,
                          CLAY, CIRCLE,
                          GtkWidget)


struct _ClayCircleClass
{
  GtkWidgetClass  parent_class;

  void (* reset_target) (ClayCircle *circle);
};


GtkWidget * clay_circle_new                     (void);

gboolean    _clay_circle_has_grab               (ClayCircle *circle);
gdouble     _clay_circle_get_angle_and_distance (ClayCircle *circle,
                                                 gdouble     event_x,
                                                 gdouble     event_y,
                                                 gdouble    *distance);
