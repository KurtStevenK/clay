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

#include "core/core-types.h"


typedef struct _ClayPDB                ClayPDB;
typedef struct _ClayProcedure          ClayProcedure;
typedef struct _ClayPlugInProcedure    ClayPlugInProcedure;
typedef struct _ClayTemporaryProcedure ClayTemporaryProcedure;


typedef enum
{
  CLAY_PDB_COMPAT_OFF,
  CLAY_PDB_COMPAT_ON,
  CLAY_PDB_COMPAT_WARN
} ClayPDBCompatMode;


typedef enum
{
  CLAY_PDB_ITEM_CONTENT  = 1 << 0,
  CLAY_PDB_ITEM_POSITION = 1 << 1
} ClayPDBItemModify;


typedef enum
{
  CLAY_PDB_DATA_ACCESS_READ   = 0,
  CLAY_PDB_DATA_ACCESS_WRITE  = 1 << 0,
  CLAY_PDB_DATA_ACCESS_RENAME = 1 << 1
} ClayPDBDataAccess;
