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

#include "core/core-types.h"
#include "paint/paint-enums.h"


/*  paint cores  */

typedef struct _ClayPaintCore        ClayPaintCore;
typedef struct _ClayBrushCore        ClayBrushCore;
typedef struct _ClaySourceCore       ClaySourceCore;

typedef struct _ClayAirbrush         ClayAirbrush;
typedef struct _ClayClone            ClayClone;
typedef struct _ClayConvolve         ClayConvolve;
typedef struct _ClayDodgeBurn        ClayDodgeBurn;
typedef struct _ClayEraser           ClayEraser;
typedef struct _ClayHeal             ClayHeal;
typedef struct _ClayInk              ClayInk;
typedef struct _ClayMybrushCore      ClayMybrushCore;
typedef struct _ClayPaintbrush       ClayPaintbrush;
typedef struct _ClayPencil           ClayPencil;
typedef struct _ClayPerspectiveClone ClayPerspectiveClone;
typedef struct _ClaySmudge           ClaySmudge;


/*  paint options  */

typedef struct _ClayPaintOptions            ClayPaintOptions;
typedef struct _ClaySourceOptions           ClaySourceOptions;

typedef struct _ClayAirbrushOptions         ClayAirbrushOptions;
typedef struct _ClayCloneOptions            ClayCloneOptions;
typedef struct _ClayConvolveOptions         ClayConvolveOptions;
typedef struct _ClayDodgeBurnOptions        ClayDodgeBurnOptions;
typedef struct _ClayEraserOptions           ClayEraserOptions;
typedef struct _ClayInkOptions              ClayInkOptions;
typedef struct _ClayMybrushOptions          ClayMybrushOptions;
typedef struct _ClayPencilOptions           ClayPencilOptions;
typedef struct _ClayPerspectiveCloneOptions ClayPerspectiveCloneOptions;
typedef struct _ClaySmudgeOptions           ClaySmudgeOptions;


/*  functions  */

typedef void (* ClayPaintRegisterCallback) (Clay        *clay,
                                            GType        paint_type,
                                            GType        paint_options_type,
                                            const gchar *identifier,
                                            const gchar *blurb,
                                            const gchar *icon_name);

typedef void (* ClayPaintRegisterFunc)     (Clay                      *clay,
                                            ClayPaintRegisterCallback  callback);
