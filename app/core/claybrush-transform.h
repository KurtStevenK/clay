/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrush-transform.h
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


/*  virtual functions of ClayBrush, don't call directly  */

void          clay_brush_real_transform_size   (ClayBrush   *brush,
                                                gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                gint        *scaled_width,
                                                gint        *scaled_height);
ClayTempBuf * clay_brush_real_transform_mask   (ClayBrush   *brush,
                                                gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                gdouble      hardness);
ClayTempBuf * clay_brush_real_transform_pixmap (ClayBrush   *brush,
                                                gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                gdouble      hardness);

void          clay_brush_transform_get_scale   (gdouble      scale,
                                                gdouble      aspect_ratio,
                                                gdouble     *scale_x,
                                                gdouble     *scale_y);
void          clay_brush_transform_matrix      (gdouble      width,
                                                gdouble      height,
                                                gdouble      scale_x,
                                                gdouble      scale_y,
                                                gdouble      angle,
                                                gboolean     reflect,
                                                ClayMatrix3 *matrix);
