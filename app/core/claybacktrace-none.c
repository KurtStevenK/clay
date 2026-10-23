/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claybacktrace-none.c
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


#include "config.h"

#include <gio/gio.h>

#include "claybacktrace-backend.h"


#ifdef CLAY_BACKTRACE_BACKEND_NONE


#include "core-types.h"

#include "claybacktrace.h"


/*  public functions  */


void
clay_backtrace_init (void)
{
}

gboolean
clay_backtrace_start (void)
{
  return FALSE;
}

void
clay_backtrace_stop (void)
{
}

ClayBacktrace *
clay_backtrace_new (gboolean include_current_thread)
{
  return NULL;
}

void
clay_backtrace_free (ClayBacktrace *backtrace)
{
  g_return_if_fail (backtrace == NULL);
}

gint
clay_backtrace_get_n_threads (ClayBacktrace *backtrace)
{
  g_return_val_if_reached (0);
}

guintptr
clay_backtrace_get_thread_id (ClayBacktrace *backtrace,
                              gint           thread)
{
  g_return_val_if_reached (0);
}

const gchar *
clay_backtrace_get_thread_name (ClayBacktrace *backtrace,
                                gint           thread)
{
  g_return_val_if_reached (NULL);
}

gboolean
clay_backtrace_is_thread_running (ClayBacktrace *backtrace,
                                  gint           thread)
{
  g_return_val_if_reached (FALSE);
}

gint
clay_backtrace_find_thread_by_id (ClayBacktrace *backtrace,
                                  guintptr       thread_id,
                                  gint           thread_hint)
{
  g_return_val_if_reached (-1);
}

gint
clay_backtrace_get_n_frames (ClayBacktrace *backtrace,
                             gint           thread)
{
  g_return_val_if_reached (0);
}

guintptr
clay_backtrace_get_frame_address (ClayBacktrace *backtrace,
                                  gint           thread,
                                  gint           frame)
{
  g_return_val_if_reached (0);
}

gboolean
clay_backtrace_get_address_info (guintptr                  address,
                                 ClayBacktraceAddressInfo *info)
{
  return FALSE;
}


#endif /* CLAY_BACKTRACE_BACKEND_NONE */
