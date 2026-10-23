/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-filter-history.h
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


gint            clay_filter_history_size   (Clay          *clay);
gint            clay_filter_history_length (Clay          *clay);
ClayProcedure * clay_filter_history_nth    (Clay          *clay,
                                            gint           n);
void            clay_filter_history_add    (Clay          *clay,
                                            ClayProcedure *procedure);
void            clay_filter_history_remove (Clay          *clay,
                                            ClayProcedure *procedure);
void            clay_filter_history_clear  (Clay          *clay);
