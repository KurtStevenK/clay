/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugindebug.h
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


typedef enum
{
  CLAY_DEBUG_WRAP_QUERY = 1 << 0,
  CLAY_DEBUG_WRAP_INIT  = 1 << 1,
  CLAY_DEBUG_WRAP_RUN   = 1 << 2,

  CLAY_DEBUG_WRAP_DEFAULT = CLAY_DEBUG_WRAP_RUN
} ClayDebugWrapFlag;


ClayPlugInDebug  * clay_plug_in_debug_new  (void);
void               clay_plug_in_debug_free (ClayPlugInDebug    *debug);

gchar           ** clay_plug_in_debug_argv (ClayPlugInDebug    *debug,
                                            const gchar        *name,
                                            ClayDebugWrapFlag   flag,
                                            const gchar       **args);
