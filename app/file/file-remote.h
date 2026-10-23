/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * file-remote.h
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *
 * Based on: URI plug-in, GIO/GVfs backend
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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


gboolean   file_remote_mount_file           (Clay          *clay,
                                             GFile         *file,
                                             ClayProgress  *progress,
                                             GError       **error);

GFile    * file_remote_download_image       (Clay          *clay,
                                             GFile         *file,
                                             ClayProgress  *progress,
                                             GError       **error);

GFile    * file_remote_upload_image_prepare (Clay          *clay,
                                             GFile         *file,
                                             ClayProgress  *progress,
                                             GError       **error);
gboolean   file_remote_upload_image_finish  (Clay          *clay,
                                             GFile         *file,
                                             GFile         *local_file,
                                             ClayProgress  *progress,
                                             GError       **error);
