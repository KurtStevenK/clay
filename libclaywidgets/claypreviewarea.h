/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __CLAY_PREVIEW_AREA_H__
#define __CLAY_PREVIEW_AREA_H__

G_BEGIN_DECLS


#define CLAY_TYPE_PREVIEW_AREA (clay_preview_area_get_type ())
G_DECLARE_FINAL_TYPE (ClayPreviewArea, clay_preview_area, CLAY, PREVIEW_AREA, GtkDrawingArea)


GtkWidget * clay_preview_area_new              (void);

void        clay_preview_area_draw             (ClayPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                ClayImageType    type,
                                                const guchar    *buf,
                                                gint             rowstride);
void        clay_preview_area_blend            (ClayPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                ClayImageType    type,
                                                const guchar    *buf1,
                                                gint             rowstride1,
                                                const guchar    *buf2,
                                                gint             rowstride2,
                                                guchar           opacity);
void        clay_preview_area_mask             (ClayPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                ClayImageType    type,
                                                const guchar    *buf1,
                                                gint             rowstride1,
                                                const guchar    *buf2,
                                                gint             rowstride2,
                                                const guchar    *mask,
                                                gint             rowstride_mask);
void        clay_preview_area_fill             (ClayPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                guchar           red,
                                                guchar           green,
                                                guchar           blue);
void        clay_preview_area_reset            (ClayPreviewArea *area);

void        clay_preview_area_set_offsets      (ClayPreviewArea *area,
                                                gint             x,
                                                gint             y);

void        clay_preview_area_set_colormap     (ClayPreviewArea *area,
                                                const guchar    *colormap,
                                                gint             num_colors);

void        clay_preview_area_set_color_config (ClayPreviewArea *area,
                                                ClayColorConfig *config);

void        clay_preview_area_get_size         (ClayPreviewArea *area,
                                                gint            *width,
                                                gint            *height);
void        clay_preview_area_set_max_size     (ClayPreviewArea *area,
                                                gint             width,
                                                gint             height);

void        clay_preview_area_menu_popup       (ClayPreviewArea *area,
                                                GdkEventButton  *event);


G_END_DECLS

#endif /* __CLAY_PREVIEW_AREA_H__ */
