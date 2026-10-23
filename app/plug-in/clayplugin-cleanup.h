/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-cleanup.h
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


gboolean   clay_plug_in_cleanup_undo_group_start (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);
gboolean   clay_plug_in_cleanup_undo_group_end   (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);

gboolean   clay_plug_in_cleanup_layers_freeze    (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);
gboolean   clay_plug_in_cleanup_layers_thaw      (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);

gboolean   clay_plug_in_cleanup_channels_freeze  (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);
gboolean   clay_plug_in_cleanup_channels_thaw    (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);

gboolean   clay_plug_in_cleanup_paths_freeze     (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);
gboolean   clay_plug_in_cleanup_paths_thaw       (ClayPlugIn          *plug_in,
                                                  ClayImage           *image);

gboolean   clay_plug_in_cleanup_add_shadow       (ClayPlugIn          *plug_in,
                                                  ClayDrawable        *drawable);
gboolean   clay_plug_in_cleanup_remove_shadow    (ClayPlugIn          *plug_in,
                                                  ClayDrawable        *drawable);

void       clay_plug_in_cleanup                  (ClayPlugIn          *plug_in,
                                                  ClayPlugInProcFrame *proc_frame);
