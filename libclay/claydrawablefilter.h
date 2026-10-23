/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claydrawablefilter.h
 * Copyright (C) 2024 Jehan
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

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_DRAWABLE_FILTER_H__
#define __CLAY_DRAWABLE_FILTER_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_DRAWABLE_FILTER (clay_drawable_filter_get_type ())
G_DECLARE_FINAL_TYPE (ClayDrawableFilter, clay_drawable_filter, CLAY, DRAWABLE_FILTER, GObject)


gint32                     clay_drawable_filter_get_id              (ClayDrawableFilter *filter);
ClayDrawableFilter       * clay_drawable_filter_get_by_id           (gint32              filter_id);

gboolean                   clay_drawable_filter_is_valid            (ClayDrawableFilter *filter);

void                       clay_drawable_filter_set_opacity         (ClayDrawableFilter *filter,
                                                                     gdouble             opacity);
void                       clay_drawable_filter_set_blend_mode      (ClayDrawableFilter *filter,
                                                                     ClayLayerMode       mode);
void                       clay_drawable_filter_set_aux_input       (ClayDrawableFilter *filter,
                                                                     const gchar        *input_pad_name,
                                                                     ClayDrawable       *input);

ClayDrawableFilterConfig * clay_drawable_filter_get_config          (ClayDrawableFilter *filter);

void                       clay_drawable_filter_update              (ClayDrawableFilter *filter);


G_END_DECLS

#endif /* __CLAY_DRAWABLE_FILTER_H__ */
