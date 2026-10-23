/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayuncancelablewaitable.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clayuncancelablewaitable.h"
#include "claywaitable.h"


/*  local function prototypes  */

static void       clay_uncancelable_waitable_waitable_iface_init (ClayWaitableInterface   *iface);

static void       clay_uncancelable_waitable_finalize            (GObject                 *object);

static void       clay_uncancelable_waitable_wait                (ClayWaitable            *waitable);
static gboolean   clay_uncancelable_waitable_try_wait            (ClayWaitable            *waitable);
static gboolean   clay_uncancelable_waitable_wait_until          (ClayWaitable            *waitable,
                                                                  gint64                   end_time);


G_DEFINE_TYPE_WITH_CODE (ClayUncancelableWaitable, clay_uncancelable_waitable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_WAITABLE,
                                                clay_uncancelable_waitable_waitable_iface_init))

#define parent_class clay_uncancelable_waitable_parent_class


/*  private functions  */


static void
clay_uncancelable_waitable_class_init (ClayUncancelableWaitableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_uncancelable_waitable_finalize;
}

static void
clay_uncancelable_waitable_waitable_iface_init (ClayWaitableInterface *iface)
{
  iface->wait       = clay_uncancelable_waitable_wait;
  iface->try_wait   = clay_uncancelable_waitable_try_wait;
  iface->wait_until = clay_uncancelable_waitable_wait_until;
}

static void
clay_uncancelable_waitable_init (ClayUncancelableWaitable *uncancelable_waitable)
{
}

static void
clay_uncancelable_waitable_finalize (GObject *object)
{
  ClayUncancelableWaitable *uncancelable_waitable =
    CLAY_UNCANCELABLE_WAITABLE (object);

  g_clear_object (&uncancelable_waitable->waitable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_uncancelable_waitable_wait (ClayWaitable *waitable)
{
  ClayUncancelableWaitable *uncancelable_waitable =
    CLAY_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    clay_waitable_wait (uncancelable_waitable->waitable);
}

static gboolean
clay_uncancelable_waitable_try_wait (ClayWaitable *waitable)
{
  ClayUncancelableWaitable *uncancelable_waitable =
    CLAY_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    return clay_waitable_try_wait (uncancelable_waitable->waitable);
  else
    return TRUE;
}

static gboolean
clay_uncancelable_waitable_wait_until (ClayWaitable *waitable,
                                gint64        end_time)
{
  ClayUncancelableWaitable *uncancelable_waitable =
    CLAY_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    return clay_waitable_wait_until (uncancelable_waitable->waitable, end_time);
  else
    return TRUE;
}


/*  public functions  */


ClayWaitable *
clay_uncancelable_waitable_new (ClayWaitable *waitable)
{
  ClayUncancelableWaitable *uncancelable_waitable;

  g_return_val_if_fail (CLAY_IS_WAITABLE (waitable), NULL);

  uncancelable_waitable = g_object_new (CLAY_TYPE_UNCANCELABLE_WAITABLE, NULL);

  uncancelable_waitable->waitable = g_object_ref (waitable);

  return CLAY_WAITABLE (uncancelable_waitable);
}
