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

#ifndef __CLAY_THUMB_ENUMS_H__
#define __CLAY_THUMB_ENUMS_H__

G_BEGIN_DECLS


/**
 * SECTION: claythumb-enums
 * @title: ClayThumb-enums
 * @short_description: Enumerations used by libclaythumb
 *
 * Enumerations used by libclaythumb
 **/


/**
 * ClayThumbFileType:
 * @CLAY_THUMB_FILE_TYPE_NONE:    file does not exist
 * @CLAY_THUMB_FILE_TYPE_REGULAR: a regular file
 * @CLAY_THUMB_FILE_TYPE_FOLDER:  a directory
 * @CLAY_THUMB_FILE_TYPE_SPECIAL: a special file (device node, fifo, socket, ...)
 *
 * File types as returned by clay_thumb_file_test().
 **/
#define CLAY_TYPE_THUMB_FILE_TYPE (clay_thumb_file_type_get_type ())

GType clay_thumb_file_type_get_type (void);

typedef enum
{
  CLAY_THUMB_FILE_TYPE_NONE,
  CLAY_THUMB_FILE_TYPE_REGULAR,
  CLAY_THUMB_FILE_TYPE_FOLDER,
  CLAY_THUMB_FILE_TYPE_SPECIAL
} ClayThumbFileType;


/**
 * ClayThumbSize:
 * @CLAY_THUMB_SIZE_FAIL:   special size used to indicate a thumbnail
 *                          creation failure
 * @CLAY_THUMB_SIZE_NORMAL: normal thumbnail size (128 pixels)
 * @CLAY_THUMB_SIZE_LARGE:  large thumbnail size (256 pixels)
 *
 * Possible thumbnail sizes as defined by the Thumbnail Managing
 * Standard.
 **/
#define CLAY_TYPE_THUMB_SIZE (clay_thumb_size_get_type ())

GType clay_thumb_size_get_type (void);

typedef enum
{
  CLAY_THUMB_SIZE_FAIL   = 0,
  CLAY_THUMB_SIZE_NORMAL = 128,
  CLAY_THUMB_SIZE_LARGE  = 256
} ClayThumbSize;


/**
 * ClayThumbState:
 * @CLAY_THUMB_STATE_UNKNOWN:   nothing is known about the file/thumbnail
 * @CLAY_THUMB_STATE_REMOTE:    the file is on a remote file system
 * @CLAY_THUMB_STATE_FOLDER:    the file is a directory
 * @CLAY_THUMB_STATE_SPECIAL:   the file is a special file
 * @CLAY_THUMB_STATE_NOT_FOUND: the file/thumbnail doesn't exist
 * @CLAY_THUMB_STATE_EXISTS:    the file/thumbnail exists
 * @CLAY_THUMB_STATE_OLD:       the thumbnail may be outdated
 * @CLAY_THUMB_STATE_FAILED:    the thumbnail couldn't be created
 * @CLAY_THUMB_STATE_OK:        the thumbnail exists and matches the image
 *
 * Possible image and thumbnail file states used by libclaythumb.
 **/
#define CLAY_TYPE_THUMB_STATE (clay_thumb_state_get_type ())

GType clay_thumb_state_get_type (void);

typedef enum
{
  CLAY_THUMB_STATE_UNKNOWN,
  CLAY_THUMB_STATE_REMOTE,
  CLAY_THUMB_STATE_FOLDER,
  CLAY_THUMB_STATE_SPECIAL,
  CLAY_THUMB_STATE_NOT_FOUND,
  CLAY_THUMB_STATE_EXISTS,
  CLAY_THUMB_STATE_OLD,
  CLAY_THUMB_STATE_FAILED,
  CLAY_THUMB_STATE_OK
} ClayThumbState;


G_END_DECLS

#endif  /* __CLAY_THUMB_ENUMS_H__ */
