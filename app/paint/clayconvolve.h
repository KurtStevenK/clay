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

#include "claybrushcore.h"


#define CLAY_TYPE_CONVOLVE            (clay_convolve_get_type ())
#define CLAY_CONVOLVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONVOLVE, ClayConvolve))
#define CLAY_CONVOLVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONVOLVE, ClayConvolveClass))
#define CLAY_IS_CONVOLVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONVOLVE))
#define CLAY_IS_CONVOLVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONVOLVE))
#define CLAY_CONVOLVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONVOLVE, ClayConvolveClass))


typedef struct _ClayConvolveClass ClayConvolveClass;

struct _ClayConvolve
{
  ClayBrushCore  parent_instance;
  gfloat         matrix[9];
  gfloat         matrix_divisor;
};

struct _ClayConvolveClass
{
  ClayBrushCoreClass parent_class;
};


void    clay_convolve_register (Clay                      *clay,
                                ClayPaintRegisterCallback  callback);

GType   clay_convolve_get_type (void);
