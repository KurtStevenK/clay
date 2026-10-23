/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-progress.h
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


gint       clay_plug_in_progress_attach            (ClayProgress        *progress);
gint       clay_plug_in_progress_detach            (ClayProgress        *progress);

void       clay_plug_in_progress_start             (ClayPlugIn          *plug_in,
                                                    const gchar         *message,
                                                    ClayDisplay         *display);
void       clay_plug_in_progress_end               (ClayPlugIn          *plug_in,
                                                    ClayPlugInProcFrame *proc_frame);
void       clay_plug_in_progress_set_text          (ClayPlugIn          *plug_in,
                                                    const gchar         *message);
void       clay_plug_in_progress_set_value         (ClayPlugIn          *plug_in,
                                                    gdouble              percentage);
void       clay_plug_in_progress_pulse             (ClayPlugIn          *plug_in);
GBytes   * clay_plug_in_progress_get_window_handle (ClayPlugIn          *plug_in);

gboolean   clay_plug_in_progress_install           (ClayPlugIn          *plug_in,
                                                    const gchar         *progress_callback);
gboolean   clay_plug_in_progress_uninstall         (ClayPlugIn          *plug_in,
                                                    const gchar         *progress_callback);
gboolean   clay_plug_in_progress_cancel            (ClayPlugIn          *plug_in,
                                                    const gchar         *progress_callback);
