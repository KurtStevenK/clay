/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_THUMB_H_INSIDE__) && !defined (CLAY_THUMB_COMPILATION)
#error "Only <libclaythumb/claythumb.h> can be included directly."
#endif

#ifndef __CLAY_THUMB_ERROR_H__
#define __CLAY_THUMB_ERROR_H__

G_BEGIN_DECLS


/**
 * ClayThumbError:
 * @CLAY_THUMB_ERROR_OPEN:         there was a problem opening the file
 * @CLAY_THUMB_ERROR_OPEN_ENOENT:  the file doesn't exist
 * @CLAY_THUMB_ERROR_MKDIR:        there was a problem creating a directory
 *
 * These are the possible error codes used when a #GError is set by
 * libclaythumb.
 **/
typedef enum
{
  CLAY_THUMB_ERROR_OPEN,
  CLAY_THUMB_ERROR_OPEN_ENOENT,
  CLAY_THUMB_ERROR_MKDIR
} ClayThumbError;


/**
 * CLAY_THUMB_ERROR:
 *
 * Identifier for the libclaythumb error domain.
 **/
#define CLAY_THUMB_ERROR (clay_thumb_error_quark ())

GQuark  clay_thumb_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __CLAY_THUMB_ERROR_H__ */
