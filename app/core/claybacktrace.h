/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybacktrace.h
 * Copyright (C) 2018 Ell
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


typedef struct _ClayBacktraceAddressInfo ClayBacktraceAddressInfo;


struct _ClayBacktraceAddressInfo
{
  gchar    object_name[256];

  gchar    symbol_name[256];
  guintptr symbol_address;

  gchar    source_file[256];
  gint     source_line;
};


void            clay_backtrace_init              (void);

gboolean        clay_backtrace_start             (void);
void            clay_backtrace_stop              (void);

ClayBacktrace * clay_backtrace_new               (gboolean                 include_current_thread);
void            clay_backtrace_free              (ClayBacktrace           *backtrace);

gint            clay_backtrace_get_n_threads     (ClayBacktrace           *backtrace);
guintptr        clay_backtrace_get_thread_id     (ClayBacktrace           *backtrace,
                                                  gint                     thread);
const gchar   * clay_backtrace_get_thread_name   (ClayBacktrace           *backtrace,
                                                  gint                     thread);
gboolean        clay_backtrace_is_thread_running (ClayBacktrace           *backtrace,
                                                  gint                     thread);

gint            clay_backtrace_find_thread_by_id (ClayBacktrace           *backtrace,
                                                  guintptr                 thread_id,
                                                  gint                     thread_hint);

gint            clay_backtrace_get_n_frames      (ClayBacktrace           *backtrace,
                                                  gint                     thread);
guintptr        clay_backtrace_get_frame_address (ClayBacktrace           *backtrace,
                                                  gint                     thread,
                                                  gint                     frame);

gboolean        clay_backtrace_get_address_info  (guintptr                  address,
                                                  ClayBacktraceAddressInfo *info);
