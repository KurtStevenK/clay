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

#pragma once


gboolean clay_display_shell_scale_revert             (ClayDisplayShell *shell);
gboolean clay_display_shell_scale_can_revert         (ClayDisplayShell *shell);
void     clay_display_shell_scale_save_revert_values (ClayDisplayShell *shell);

void     clay_display_shell_scale_set_dot_for_dot    (ClayDisplayShell *shell,
                                                      gboolean          dot_for_dot);

void     clay_display_shell_scale_get_image_size     (ClayDisplayShell *shell,
                                                      gint             *w,
                                                      gint             *h);
void     clay_display_shell_scale_get_image_bounds   (ClayDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void     clay_display_shell_scale_get_image_unrotated_bounds
                                                     (ClayDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void     clay_display_shell_scale_get_image_bounding_box
                                                     (ClayDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void     clay_display_shell_scale_get_image_unrotated_bounding_box
                                                     (ClayDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
gboolean clay_display_shell_scale_image_is_within_viewport
                                                     (ClayDisplayShell *shell,
                                                      gboolean         *horizontally,
                                                      gboolean         *vertically);

void     clay_display_shell_scale_update             (ClayDisplayShell *shell);

void     clay_display_shell_scale                    (ClayDisplayShell *shell,
                                                      ClayZoomType      zoom_type,
                                                      gdouble           scale,
                                                      ClayZoomFocus     zoom_focus);
void     clay_display_shell_scale_to_rectangle       (ClayDisplayShell *shell,
                                                      ClayZoomType      zoom_type,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height,
                                                      gboolean          resize_window);
void     clay_display_shell_scale_fit_in             (ClayDisplayShell *shell);
void     clay_display_shell_scale_fill               (ClayDisplayShell *shell);
void     clay_display_shell_scale_by_values          (ClayDisplayShell *shell,
                                                      gdouble           scale,
                                                      gint              offset_x,
                                                      gint              offset_y,
                                                      gboolean          resize_window);

void     clay_display_shell_scale_drag               (ClayDisplayShell *shell,
                                                      gdouble           start_x,
                                                      gdouble           start_y,
                                                      gdouble           delta_x,
                                                      gdouble           delta_y);

void     clay_display_shell_scale_shrink_wrap        (ClayDisplayShell *shell,
                                                      gboolean          grow_only);
void     clay_display_shell_scale_resize             (ClayDisplayShell *shell,
                                                      gboolean          resize_window,
                                                      gboolean          grow_only);
void     clay_display_shell_set_initial_scale        (ClayDisplayShell *shell,
                                                      gdouble           scale,
                                                      gint             *display_width,
                                                      gint             *display_height);

void     clay_display_shell_get_rotated_scale        (ClayDisplayShell *shell,
                                                      gdouble          *scale_x,
                                                      gdouble          *scale_y);

/*  debug API for testing  */

void  clay_display_shell_push_zoom_focus_pointer_pos (ClayDisplayShell *shell,
                                                      gint              x,
                                                      gint              y);
