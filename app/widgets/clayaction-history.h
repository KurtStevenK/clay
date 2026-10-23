/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayaction-history.h
 * Copyright (C) 2013  Jehan <jehan at girinstud.io>
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


typedef gboolean (* ClayActionMatchFunc) (ClayAction  *action,
                                          const gchar *keyword,
                                          gint        *section,
                                          Clay        *clay);


void       clay_action_history_init                  (Clay                *clay);
void       clay_action_history_exit                  (Clay                *clay);

void       clay_action_history_clear                 (Clay                *clay);

GList    * clay_action_history_search                (Clay                *clay,
                                                      ClayActionMatchFunc  match_func,
                                                      const gchar         *keyword);

gboolean   clay_action_history_is_blacklisted_action (const gchar         *action_name);
gboolean   clay_action_history_is_excluded_action    (const gchar         *action_name);

void       clay_action_history_action_activated      (ClayAction          *action);
