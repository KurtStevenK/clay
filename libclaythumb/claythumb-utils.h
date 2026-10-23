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

#ifndef __CLAY_THUMB_UTILS_H__
#define __CLAY_THUMB_UTILS_H__

G_BEGIN_DECLS


gboolean            clay_thumb_init                   (const gchar    *creator,
                                                       const gchar    *thumb_basedir);

const gchar       * clay_thumb_get_thumb_base_dir     (void);

gchar             * clay_thumb_find_thumb             (const gchar    *uri,
                                                       ClayThumbSize  *size) G_GNUC_MALLOC;

ClayThumbFileType   clay_thumb_file_test              (const gchar    *filename,
                                                       gint64         *mtime,
                                                       gint64         *size,
                                                       gint           *err_no);

gchar             * clay_thumb_name_from_uri          (const gchar    *uri,
                                                       ClayThumbSize   size) G_GNUC_MALLOC;
const gchar       * clay_thumb_get_thumb_dir          (ClayThumbSize   size);
gboolean            clay_thumb_ensure_thumb_dir       (ClayThumbSize   size,
                                                       GError        **error);
void                clay_thumbs_delete_for_uri        (const gchar    *uri);

gchar             * clay_thumb_name_from_uri_local    (const gchar    *uri,
                                                       ClayThumbSize   size) G_GNUC_MALLOC;
gchar             * clay_thumb_get_thumb_dir_local    (const gchar    *dirname,
                                                       ClayThumbSize   size) G_GNUC_MALLOC;
gboolean            clay_thumb_ensure_thumb_dir_local (const gchar    *dirname,
                                                       ClayThumbSize   size,
                                                       GError        **error);
void                clay_thumbs_delete_for_uri_local  (const gchar    *uri);


/*  for internal use only   */
G_GNUC_INTERNAL void    _clay_thumbs_delete_others    (const gchar    *uri,
                                                       ClayThumbSize   size);
G_GNUC_INTERNAL gchar * _clay_thumb_filename_from_uri (const gchar    *uri);


G_END_DECLS

#endif /* __CLAY_THUMB_UTILS_H__ */
