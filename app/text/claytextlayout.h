/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_TEXT_LAYOUT    (clay_text_layout_get_type ())
#define CLAY_TEXT_LAYOUT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_LAYOUT, ClayTextLayout))
#define CLAY_IS_TEXT_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_LAYOUT))


typedef struct _ClayTextLayoutClass ClayTextLayoutClass;

struct _ClayTextLayoutClass
{
  GObjectClass   parent_class;
};


GType            clay_text_layout_get_type             (void);

ClayTextLayout * clay_text_layout_new                  (ClayText       *text,
                                                        ClayImage      *target_image,
                                                        gdouble         xres,
                                                        gdouble         yres,
                                                        GError        **error);

const Babl     * clay_text_layout_get_space            (ClayTextLayout *layout);
const ClayTRCType clay_text_layout_get_trc             (ClayTextLayout *layout);
const Babl *     clay_text_layout_get_format           (ClayTextLayout *layout,
                                                        const gchar    *babl_type);

gboolean         clay_text_layout_get_size             (ClayTextLayout *layout,
                                                        gint           *width,
                                                        gint           *height);
void             clay_text_layout_get_offsets          (ClayTextLayout *layout,
                                                        gint           *x,
                                                        gint           *y);
void             clay_text_layout_get_resolution       (ClayTextLayout *layout,
                                                        gdouble        *xres,
                                                        gdouble        *yres);

ClayText       * clay_text_layout_get_text             (ClayTextLayout *layout);
PangoLayout    * clay_text_layout_get_pango_layout     (ClayTextLayout *layout);

void             clay_text_layout_get_transform        (ClayTextLayout *layout,
                                                        cairo_matrix_t *matrix);

gboolean         clay_text_layout_is_transformed       (ClayTextLayout *layout);
void             clay_text_layout_transform_quad       (ClayTextLayout *layout,
                                                        const PangoRectangle
                                                                       *rect,
                                                        ClayVector2    *corners);
void             clay_text_layout_transform_rect       (ClayTextLayout *layout,
                                                        PangoRectangle *rect);
void             clay_text_layout_transform_point      (ClayTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
void             clay_text_layout_transform_distance   (ClayTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);

void             clay_text_layout_untransform_rect     (ClayTextLayout *layout,
                                                        PangoRectangle *rect);
void             clay_text_layout_untransform_point    (ClayTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
void             clay_text_layout_untransform_distance (ClayTextLayout *layout,
                                                        gdouble        *x,
                                                        gdouble        *y);
