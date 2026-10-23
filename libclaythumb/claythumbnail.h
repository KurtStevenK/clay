/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2004  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_THUMBNAIL_H__
#define __CLAY_THUMBNAIL_H__

G_BEGIN_DECLS


#define CLAY_TYPE_THUMBNAIL (clay_thumbnail_get_type ())
G_DECLARE_FINAL_TYPE (ClayThumbnail, clay_thumbnail, CLAY, THUMBNAIL, GObject)


ClayThumbnail  * clay_thumbnail_new              (void);

void             clay_thumbnail_set_uri          (ClayThumbnail  *thumbnail,
                                                  const gchar    *uri);
gboolean         clay_thumbnail_set_filename     (ClayThumbnail  *thumbnail,
                                                  const gchar    *filename,
                                                  GError        **error);
gboolean         clay_thumbnail_set_from_thumb   (ClayThumbnail  *thumbnail,
                                                  const gchar    *filename,
                                                  GError        **error);

ClayThumbState   clay_thumbnail_peek_image       (ClayThumbnail  *thumbnail);
ClayThumbState   clay_thumbnail_peek_thumb       (ClayThumbnail  *thumbnail,
                                                  ClayThumbSize   size);

ClayThumbState   clay_thumbnail_check_thumb      (ClayThumbnail  *thumbnail,
                                                  ClayThumbSize   size);

GdkPixbuf      * clay_thumbnail_load_thumb       (ClayThumbnail  *thumbnail,
                                                  ClayThumbSize   size,
                                                  GError        **error);

gboolean         clay_thumbnail_save_thumb       (ClayThumbnail  *thumbnail,
                                                  GdkPixbuf      *pixbuf,
                                                  const gchar    *software,
                                                  GError        **error);
gboolean         clay_thumbnail_save_thumb_local (ClayThumbnail  *thumbnail,
                                                  GdkPixbuf      *pixbuf,
                                                  const gchar    *software,
                                                  GError        **error);

gboolean         clay_thumbnail_save_failure     (ClayThumbnail  *thumbnail,
                                                  const gchar    *software,
                                                  GError        **error);
void             clay_thumbnail_delete_failure   (ClayThumbnail  *thumbnail);
void             clay_thumbnail_delete_others    (ClayThumbnail  *thumbnail,
                                                  ClayThumbSize   size);

gboolean         clay_thumbnail_has_failed       (ClayThumbnail  *thumbnail);


G_END_DECLS

#endif /* __CLAY_THUMBNAIL_H__ */
