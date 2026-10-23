/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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


GtkWidget * clay_brush_box_new         (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing);
GtkWidget * clay_prop_brush_box_new    (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop,
                                        const gchar   *editor_id,
                                        const gchar   *editor_tooltip);
GtkWidget * clay_dynamics_box_new      (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing);
GtkWidget * clay_prop_dynamics_box_new (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop,
                                        const gchar   *editor_id,
                                        const gchar   *editor_tooltip);

GtkWidget * clay_mybrush_box_new       (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing);
GtkWidget * clay_prop_mybrush_box_new  (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop);

GtkWidget * clay_pattern_box_new       (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing);
GtkWidget * clay_prop_pattern_box_new  (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop);

GtkWidget * clay_gradient_box_new      (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           scacing,
                                        const gchar   *reverse_prop,
                                        const gchar   *blend_color_space_prop);
GtkWidget * clay_prop_gradient_box_new (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           scacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop,
                                        const gchar   *reverse_prop,
                                        const gchar   *blend_color_space_prop,
                                        const gchar   *editor_id,
                                        const gchar   *editor_tooltip);

GtkWidget * clay_palette_box_new       (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing);
GtkWidget * clay_prop_palette_box_new  (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop,
                                        const gchar   *editor_id,
                                        const gchar   *editor_tooltip);

GtkWidget * clay_font_box_new          (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing);
GtkWidget * clay_prop_font_box_new     (ClayContainer *container,
                                        ClayContext   *context,
                                        const gchar   *label,
                                        gint           spacing,
                                        const gchar   *view_type_prop,
                                        const gchar   *view_size_prop);
