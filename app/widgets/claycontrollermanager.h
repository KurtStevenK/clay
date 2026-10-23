/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontrollers.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "core/claylist.h"


#define CLAY_TYPE_CONTROLLER_MANAGER (clay_controller_manager_get_type ())
G_DECLARE_FINAL_TYPE (ClayControllerManager,
                      clay_controller_manager,
                      CLAY, CONTROLLER_MANAGER,
                      ClayList)


ClayControllerManager * clay_controller_manager_new     (Clay                  *clay);

void             clay_controller_manager_restore        (ClayControllerManager *self,
                                                         ClayUIManager         *ui_manager);
void             clay_controller_manager_save           (ClayControllerManager *self);

Clay           * clay_controller_manager_get_clay       (ClayControllerManager *self);
ClayUIManager  * clay_controller_manager_get_ui_manager (ClayControllerManager *self);
ClayController * clay_controller_manager_get_wheel      (ClayControllerManager *self);
ClayController * clay_controller_manager_get_keyboard   (ClayControllerManager *self);
