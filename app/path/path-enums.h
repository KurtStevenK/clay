/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * path-enums.h
 * Copyright (C) 2006 Simon Budig  <simon@gimp.org>
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
  CLAY_ANCHOR_ANCHOR,
  CLAY_ANCHOR_CONTROL
} ClayAnchorType;

typedef enum
{
  CLAY_ANCHOR_FEATURE_NONE,
  CLAY_ANCHOR_FEATURE_EDGE,
  CLAY_ANCHOR_FEATURE_ALIGNED,
  CLAY_ANCHOR_FEATURE_SYMMETRIC
} ClayAnchorFeatureType;

typedef enum
{
  CLAY_STROKE_EXTEND_SIMPLE,
  CLAY_STROKE_EXTEND_EDITABLE
} ClayStrokeExtendMode;
