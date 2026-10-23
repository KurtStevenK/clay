/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytriviallycancelablewaitable.h
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

#include "clayuncancelablewaitable.h"


#define CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE            (clay_trivially_cancelable_waitable_get_type ())
#define CLAY_TRIVIALLY_CANCELABLE_WAITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE, ClayTriviallyCancelableWaitable))
#define CLAY_TRIVIALLY_CANCELABLE_WAITABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE, ClayTriviallyCancelableWaitableClass))
#define CLAY_IS_TRIVIALLY_CANCELABLE_WAITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE))
#define CLAY_IS_TRIVIALLY_CANCELABLE_WAITABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE))
#define CLAY_TRIVIALLY_CANCELABLE_WAITABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TRIVIALLY_CANCELABLE_WAITABLE, ClayTriviallyCancelableWaitableClass))


typedef struct _ClayTriviallyCancelableWaitablePrivate ClayTriviallyCancelableWaitablePrivate;
typedef struct _ClayTriviallyCancelableWaitableClass   ClayTriviallyCancelableWaitableClass;

struct _ClayTriviallyCancelableWaitable
{
  ClayUncancelableWaitable                parent_instance;

  ClayTriviallyCancelableWaitablePrivate *priv;
};

struct _ClayTriviallyCancelableWaitableClass
{
  ClayUncancelableWaitableClass  parent_class;
};


GType          clay_trivially_cancelable_waitable_get_type (void);

ClayWaitable * clay_trivially_cancelable_waitable_new      (ClayWaitable *waitable);
