/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayextension-error.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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
  /* Generic error condition. */
  CLAY_EXTENSION_FAILED,
  CLAY_EXTENSION_BAD_APPDATA,
  CLAY_EXTENSION_BAD_ID,
  CLAY_EXTENSION_NO_VERSION,
  CLAY_EXTENSION_BAD_PATH
} ClayExtensionErrorCode;


#define CLAY_EXTENSION_ERROR (clay_extension_error_quark ())

GQuark  clay_extension_error_quark (void) G_GNUC_CONST;
