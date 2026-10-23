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

#include "clayobject.h"


#define CLAY_TYPE_DYNAMICS_OUTPUT (clay_dynamics_output_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDynamicsOutput,
                          clay_dynamics_output,
                          CLAY, DYNAMICS_OUTPUT,
                          ClayObject)


struct _ClayDynamicsOutputClass
{
  ClayObjectClass  parent_class;
};


ClayDynamicsOutput * clay_dynamics_output_new     (const gchar            *name,
                                                   ClayDynamicsOutputType  type);

gboolean   clay_dynamics_output_is_enabled        (ClayDynamicsOutput *output);

gdouble    clay_dynamics_output_get_linear_value  (ClayDynamicsOutput *output,
                                                   const ClayCoords   *coords,
                                                   ClayPaintOptions   *options,
                                                   gdouble             fade_point);

gdouble    clay_dynamics_output_get_angular_value (ClayDynamicsOutput *output,
                                                   const ClayCoords   *coords,
                                                   ClayPaintOptions   *options,
                                                   gdouble             fade_point);
gdouble    clay_dynamics_output_get_aspect_value  (ClayDynamicsOutput *output,
                                                   const ClayCoords   *coords,
                                                   ClayPaintOptions   *options,
                                                   gdouble             fade_point);
