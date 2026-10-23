/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationhsvhuelegacy.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include "operations/layer-modes/clayoperationlayermode.h"


#define CLAY_TYPE_OPERATION_HSV_HUE_LEGACY            (clay_operation_hsv_hue_legacy_get_type ())
#define CLAY_OPERATION_HSV_HUE_LEGACY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_HSV_HUE_LEGACY, ClayOperationHsvHueLegacy))
#define CLAY_OPERATION_HSV_HUE_LEGACY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_HSV_HUE_LEGACY, ClayOperationHsvHueLegacyClass))
#define CLAY_IS_OPERATION_HSV_HUE_LEGACY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_HSV_HUE_LEGACY))
#define CLAY_IS_OPERATION_HSV_HUE_LEGACY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_HSV_HUE_LEGACY))
#define CLAY_OPERATION_HSV_HUE_LEGACY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_HSV_HUE_LEGACY, ClayOperationHsvHueLegacyClass))


typedef struct _ClayOperationHsvHueLegacy      ClayOperationHsvHueLegacy;
typedef struct _ClayOperationHsvHueLegacyClass ClayOperationHsvHueLegacyClass;

struct _ClayOperationHsvHueLegacy
{
  ClayOperationLayerMode  parent_instance;
};

struct _ClayOperationHsvHueLegacyClass
{
  ClayOperationLayerModeClass  parent_class;
};


GType   clay_operation_hsv_hue_legacy_get_type (void);
