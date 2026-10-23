/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * clayexportoptions.h
 * Copyright (C) 2024 Alx Sa.
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

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __LIBCLAY_CLAY_EXPORT_OPTIONS_H__
#define __LIBCLAY_CLAY_EXPORT_OPTIONS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayExportReturn:
 * @CLAY_EXPORT_IGNORE: The image is unmodified but export shall continue anyway
 * @CLAY_EXPORT_EXPORT: The chosen transforms were applied to a new image
 *
 * Possible return values of [method@ExportOptions.get_image].
 **/
typedef enum
{
  CLAY_EXPORT_IGNORE,
  CLAY_EXPORT_EXPORT
} ClayExportReturn;


ClayExportReturn    clay_export_options_get_image     (ClayExportOptions  *options,
                                                       ClayImage         **image) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS


#endif /* __LIBCLAY_CLAY_EXPORT_OPTIONS_H__ */
