/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugin-proc.h
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


gboolean   clay_plug_in_set_proc_image_types         (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *image_types,
                                                      GError       **error);
gboolean   clay_plug_in_set_proc_sensitivity_mask    (ClayPlugIn   *plug_in,
                                                      const gchar  *proc_name,
                                                      gint          sensitivity_mask,
                                                      GError      **error);
gboolean   clay_plug_in_set_proc_menu_label          (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *menu_label,
                                                      GError       **error);
gboolean   clay_plug_in_add_proc_menu_path           (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *menu_path,
                                                      GError       **error);
gboolean   clay_plug_in_set_proc_icon                (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      ClayIconType   type,
                                                      const guint8  *data,
                                                      gint           data_length,
                                                      GError       **error);
gboolean   clay_plug_in_set_proc_help                (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *blurb,
                                                      const gchar   *help,
                                                      const gchar   *help_id,
                                                      GError       **error);
gboolean   clay_plug_in_set_proc_attribution         (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *authors,
                                                      const gchar   *copyright,
                                                      const gchar   *date,
                                                      GError       **error);

gboolean   clay_plug_in_set_file_proc_load_handler   (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *extensions,
                                                      const gchar   *meta_extensions,
                                                      const gchar   *prefixes,
                                                      const gchar   *magics,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_save_handler   (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *extensions,
                                                      const gchar   *meta_extensions,
                                                      const gchar   *prefixes,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_priority       (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      gint           priority,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_mime_types     (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *mime_types,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_handles_remote (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_handles_raw    (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_handles_vector (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      GError       **error);
gboolean   clay_plug_in_set_file_proc_thumb_loader   (ClayPlugIn    *plug_in,
                                                      const gchar   *proc_name,
                                                      const gchar   *thumb_proc,
                                                      GError       **error);
gboolean   clay_plug_in_set_batch_interpreter        (ClayPlugIn   *plug_in,
                                                      const gchar  *proc_name,
                                                      const gchar  *interpreter_name,
                                                      GError      **error);
gboolean   clay_plug_in_set_proc_help_uri            (ClayPlugIn   *plug_in,
                                                      const gchar  *proc_name,
                                                      const gchar  *help_uri,
                                                      GError      **error);
