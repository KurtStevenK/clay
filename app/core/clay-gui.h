/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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


typedef struct _ClayGui ClayGui;

struct _ClayGui
{
  void           (* ungrab)                    (Clay                *clay);

  void           (* set_busy)                  (Clay                *clay);
  void           (* unset_busy)                (Clay                *clay);

  void           (* show_message)              (Clay                *clay,
                                                GObject             *handler,
                                                ClayMessageSeverity  severity,
                                                const gchar         *domain,
                                                const gchar         *message);
  void           (* help)                      (Clay                *clay,
                                                ClayProgress        *progress,
                                                const gchar         *help_domain,
                                                const gchar         *help_id);

  gboolean       (* wait)                      (Clay                *clay,
                                                ClayWaitable        *waitable,
                                                const gchar         *message);

  const gchar  * (* get_program_class)         (Clay                *clay);
  gchar        * (* get_display_name)          (Clay                *clay,
                                                gint                 display_id,
                                                GObject            **monitor,
                                                gint                *monitor_number);
  guint32        (* get_user_time)             (Clay                *clay);

  GFile        * (* get_theme_dir)             (Clay                *clay);
  GFile        * (* get_icon_theme_dir)        (Clay                *clay);

  ClayObject   * (* get_window_strategy)       (Clay                *clay);
  ClayDisplay  * (* get_empty_display)         (Clay                *clay);
  GBytes       * (* display_get_window_handle) (ClayDisplay         *display);
  ClayDisplay  * (* display_create)            (Clay                *clay,
                                                ClayImage           *image,
                                                ClayUnit            *unit,
                                                gdouble              scale,
                                                GObject             *monitor);
  void           (* display_delete)            (ClayDisplay         *display);
  void           (* displays_reconnect)        (Clay                *clay,
                                                ClayImage           *old_image,
                                                ClayImage           *new_image);

  ClayProgress * (* progress_new)              (Clay                *clay,
                                                ClayDisplay         *display);
  void           (* progress_free)             (Clay                *clay,
                                                ClayProgress        *progress);

  gboolean       (* pdb_dialog_new)            (Clay                *clay,
                                                ClayContext         *context,
                                                ClayProgress        *progress,
                                                GType                contents_type,
                                                GBytes              *parent_handle,
                                                const gchar         *title,
                                                const gchar         *callback_name,
                                                ClayObject          *object,
                                                va_list              args);
  gboolean       (* pdb_dialog_set)            (Clay                *clay,
                                                GType                contents_type,
                                                const gchar         *callback_name,
                                                ClayObject          *object,
                                                va_list              args);
  gboolean       (* pdb_dialog_close)          (Clay                *clay,
                                                GType                contents_type,
                                                const gchar         *callback_name);
  gboolean       (* recent_list_add_file)      (Clay                *clay,
                                                GFile               *file,
                                                const gchar         *mime_type);
  void           (* recent_list_load)          (Clay                *clay);

  GMountOperation
               * (* get_mount_operation)       (Clay                *clay,
                                                ClayProgress        *progress);

  ClayColorProfilePolicy
                 (* query_profile_policy)      (Clay                *clay,
                                                ClayImage           *image,
                                                ClayContext         *context,
                                                ClayColorProfile   **dest_profile,
                                                ClayColorRenderingIntent *intent,
                                                gboolean            *bpc,
                                                gboolean            *dont_ask);

  ClayMetadataRotationPolicy
                 (* query_rotation_policy)     (Clay                *clay,
                                                ClayImage           *image,
                                                ClayContext         *context,
                                                gboolean            *dont_ask);
};


void           clay_gui_init                  (Clay                *clay);

void           clay_gui_ungrab                (Clay                *clay);

ClayObject   * clay_get_window_strategy       (Clay                *clay);
ClayDisplay  * clay_get_empty_display         (Clay                *clay);
GBytes       * clay_get_display_window_handle (Clay                *clay,
                                               ClayDisplay         *display);
ClayDisplay  * clay_create_display            (Clay                *clay,
                                               ClayImage           *image,
                                               ClayUnit            *unit,
                                               gdouble              scale,
                                               GObject             *monitor);
void           clay_delete_display            (Clay                *clay,
                                               ClayDisplay         *display);
void           clay_reconnect_displays        (Clay                *clay,
                                               ClayImage           *old_image,
                                               ClayImage           *new_image);

void           clay_set_busy                  (Clay                *clay);
void           clay_set_busy_until_idle       (Clay                *clay);
void           clay_unset_busy                (Clay                *clay);

void           clay_show_message              (Clay                *clay,
                                               GObject             *handler,
                                               ClayMessageSeverity  severity,
                                               const gchar         *domain,
                                               const gchar         *message);
void           clay_help                      (Clay                *clay,
                                               ClayProgress        *progress,
                                               const gchar         *help_domain,
                                               const gchar         *help_id);

void           clay_wait                      (Clay                *clay,
                                               ClayWaitable        *waitable,
                                               const gchar         *format,
                                               ...) G_GNUC_PRINTF (3, 4);

ClayProgress * clay_new_progress              (Clay                *clay,
                                               ClayDisplay         *display);
void           clay_free_progress             (Clay                *clay,
                                               ClayProgress        *progress);

const gchar  * clay_get_program_class         (Clay                *clay);
gchar        * clay_get_display_name          (Clay                *clay,
                                               gint                 display_id,
                                               GObject            **monitor,
                                               gint                *monitor_number);
guint32        clay_get_user_time             (Clay                *clay);
GFile        * clay_get_theme_dir             (Clay                *clay);
GFile        * clay_get_icon_theme_dir        (Clay                *clay);

gboolean       clay_pdb_dialog_new            (Clay                *clay,
                                               ClayContext         *context,
                                               ClayProgress        *progress,
                                               GType                contents_type,
                                               GBytes              *parent_handle,
                                               const gchar         *title,
                                               const gchar         *callback_name,
                                               ClayObject          *object,
                                               ...) G_GNUC_NULL_TERMINATED;
gboolean       clay_pdb_dialog_set            (Clay                *clay,
                                               GType                contents_type,
                                               const gchar         *callback_name,
                                               ClayObject          *object,
                                               ...) G_GNUC_NULL_TERMINATED;
gboolean       clay_pdb_dialog_close          (Clay                *clay,
                                               GType                contents_type,
                                               const gchar         *callback_name);

gboolean       clay_recent_list_add_file      (Clay                *clay,
                                               GFile               *file,
                                               const gchar         *mime_type);
void           clay_recent_list_load          (Clay                *clay);

GMountOperation
             * clay_get_mount_operation       (Clay                *clay,
                                               ClayProgress        *progress);

ClayColorProfilePolicy
               clay_query_profile_policy      (Clay                *clay,
                                               ClayImage           *image,
                                               ClayContext         *context,
                                               ClayColorProfile   **dest_profile,
                                               ClayColorRenderingIntent *intent,
                                               gboolean            *bpc,
                                               gboolean            *dont_ask);

ClayMetadataRotationPolicy
               clay_query_rotation_policy     (Clay                *clay,
                                               ClayImage           *image,
                                               ClayContext         *context,
                                               gboolean            *dont_ask);
