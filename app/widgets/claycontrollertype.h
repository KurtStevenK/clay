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

#pragma once

#include "core/clayviewable.h"


#define CLAY_TYPE_CONTROLLER_TYPE (clay_controller_type_get_type ())
G_DECLARE_FINAL_TYPE (ClayControllerType,
                      clay_controller_type,
                      CLAY, CONTROLLER_TYPE,
                      ClayViewable)


ClayControllerType * clay_controller_type_new       (GType               gtype);

GType                clay_controller_type_get_gtype (ClayControllerType *self);

GListModel         * clay_controller_type_get_model (void);
