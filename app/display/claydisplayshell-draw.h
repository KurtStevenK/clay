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


void   clay_display_shell_draw_selection_out (ClayDisplayShell   *shell,
                                              cairo_t            *cr,
                                              ClaySegment        *segs,
                                              gint                n_segs);
void   clay_display_shell_draw_selection_in  (ClayDisplayShell   *shell,
                                              cairo_t            *cr,
                                              cairo_pattern_t    *mask,
                                              gint                index);

void   clay_display_shell_draw_background    (ClayDisplayShell   *shell,
                                              cairo_t            *cr);
void   clay_display_shell_draw_checkerboard  (ClayDisplayShell   *shell,
                                              cairo_t            *cr);
void   clay_display_shell_draw_image         (ClayDisplayShell   *shell,
                                              cairo_t            *cr,
                                              gint                x,
                                              gint                y,
                                              gint                w,
                                              gint                h);
