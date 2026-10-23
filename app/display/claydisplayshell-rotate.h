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


void   clay_display_shell_flip                    (ClayDisplayShell *shell,
                                                   gboolean          flip_horizontally,
                                                   gboolean          flip_vertically);

void   clay_display_shell_rotate                  (ClayDisplayShell *shell,
                                                   gdouble           delta);
void   clay_display_shell_rotate_to               (ClayDisplayShell *shell,
                                                   gdouble           value);
void   clay_display_shell_rotate_drag             (ClayDisplayShell *shell,
                                                   gdouble           last_x,
                                                   gdouble           last_y,
                                                   gdouble           cur_x,
                                                   gdouble           cur_y,
                                                   gboolean          constrain);

void   clay_display_shell_rotate_update_transform (ClayDisplayShell *shell);
