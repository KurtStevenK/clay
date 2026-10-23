/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   clayvectorlayer.h
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

#ifndef __CLAY_VECTOR_LAYER_H__
#define __CLAY_VECTOR_LAYER_H__


#include "core/claylayer.h"


#define CLAY_TYPE_VECTOR_LAYER            (clay_vector_layer_get_type ())
#define CLAY_VECTOR_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VECTOR_LAYER, ClayVectorLayer))
#define CLAY_VECTOR_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VECTOR_LAYER, ClayVectorLayerClass))
#define CLAY_IS_VECTOR_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_VECTOR_LAYER))
#define CLAY_IS_VECTOR_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VECTOR_LAYER))
#define CLAY_VECTOR_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VECTOR_LAYER, ClayVectorLayerClass))


typedef struct _ClayVectorLayerClass ClayVectorLayerClass;

struct _ClayVectorLayer
{
  ClayLayer               parent_instance;

  ClayVectorLayerOptions *options;
};

struct _ClayVectorLayerClass
{
  ClayLayerClass  parent_class;
};


GType                     clay_vector_layer_get_type    (void);

ClayVectorLayer         * clay_vector_layer_new         (ClayImage       *image,
                                                         ClayPath        *path,
                                                         ClayContext     *context);

void                      clay_vector_layer_set_path    (ClayVectorLayer *layer,
                                                         ClayPath        *path,
                                                         gboolean         push_undo);
ClayPath                * clay_vector_layer_get_path    (ClayVectorLayer *layer);

ClayVectorLayerOptions  * clay_vector_layer_get_options (ClayVectorLayer *layer);

void                      clay_vector_layer_set         (ClayVectorLayer *layer,
                                                         const gchar     *undo_desc,
                                                         const gchar     *first_property_name,
                                                         ...) G_GNUC_NULL_TERMINATED;

void                      clay_vector_layer_refresh     (ClayVectorLayer *layer);

gboolean                  clay_item_is_vector_layer     (ClayItem        *item);


#endif /* __CLAY_VECTOR_LAYER_H__ */
