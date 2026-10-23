/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysymmetry-tiling.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include "claysymmetry.h"


#define CLAY_TYPE_TILING            (clay_tiling_get_type ())
#define CLAY_TILING(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TILING, ClayTiling))
#define CLAY_TILING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TILING, ClayTilingClass))
#define CLAY_IS_TILING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TILING))
#define CLAY_IS_TILING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TILING))
#define CLAY_TILING_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TILING, ClayTilingClass))


typedef struct _ClayTilingClass ClayTilingClass;

struct _ClayTiling
{
  ClaySymmetry  parent_instance;

  gdouble       interval_x;
  gdouble       interval_y;
  gdouble       shift;
  gint          max_x;
  gint          max_y;
};

struct _ClayTilingClass
{
  ClaySymmetryClass  parent_class;
};


GType   clay_tiling_get_type (void);
