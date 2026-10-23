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


void   clay_display_shell_scroll                     (ClayDisplayShell *shell,
                                                      gint              x_offset,
                                                      gint              y_offset);
void   clay_display_shell_scroll_set_offset          (ClayDisplayShell *shell,
                                                      gint              offset_x,
                                                      gint              offset_y);

void   clay_display_shell_scroll_clamp_and_update    (ClayDisplayShell *shell);

void   clay_display_shell_scroll_unoverscrollify     (ClayDisplayShell *shell,
                                                      gint              in_offset_x,
                                                      gint              in_offset_y,
                                                      gint             *out_offset_x,
                                                      gint             *out_offset_y);

void   clay_display_shell_scroll_center_image_xy     (ClayDisplayShell *shell,
                                                      gdouble           image_x,
                                                      gdouble           image_y);
void   clay_display_shell_scroll_center_image        (ClayDisplayShell *shell,
                                                      gboolean          horizontally,
                                                      gboolean          vertically);
void   clay_display_shell_scroll_center_content      (ClayDisplayShell *shell,
                                                      gboolean          horizontally,
                                                      gboolean          vertically);

void   clay_display_shell_scroll_get_scaled_viewport (ClayDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void   clay_display_shell_scroll_get_viewport        (ClayDisplayShell *shell,
                                                      gdouble          *x,
                                                      gdouble          *y,
                                                      gdouble          *w,
                                                      gdouble          *h);
