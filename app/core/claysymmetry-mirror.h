/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysymmetry-mirror.h
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


#define CLAY_TYPE_MIRROR            (clay_mirror_get_type ())
#define CLAY_MIRROR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MIRROR, ClayMirror))
#define CLAY_MIRROR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MIRROR, ClayMirrorClass))
#define CLAY_IS_MIRROR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MIRROR))
#define CLAY_IS_MIRROR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MIRROR))
#define CLAY_MIRROR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MIRROR, ClayMirrorClass))


typedef struct _ClayMirrorClass ClayMirrorClass;

struct _ClayMirror
{
  ClaySymmetry  parent_instance;

  gboolean      horizontal_mirror;
  gboolean      vertical_mirror;
  gboolean      point_symmetry;
  gboolean      disable_transformation;

  gdouble       mirror_position_y;
  gdouble       mirror_position_x;
  ClayGuide    *horizontal_guide;
  ClayGuide    *vertical_guide;
};

struct _ClayMirrorClass
{
  ClaySymmetryClass  parent_class;
};


GType   clay_mirror_get_type (void);
