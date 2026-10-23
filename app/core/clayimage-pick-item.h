/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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


ClayLayer       * clay_image_pick_layer           (ClayImage *image,
                                                   gint       x,
                                                   gint       y,
                                                   ClayLayer *previously_picked);
ClayLayer       * clay_image_pick_layer_by_bounds (ClayImage *image,
                                                   gint       x,
                                                   gint       y);
ClayTextLayer   * clay_image_pick_text_layer      (ClayImage *image,
                                                   gint       x,
                                                   gint       y);

ClayPath        * clay_image_pick_path            (ClayImage *image,
                                                   gdouble    x,
                                                   gdouble    y,
                                                   gdouble    epsilon_x,
                                                   gdouble    epsilon_y);

ClayGuide       * clay_image_pick_guide           (ClayImage *image,
                                                   gdouble    x,
                                                   gdouble    y,
                                                   gdouble    epsilon_x,
                                                   gdouble    epsilon_y);
GList           * clay_image_pick_guides          (ClayImage *image,
                                                   gdouble    x,
                                                   gdouble    y,
                                                   gdouble    epsilon_x,
                                                   gdouble    epsilon_y);

ClaySamplePoint * clay_image_pick_sample_point    (ClayImage *image,
                                                   gdouble    x,
                                                   gdouble    y,
                                                   gdouble    epsilon_x,
                                                   gdouble    epsilon_y);
