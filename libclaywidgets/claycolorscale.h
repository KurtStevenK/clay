/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorscale.h
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_SCALE_H__
#define __CLAY_COLOR_SCALE_H__

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_SCALE (clay_color_scale_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorScale, clay_color_scale, CLAY, COLOR_SCALE, GtkScale)


GtkWidget * clay_color_scale_new              (GtkOrientation            orientation,
                                               ClayColorSelectorChannel  channel);

void        clay_color_scale_set_format       (ClayColorScale           *scale,
                                               const Babl               *format);
void        clay_color_scale_set_channel      (ClayColorScale           *scale,
                                               ClayColorSelectorChannel  channel);
void        clay_color_scale_set_color        (ClayColorScale           *scale,
                                               GeglColor                *color);

void        clay_color_scale_set_color_config (ClayColorScale           *scale,
                                               ClayColorConfig          *config);


G_END_DECLS

#endif /* __CLAY_COLOR_SCALE_H__ */
