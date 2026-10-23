/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "claydata.h"


#define CLAY_TYPE_DYNAMICS (clay_dynamics_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDynamics,
                          clay_dynamics,
                          CLAY, DYNAMICS,
                          ClayData)


struct _ClayDynamicsClass
{
  ClayDataClass  parent_class;
};


ClayData           * clay_dynamics_new          (ClayContext            *context,
                                                 const gchar            *name);
ClayData           * clay_dynamics_get_standard (ClayContext            *context);

ClayDynamicsOutput * clay_dynamics_get_output   (ClayDynamics           *dynamics,
                                                 ClayDynamicsOutputType  type);

gboolean        clay_dynamics_is_output_enabled (ClayDynamics           *dynamics,
                                                 ClayDynamicsOutputType  type);

gdouble         clay_dynamics_get_linear_value  (ClayDynamics           *dynamics,
                                                 ClayDynamicsOutputType  type,
                                                 const ClayCoords       *coords,
                                                 ClayPaintOptions       *options,
                                                 gdouble                 fade_point);

gdouble         clay_dynamics_get_angular_value (ClayDynamics           *dynamics,
                                                 ClayDynamicsOutputType  type,
                                                 const ClayCoords       *coords,
                                                 ClayPaintOptions       *options,
                                                 gdouble                 fade_point);

gdouble         clay_dynamics_get_aspect_value  (ClayDynamics           *dynamics,
                                                 ClayDynamicsOutputType  type,
                                                 const ClayCoords       *coords,
                                                 ClayPaintOptions       *options,
                                                 gdouble                 fade_point);
