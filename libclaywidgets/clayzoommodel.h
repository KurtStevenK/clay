/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayzoommodel.h
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_ZOOM_MODEL_H__
#define __CLAY_ZOOM_MODEL_H__

G_BEGIN_DECLS


#define CLAY_TYPE_ZOOM_MODEL (clay_zoom_model_get_type ())
G_DECLARE_FINAL_TYPE (ClayZoomModel, clay_zoom_model, CLAY, ZOOM_MODEL, GObject)


ClayZoomModel * clay_zoom_model_new          (void);
void            clay_zoom_model_set_range    (ClayZoomModel      *model,
                                              gdouble             min,
                                              gdouble             max);
void            clay_zoom_model_zoom         (ClayZoomModel      *model,
                                              ClayZoomType        zoom_type,
                                              gdouble             scale);
gdouble         clay_zoom_model_get_factor   (ClayZoomModel      *model);
void            clay_zoom_model_get_fraction (ClayZoomModel      *model,
                                              gint               *numerator,
                                              gint               *denominator);

GtkWidget     * clay_zoom_button_new         (ClayZoomModel      *model,
                                              ClayZoomType        zoom_type,
                                              GtkIconSize         icon_size);

gdouble         clay_zoom_model_zoom_step    (ClayZoomType        zoom_type,
                                              gdouble             scale,
                                              gdouble             delta);

G_END_DECLS

#endif /* __CLAY_ZOOM_MODEL_H__ */
