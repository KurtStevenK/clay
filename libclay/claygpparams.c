/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * claygpparams.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-paramspecs.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaybase/clayprotocol-private.h"

#include "clay.h"
#include "claygpparams.h"


/*  include the implementation, they are shared between app/ and
 *  libclay/ but need different headers.
 */
#define LIBCLAY_COMPILATION
#include "claygpparams-body.c"
#undef LIBCLAY_COMPILATION
