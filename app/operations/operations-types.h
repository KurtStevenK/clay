/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * operations-types.h
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

#include <gegl-types.h>

#include "gegl/clay-gegl-types.h"

#include "operations-enums.h"


/*  operations  */

typedef struct _ClayOperationPointFilter        ClayOperationPointFilter;
typedef struct _ClayOperationLayerMode          ClayOperationLayerMode;


/*  operation config objects  */

typedef struct _ClayOperationSettings           ClayOperationSettings;

typedef struct _ClayBrightnessContrastConfig    ClayBrightnessContrastConfig;
typedef struct _ClayCageConfig                  ClayCageConfig;
typedef struct _ClayColorBalanceConfig          ClayColorBalanceConfig;
typedef struct _ClayColorizeConfig              ClayColorizeConfig;
typedef struct _ClayCurvesConfig                ClayCurvesConfig;
typedef struct _ClayDesaturateConfig            ClayDesaturateConfig;
typedef struct _ClayHueSaturationConfig         ClayHueSaturationConfig;
typedef struct _ClayLevelsConfig                ClayLevelsConfig;
typedef struct _ClayPosterizeConfig             ClayPosterizeConfig;
typedef struct _ClayThresholdConfig             ClayThresholdConfig;


/*  non-object types  */

typedef struct _ClayCagePoint                   ClayCagePoint;


/*  functions  */

typedef gboolean (* ClayLayerModeFunc)      (GeglOperation          *operation,
                                             void                   *in,
                                             void                   *aux,
                                             void                   *mask,
                                             void                   *out,
                                             glong                   samples,
                                             const GeglRectangle    *roi,
                                             gint                    level);

typedef  void    (* ClayLayerModeBlendFunc) (GeglOperation          *operation,
                                             const gfloat           *in,
                                             const gfloat           *layer,
                                             gfloat                 *out,
                                             gint                    samples);
