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


#define CLAY_TYPE_COLOR_BAR            (clay_color_bar_get_type ())
#define CLAY_COLOR_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_BAR, ClayColorBar))
#define CLAY_COLOR_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_BAR, ClayColorBarClass))
#define CLAY_IS_COLOR_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_BAR))
#define CLAY_IS_COLOR_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_BAR))
#define CLAY_COLOR_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_BAR, ClayColorBarClass))


typedef struct _ClayColorBarClass  ClayColorBarClass;

struct _ClayColorBar
{
  GtkEventBox     parent_class;

  GtkOrientation  orientation;
  guchar          buf[3 * 256];
};

struct _ClayColorBarClass
{
  GtkEventBoxClass  parent_class;
};


GType       clay_color_bar_get_type    (void);

GtkWidget * clay_color_bar_new         (GtkOrientation        orientation);

void        clay_color_bar_set_color   (ClayColorBar         *bar,
                                        GeglColor            *color);
void        clay_color_bar_set_channel (ClayColorBar         *bar,
                                        ClayHistogramChannel  channel);
void        clay_color_bar_set_buffers (ClayColorBar         *bar,
                                        const guchar         *red,
                                        const guchar         *green,
                                        const guchar         *blue);
