/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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
  CLAY_CONFIG_DUMP_NONE,
  CLAY_CONFIG_DUMP_CLAYRC,
  CLAY_CONFIG_DUMP_CLAYRC_SYSTEM,
  CLAY_CONFIG_DUMP_CLAYRC_MANPAGE
} ClayConfigDumpFormat;


gboolean  clay_config_dump (GObject              *clay,
                            ClayConfigDumpFormat  format);
