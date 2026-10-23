/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claywaitable.h
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


#define CLAY_TYPE_WAITABLE (clay_waitable_get_type ())
G_DECLARE_INTERFACE (ClayWaitable,
                     clay_waitable,
                     CLAY, WAITABLE,
                     GObject)


struct _ClayWaitableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void       (* wait)       (ClayWaitable *waitable);
  gboolean   (* try_wait)   (ClayWaitable *waitable);
  gboolean   (* wait_until) (ClayWaitable *waitable,
                             gint64        end_time);
};


void       clay_waitable_wait       (ClayWaitable *waitable);
gboolean   clay_waitable_try_wait   (ClayWaitable *waitable);
gboolean   clay_waitable_wait_until (ClayWaitable *waitable,
                                     gint64        end_time);
gboolean   clay_waitable_wait_for   (ClayWaitable *waitable,
                                     gint64        wait_duration);
