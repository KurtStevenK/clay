/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "paint-types.h"

#include "claypencil.h"
#include "claypenciloptions.h"

#include "clay-intl.h"


G_DEFINE_TYPE (ClayPencil, clay_pencil, CLAY_TYPE_PAINTBRUSH)


void
clay_pencil_register (Clay                      *clay,
                      ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_PENCIL,
                CLAY_TYPE_PENCIL_OPTIONS,
                "clay-pencil",
                _("Pencil"),
                "clay-tool-pencil");
}

static void
clay_pencil_class_init (ClayPencilClass *klass)
{
}

static void
clay_pencil_init (ClayPencil *pencil)
{
}
