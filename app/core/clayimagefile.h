/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimagefile.h
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#include "clayviewable.h"


#define CLAY_TYPE_IMAGEFILE (clay_imagefile_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayImagefile,
                          clay_imagefile,
                          CLAY, IMAGEFILE,
                          ClayViewable)


struct _ClayImagefileClass
{
  ClayViewableClass  parent_class;

  void (* info_changed) (ClayImagefile *imagefile);
};


ClayImagefile * clay_imagefile_new                   (Clay           *clay,
                                                      GFile          *file);

GFile         * clay_imagefile_get_file              (ClayImagefile  *imagefile);
void            clay_imagefile_set_file              (ClayImagefile  *imagefile,
                                                      GFile          *file);

ClayThumbnail * clay_imagefile_get_thumbnail         (ClayImagefile  *imagefile);
GIcon         * clay_imagefile_get_gicon             (ClayImagefile  *imagefile);

void            clay_imagefile_set_mime_type         (ClayImagefile  *imagefile,
                                                      const gchar    *mime_type);
void            clay_imagefile_update                (ClayImagefile  *imagefile);
gboolean        clay_imagefile_create_thumbnail      (ClayImagefile  *imagefile,
                                                      ClayContext    *context,
                                                      ClayProgress   *progress,
                                                      gint            size,
                                                      gboolean        replace,
                                                      GError        **error);
void            clay_imagefile_create_thumbnail_weak (ClayImagefile  *imagefile,
                                                      ClayContext    *context,
                                                      ClayProgress   *progress,
                                                      gint            size,
                                                      gboolean        replace);
gboolean        clay_imagefile_check_thumbnail       (ClayImagefile  *imagefile);
gboolean        clay_imagefile_save_thumbnail        (ClayImagefile  *imagefile,
                                                      const gchar    *mime_type,
                                                      ClayImage      *image,
                                                      GError        **error);
const gchar   * clay_imagefile_get_desc_string       (ClayImagefile  *imagefile);
