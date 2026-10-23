/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydisplayshell-items.h
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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


void   clay_display_shell_items_init            (ClayDisplayShell *shell);
void   clay_display_shell_items_free            (ClayDisplayShell *shell);

void   clay_display_shell_add_item              (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);
void   clay_display_shell_remove_item           (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);

void   clay_display_shell_add_preview_item      (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);
void   clay_display_shell_remove_preview_item   (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);

void   clay_display_shell_add_unrotated_item    (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);
void   clay_display_shell_remove_unrotated_item (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);

void   clay_display_shell_add_tool_item         (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);
void   clay_display_shell_remove_tool_item      (ClayDisplayShell *shell,
                                                 ClayCanvasItem   *item);
