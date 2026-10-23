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


void       clay_display_shell_appearance_update        (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_menubar         (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_menubar         (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_statusbar       (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_statusbar       (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_rulers          (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_rulers          (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_scrollbars      (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_scrollbars      (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_selection       (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_selection       (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_layer           (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_layer           (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_canvas          (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_canvas          (ClayDisplayShell       *shell);
void       clay_display_shell_update_show_canvas       (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_grid            (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_grid            (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_guides          (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_guides          (ClayDisplayShell       *shell);

void       clay_display_shell_set_snap_to_grid         (ClayDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   clay_display_shell_get_snap_to_grid         (ClayDisplayShell       *shell);

void       clay_display_shell_set_show_sample_points   (ClayDisplayShell       *shell,
                                                        gboolean                show);
gboolean   clay_display_shell_get_show_sample_points   (ClayDisplayShell       *shell);

void       clay_display_shell_set_snap_to_guides       (ClayDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   clay_display_shell_get_snap_to_guides       (ClayDisplayShell       *shell);

void       clay_display_shell_set_snap_to_canvas       (ClayDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   clay_display_shell_get_snap_to_canvas       (ClayDisplayShell       *shell);

void       clay_display_shell_set_snap_to_path         (ClayDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   clay_display_shell_get_snap_to_path         (ClayDisplayShell       *shell);

void       clay_display_shell_set_snap_to_bbox         (ClayDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   clay_display_shell_get_snap_to_bbox         (ClayDisplayShell       *shell);

void       clay_display_shell_set_snap_to_equidistance (ClayDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   clay_display_shell_get_snap_to_equidistance (ClayDisplayShell       *shell);

void       clay_display_shell_set_padding              (ClayDisplayShell       *shell,
                                                        ClayCanvasPaddingMode   mode,
                                                        GeglColor              *color);
void       clay_display_shell_get_padding              (ClayDisplayShell       *shell,
                                                        ClayCanvasPaddingMode  *mode,
                                                        GeglColor             **color);
void       clay_display_shell_set_padding_in_show_all  (ClayDisplayShell       *shell,
                                                        gboolean                keep);
gboolean   clay_display_shell_get_padding_in_show_all  (ClayDisplayShell       *shell);
