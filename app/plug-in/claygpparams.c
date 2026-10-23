/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claygpparams.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-paramspecs.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaybase/clayprotocol-private.h"

#include "plug-in-types.h"

#include "core/clay.h"
#include "core/claybrush.h"
#include "core/claycurve.h"
#include "core/claydisplay.h"
#include "core/claydrawablefilter.h"
#include "core/claygradient.h"
#include "core/claygrouplayer.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claylinklayer.h"
#include "core/claypalette.h"
#include "core/claypattern.h"
#include "core/clayrasterizable.h"
#include "core/clayselection.h"
#include "core/clayunit.h"

#include "path/claypath.h"
#include "path/clayvectorlayer.h"

#include "text/clayfont.h"
#include "text/claytextlayer.h"

#include "core/clayparamspecs.h"

#include "pdb/claypdb-utils.h"

#include "libclay/claygpparams.h"


/*  include the implementation, they are shared between app/ and
 *  libclay/ but need different headers.
 */
#include "../../libclay/claygpparams-body.c"
