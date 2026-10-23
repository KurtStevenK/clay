/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * icon-themes.h
 * Copyright (C) 2015 Benoit Touchette
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


void        icon_themes_init                    (Clay        *clay);
void        icon_themes_exit                    (Clay        *clay);

gboolean    icon_themes_theme_is_default        (Clay        *clay,
                                                 const gchar *theme_name);
gboolean    icon_themes_theme_is_legacy         (Clay        *clay,
                                                 const gchar *theme_name);

gchar    ** icon_themes_list_themes             (Clay        *clay,
                                                 gint        *n_themes);
GFile     * icon_themes_get_theme_dir           (Clay        *clay,
                                                 const gchar *theme_name);

gboolean    icon_themes_current_prefer_symbolic (Clay        *clay);
