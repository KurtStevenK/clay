/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytriviallycancelablewaitable.c
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

#include "claycancelable.h"
#include "claytriviallycancelablewaitable.h"
#include "claywaitable.h"


/*  local function prototypes  */

static void   clay_trivially_cancelable_waitable_cancelable_iface_init (ClayCancelableInterface *iface);

static void   clay_trivially_cancelable_waitable_cancel                (ClayCancelable          *cancelable);


G_DEFINE_TYPE_WITH_CODE (ClayTriviallyCancelableWaitable, clay_trivially_cancelable_waitable, CLAY_TYPE_UNCANCELABLE_WAITABLE,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CANCELABLE,
                                                clay_trivially_cancelable_waitable_cancelable_iface_init))

#define parent_class clay_trivially_cancelable_waitable_parent_class


/*  private functions  */


static void
clay_trivially_cancelable_waitable_class_init (ClayTriviallyCancelableWaitableClass *klass)
{
}

static void
clay_trivially_cancelable_waitable_cancelable_iface_init (ClayCancelableInterface *iface)
{
  iface->cancel = clay_trivially_cancelable_waitable_cancel;
}

static void
clay_trivially_cancelable_waitable_init (ClayTriviallyCancelableWaitable *trivially_cancelable_waitable)
{
}

static void
clay_trivially_cancelable_waitable_cancel (ClayCancelable *cancelable)
{
  ClayUncancelableWaitable *uncancelable_waitable =
    CLAY_UNCANCELABLE_WAITABLE (cancelable);

  g_clear_object (&uncancelable_waitable->waitable);
}


/*  public functions  */


ClayWaitable *
clay_trivially_cancelable_waitable_new (ClayWaitable *waitable)
{
  ClayUncancelableWaitable *uncancelable_waitable;

  g_return_val_if_fail (CLAY_IS_WAITABLE (waitable), NULL);

  uncancelable_waitable = g_object_new (CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE,
                                        NULL);

  uncancelable_waitable->waitable = g_object_ref (waitable);

  return CLAY_WAITABLE (uncancelable_waitable);
}
