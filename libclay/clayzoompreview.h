/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayzoompreview.h
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
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

#ifndef __CLAY_ZOOM_PREVIEW_H__
#define __CLAY_ZOOM_PREVIEW_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_ZOOM_PREVIEW (clay_zoom_preview_get_type ())
G_DECLARE_FINAL_TYPE (ClayZoomPreview, clay_zoom_preview, CLAY, ZOOM_PREVIEW, ClayScrolledPreview)


GtkWidget     * clay_zoom_preview_new_from_drawable            (ClayDrawable    *drawable);
GtkWidget     * clay_zoom_preview_new_with_model_from_drawable (ClayDrawable    *drawable,
                                                                ClayZoomModel   *model);

guchar        * clay_zoom_preview_get_source                   (ClayZoomPreview *preview,
                                                                gint            *width,
                                                                gint            *height,
                                                                gint            *bpp);

ClayDrawable  * clay_zoom_preview_get_drawable                 (ClayZoomPreview *preview);
ClayZoomModel * clay_zoom_preview_get_model                    (ClayZoomPreview *preview);
gdouble         clay_zoom_preview_get_factor                   (ClayZoomPreview *preview);


G_END_DECLS

#endif /* __CLAY_ZOOM_PREVIEW_H__ */
