/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef  __CLAY_DRAWABLE_GRADIENT_H__
#define  __CLAY_DRAWABLE_GRADIENT_H__


void         clay_drawable_gradient                    (ClayDrawable                *drawable,
                                                        ClayContext                 *context,
                                                        ClayGradient                *gradient,
                                                        GeglDistanceMetric           metric,
                                                        ClayLayerMode                paint_mode,
                                                        ClayGradientType             gradient_type,
                                                        gdouble                      opacity,
                                                        gdouble                      offset,
                                                        ClayRepeatMode               repeat,
                                                        gboolean                     reverse,
                                                        ClayGradientBlendColorSpace  blend_color_space,
                                                        gboolean                     supersample,
                                                        gint                         max_depth,
                                                        gdouble                      threshold,
                                                        gboolean                     dither,
                                                        gdouble                      startx,
                                                        gdouble                      starty,
                                                        gdouble                      endx,
                                                        gdouble                      endy,
                                                        ClayProgress                *progress);

GeglBuffer * clay_drawable_gradient_shapeburst_distmap (ClayDrawable                *drawable,
                                                        GeglDistanceMetric           metric,
                                                        const GeglRectangle         *region,
                                                        ClayProgress                *progress);

void         clay_drawable_gradient_adjust_coords      (ClayDrawable                *drawable,
                                                        ClayGradientType             gradient_type,
                                                        const GeglRectangle         *region,
                                                        gdouble                     *startx,
                                                        gdouble                     *starty,
                                                        gdouble                     *endx,
                                                        gdouble                     *endy);


#endif /* __CLAY_DRAWABLE_GRADIENT_H__ */
