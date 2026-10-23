/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claydrawablepreview.h
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

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_DRAWABLE_PREVIEW_H__
#define __CLAY_DRAWABLE_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_DRAWABLE_PREVIEW (clay_drawable_preview_get_type ())
G_DECLARE_FINAL_TYPE (ClayDrawablePreview, clay_drawable_preview, CLAY, DRAWABLE_PREVIEW, ClayScrolledPreview)


GtkWidget    * clay_drawable_preview_new_from_drawable (ClayDrawable        *drawable);
ClayDrawable * clay_drawable_preview_get_drawable      (ClayDrawablePreview *preview);

/*  for internal use only  */
G_GNUC_INTERNAL void      _clay_drawable_preview_area_draw_thumb (ClayPreviewArea *area,
                                                                  ClayDrawable    *drawable,
                                                                  gint             width,
                                                                  gint             height);
G_GNUC_INTERNAL gboolean  _clay_drawable_preview_get_bounds      (ClayDrawable    *drawable,
                                                                  gint            *xmin,
                                                                  gint            *ymin,
                                                                  gint            *xmax,
                                                                  gint            *ymax);


G_END_DECLS

#endif /* __CLAY_DRAWABLE_PREVIEW_H__ */

