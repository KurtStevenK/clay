/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorarea.h
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

/* This provides a color preview area. The preview
 * can handle transparency by showing the checkerboard and
 * handles drag'n'drop.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_AREA_H__
#define __CLAY_COLOR_AREA_H__

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_AREA (clay_color_area_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorArea, clay_color_area, CLAY, COLOR_AREA, GtkDrawingArea)


GtkWidget * clay_color_area_new              (GeglColor         *color,
                                              ClayColorAreaType  type,
                                              GdkModifierType    drag_mask);

void        clay_color_area_set_color        (ClayColorArea     *area,
                                              GeglColor         *color);
GeglColor * clay_color_area_get_color        (ClayColorArea     *area);

gboolean    clay_color_area_has_alpha        (ClayColorArea     *area);
void        clay_color_area_set_type         (ClayColorArea     *area,
                                              ClayColorAreaType  type);
void        clay_color_area_enable_drag      (ClayColorArea     *area,
                                              GdkModifierType    drag_mask);
void        clay_color_area_set_draw_border  (ClayColorArea     *area,
                                              gboolean           draw_border);
void        clay_color_area_set_out_of_gamut (ClayColorArea     *area,
                                              gboolean           out_of_gamut);

void        clay_color_area_set_color_config (ClayColorArea     *area,
                                              ClayColorConfig   *config);


G_END_DECLS

#endif /* __CLAY_COLOR_AREA_H__ */
