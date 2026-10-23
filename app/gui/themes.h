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

#ifdef PLATFORM_OSX
/* Needed for MAC_OS_X_VERSION_MIN_REQUIRED check below */
#include <AvailabilityMacros.h>
#endif

void     themes_init                 (Clay          *clay);
void     themes_exit                 (Clay          *clay);

gboolean themes_theme_is_system      (Clay          *clay,
                                      const gchar   *theme_name);
gboolean themes_theme_is_default     (Clay          *clay,
                                      const gchar   *theme_name);

gchar ** themes_list_themes          (Clay          *clay,
                                      gint          *n_themes);
GFile  * themes_get_theme_dir        (Clay          *clay,
                                      const gchar   *theme_name);
GFile  * themes_get_theme_file       (Clay          *clay,
                                      const gchar   *first_component,
                                      ...) G_GNUC_NULL_TERMINATED;

void     themes_theme_change_notify  (ClayGuiConfig *config,
                                      GParamSpec    *pspec,
                                      Clay          *clay);

#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
void     themes_set_title_bar        (Clay          *clay);
#endif
