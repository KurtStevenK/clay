/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorscales.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_SCALES_H__
#define __CLAY_COLOR_SCALES_H__

#include <libclaywidgets/claycolorselector.h>

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_SCALES (clay_color_scales_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorScales, clay_color_scales, CLAY, COLOR_SCALES, ClayColorSelector)


void       clay_color_scales_set_show_rgb_u8 (ClayColorScales *scales,
                                              gboolean         show_rgb_u8);
gboolean   clay_color_scales_get_show_rgb_u8 (ClayColorScales *scales);


G_END_DECLS

#endif /* __CLAY_COLOR_SCALES_H__ */
