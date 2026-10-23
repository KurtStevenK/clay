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

#include "dialogs/dialogs-types.h"
#include "tools/tools-types.h"


typedef enum
{
  CLAY_ACTION_SELECT_SET              =  0,
  CLAY_ACTION_SELECT_SET_TO_DEFAULT   = -1,
  CLAY_ACTION_SELECT_FIRST            = -2,
  CLAY_ACTION_SELECT_LAST             = -3,
  CLAY_ACTION_SELECT_SMALL_PREVIOUS   = -4,
  CLAY_ACTION_SELECT_SMALL_NEXT       = -5,
  CLAY_ACTION_SELECT_PREVIOUS         = -6,
  CLAY_ACTION_SELECT_NEXT             = -7,
  CLAY_ACTION_SELECT_SKIP_PREVIOUS    = -8,
  CLAY_ACTION_SELECT_SKIP_NEXT        = -9,
  CLAY_ACTION_SELECT_PERCENT_PREVIOUS = -10,
  CLAY_ACTION_SELECT_PERCENT_NEXT     = -11,
  CLAY_ACTION_SELECT_FLAT_PREVIOUS    = -12,
  CLAY_ACTION_SELECT_FLAT_NEXT        = -13,
} ClayActionSelectType;

typedef enum
{
  CLAY_SAVE_MODE_SAVE,
  CLAY_SAVE_MODE_SAVE_AS,
  CLAY_SAVE_MODE_SAVE_A_COPY,
  CLAY_SAVE_MODE_SAVE_AND_CLOSE,
  CLAY_SAVE_MODE_EXPORT,
  CLAY_SAVE_MODE_EXPORT_AS,
  CLAY_SAVE_MODE_OVERWRITE
} ClaySaveMode;
