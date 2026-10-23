/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprogress.h
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PROGRESS (clay_progress_get_type ())
G_DECLARE_INTERFACE (ClayProgress,
                     clay_progress,
                     CLAY, PROGRESS,
                     GObject)


struct _ClayProgressInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  ClayProgress * (* start)             (ClayProgress        *progress,
                                        gboolean             cancellable,
                                        const gchar         *message);
  void           (* end)               (ClayProgress        *progress);
  gboolean       (* is_active)         (ClayProgress        *progress);

  void           (* set_text)          (ClayProgress        *progress,
                                        const gchar         *message);
  void           (* set_value)         (ClayProgress        *progress,
                                        gdouble              percentage);
  gdouble        (* get_value)         (ClayProgress        *progress);
  void           (* pulse)             (ClayProgress        *progress);

  GBytes       * (* get_window_handle) (ClayProgress        *progress);

  gboolean       (* message)           (ClayProgress        *progress,
                                        Clay                *clay,
                                        ClayMessageSeverity  severity,
                                        const gchar         *domain,
                                        const gchar         *message);

  /*  signals  */
  void           (* cancel)            (ClayProgress        *progress);
};


ClayProgress * clay_progress_start             (ClayProgress        *progress,
                                                gboolean             cancellable,
                                                const gchar         *format,
                                                ...) G_GNUC_PRINTF (3, 4);
void           clay_progress_end               (ClayProgress        *progress);
gboolean       clay_progress_is_active         (ClayProgress        *progress);

void           clay_progress_set_text          (ClayProgress        *progress,
                                                const gchar         *format,
                                                ...) G_GNUC_PRINTF (2, 3);
void           clay_progress_set_text_literal  (ClayProgress        *progress,
                                                const gchar         *message);
void           clay_progress_set_value         (ClayProgress        *progress,
                                                gdouble              percentage);
gdouble        clay_progress_get_value         (ClayProgress        *progress);
void           clay_progress_pulse             (ClayProgress        *progress);

GBytes       * clay_progress_get_window_handle (ClayProgress        *progress);

gboolean       clay_progress_message           (ClayProgress        *progress,
                                                Clay                *clay,
                                                ClayMessageSeverity  severity,
                                                const gchar         *domain,
                                                const gchar         *message);

void           clay_progress_cancel            (ClayProgress        *progress);

void           clay_progress_update_and_flush  (gint                 min,
                                                gint                 max,
                                                gint                 current,
                                                gpointer             data);
