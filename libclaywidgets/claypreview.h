/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypreview.h
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

#ifndef __CLAY_PREVIEW_H__
#define __CLAY_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PREVIEW (clay_preview_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayPreview, clay_preview, CLAY, PREVIEW, GtkBox)

struct _ClayPreviewClass
{
  GtkBoxClass  parent_class;

  /* virtual methods */
  void   (* draw)        (ClayPreview     *preview);
  void   (* draw_thumb)  (ClayPreview     *preview,
                          ClayPreviewArea *area,
                          gint             width,
                          gint             height);
  void   (* draw_buffer) (ClayPreview     *preview,
                          const guchar    *buffer,
                          gint             rowstride);
  void   (* set_cursor)  (ClayPreview     *preview);

  void   (* transform)   (ClayPreview     *preview,
                          gint             src_x,
                          gint             src_y,
                          gint            *dest_x,
                          gint            *dest_y);
  void   (* untransform) (ClayPreview     *preview,
                          gint             src_x,
                          gint             src_y,
                          gint            *dest_x,
                          gint            *dest_y);

  /* signal */
  void   (* invalidated) (ClayPreview     *preview);

  /* Padding for future expansion */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


void        clay_preview_set_update         (ClayPreview  *preview,
                                             gboolean      update);
gboolean    clay_preview_get_update         (ClayPreview  *preview);

void        clay_preview_set_bounds         (ClayPreview  *preview,
                                             gint          xmin,
                                             gint          ymin,
                                             gint          xmax,
                                             gint          ymax);
void        clay_preview_get_bounds         (ClayPreview  *preview,
                                             gint         *xmin,
                                             gint         *ymin,
                                             gint         *xmax,
                                             gint         *ymax);

void        clay_preview_set_size           (ClayPreview  *preview,
                                             gint          width,
                                             gint          height);
void        clay_preview_get_size           (ClayPreview  *preview,
                                             gint         *width,
                                             gint         *height);

void        clay_preview_set_offsets        (ClayPreview  *preview,
                                             gint          xoff,
                                             gint          yoff);
void        clay_preview_get_offsets        (ClayPreview  *preview,
                                             gint         *xoff,
                                             gint         *yoff);

void        clay_preview_get_position       (ClayPreview  *preview,
                                             gint         *x,
                                             gint         *y);

void        clay_preview_transform          (ClayPreview *preview,
                                             gint         src_x,
                                             gint         src_y,
                                             gint        *dest_x,
                                             gint        *dest_y);
void        clay_preview_untransform        (ClayPreview *preview,
                                             gint         src_x,
                                             gint         src_y,
                                             gint        *dest_x,
                                             gint        *dest_y);

GtkWidget * clay_preview_get_frame          (ClayPreview  *preview);
GtkWidget * clay_preview_get_grid           (ClayPreview  *preview);
GtkWidget * clay_preview_get_area           (ClayPreview  *preview);

void        clay_preview_draw               (ClayPreview  *preview);
void        clay_preview_draw_buffer        (ClayPreview  *preview,
                                             const guchar *buffer,
                                             gint          rowstride);

void        clay_preview_invalidate         (ClayPreview  *preview);

void        clay_preview_set_default_cursor (ClayPreview  *preview,
                                             GdkCursor    *cursor);
GdkCursor * clay_preview_get_default_cursor (ClayPreview  *preview);

GtkWidget * clay_preview_get_controls       (ClayPreview  *preview);


G_END_DECLS

#endif /* __CLAY_PREVIEW_H__ */
