/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationhuesaturation.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "clayoperationpointfilter.h"


#define CLAY_TYPE_OPERATION_HUE_SATURATION            (clay_operation_hue_saturation_get_type ())
#define CLAY_OPERATION_HUE_SATURATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_HUE_SATURATION, ClayOperationHueSaturation))
#define CLAY_OPERATION_HUE_SATURATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_HUE_SATURATION, ClayOperationHueSaturationClass))
#define CLAY_IS_OPERATION_HUE_SATURATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_HUE_SATURATION))
#define CLAY_IS_OPERATION_HUE_SATURATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_HUE_SATURATION))
#define CLAY_OPERATION_HUE_SATURATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_HUE_SATURATION, ClayOperationHueSaturationClass))


typedef struct _ClayOperationHueSaturation      ClayOperationHueSaturation;
typedef struct _ClayOperationHueSaturationClass ClayOperationHueSaturationClass;

struct _ClayOperationHueSaturation
{
  ClayOperationPointFilter  parent_instance;
};

struct _ClayOperationHueSaturationClass
{
  ClayOperationPointFilterClass  parent_class;
};


GType   clay_operation_hue_saturation_get_type (void);

void    clay_operation_hue_saturation_map      (ClayHueSaturationConfig *config,
                                                GeglColor               *color,
                                                ClayHueRange             range);
