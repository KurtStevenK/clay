/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationhslcolorlegacy.h
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


#define CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY            (clay_operation_hsl_color_legacy_get_type ())
#define CLAY_OPERATION_HSL_COLOR_LEGACY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY, ClayOperationHslColorLegacy))
#define CLAY_OPERATION_HSL_COLOR_LEGACY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY, ClayOperationHslColorLegacyClass))
#define CLAY_IS_OPERATION_HSL_COLOR_LEGACY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY))
#define CLAY_IS_OPERATION_HSL_COLOR_LEGACY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY))
#define CLAY_OPERATION_HSL_COLOR_LEGACY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY, ClayOperationHslColorLegacyClass))


typedef struct _ClayOperationHslColorLegacy      ClayOperationHslColorLegacy;
typedef struct _ClayOperationHslColorLegacyClass ClayOperationHslColorLegacyClass;

struct _ClayOperationHslColorLegacy
{
  ClayOperationLayerMode  parent_instance;
};

struct _ClayOperationHslColorLegacyClass
{
  ClayOperationLayerModeClass  parent_class;
};


GType   clay_operation_hsl_color_legacy_get_type (void);
