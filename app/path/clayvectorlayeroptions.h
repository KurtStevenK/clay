/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   clayvectorlayeroptions.h
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_VECTOR_LAYER_OPTIONS_H__
#define __CLAY_VECTOR_LAYER_OPTIONS_H__


#include "core/clayobject.h"


#define CLAY_TYPE_VECTOR_LAYER_OPTIONS            (clay_vector_layer_options_get_type ())
#define CLAY_VECTOR_LAYER_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VECTOR_LAYER_OPTIONS, ClayVectorLayerOptions))
#define CLAY_VECTOR_LAYER_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VECTOR_LAYER_OPTIONS, ClayVectorLayerOptionsClass))
#define CLAY_IS_VECTOR_LAYER_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_VECTOR_LAYER_OPTIONS))
#define CLAY_IS_VECTOR_LAYER_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VECTOR_LAYER_OPTIONS))
#define CLAY_VECTOR_LAYER_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VECTOR_LAYER_OPTIONS, ClayVectorLayerOptionsClass))


typedef struct _ClayVectorLayerOptionsClass ClayVectorLayerOptionsClass;

struct _ClayVectorLayerOptions
{
  ClayObject         parent_instance;

  Clay              *clay;

  ClayTattoo         path_tattoo;
  ClayPath          *path;

  gboolean           enable_fill;
  gboolean           enable_stroke;
  ClayFillOptions   *fill_options;
  ClayStrokeOptions *stroke_options;
};

struct _ClayVectorLayerOptionsClass
{
  ClayObjectClass  parent_class;
};

GType                    clay_vector_layer_options_get_type (void);

ClayVectorLayerOptions * clay_vector_layer_options_new      (ClayImage   *image,
                                                             ClayPath    *path,
                                                             ClayContext *context);


#endif /* __CLAY_VECTOR_LAYER_OPTIONS_H__ */
