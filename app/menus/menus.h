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


void              menus_init                        (Clay      *clay);
void              menus_exit                        (Clay      *clay);

void              menus_restore                     (Clay      *clay);
void              menus_save                        (Clay      *clay,
                                                     gboolean   always_save);

gboolean          menus_clear                       (Clay      *clay,
                                                     GError   **error);
void              menus_remove                      (Clay      *clay);

ClayMenuFactory * menus_get_global_menu_factory     (Clay      *clay);
ClayUIManager   * menus_get_image_manager_singleton (Clay      *clay);

#ifdef PLATFORM_OSX
void              menus_quartz_app_menu             (Clay      *clay);
gchar           * menus_quartz_get_settings_label   (void);
#endif
