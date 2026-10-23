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


#define CLAY_TYPE_PLUG_IN_IMAGE_TYPE (clay_plug_in_image_type_get_type ())

GType clay_plug_in_image_type_get_type (void);

typedef enum
{
  CLAY_PLUG_IN_RGB_IMAGE      = 1 << 0,
  CLAY_PLUG_IN_GRAY_IMAGE     = 1 << 1,
  CLAY_PLUG_IN_INDEXED_IMAGE  = 1 << 2,
  CLAY_PLUG_IN_RGBA_IMAGE     = 1 << 3,
  CLAY_PLUG_IN_GRAYA_IMAGE    = 1 << 4,
  CLAY_PLUG_IN_INDEXEDA_IMAGE = 1 << 5
} ClayPlugInImageType;


#define CLAY_TYPE_PLUG_CALL_MODE (clay_plug_in_call_mode_get_type ())

GType clay_plug_in_call_mode_get_type (void);

typedef enum
{
  CLAY_PLUG_IN_CALL_NONE,
  CLAY_PLUG_IN_CALL_RUN,
  CLAY_PLUG_IN_CALL_QUERY,
  CLAY_PLUG_IN_CALL_INIT
} ClayPlugInCallMode;


#define CLAY_TYPE_FILE_PROCEDURE_GROUP (clay_file_procedure_group_get_type ())

GType clay_file_procedure_group_get_type (void);

typedef enum
{
  CLAY_FILE_PROCEDURE_GROUP_NONE,
  CLAY_FILE_PROCEDURE_GROUP_ANY,
  CLAY_FILE_PROCEDURE_GROUP_OPEN,
  CLAY_FILE_PROCEDURE_GROUP_SAVE,
  CLAY_FILE_PROCEDURE_GROUP_EXPORT
} ClayFileProcedureGroup;

