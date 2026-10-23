/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   clayvectorlayerundo.h
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

#ifndef __CLAY_VECTOR_LAYER_UNDO_H__
#define __CLAY_VECTOR_LAYER_UNDO_H__


#include "core/clayitemundo.h"


#define CLAY_TYPE_VECTOR_LAYER_UNDO            (clay_vector_layer_undo_get_type ())
#define CLAY_VECTOR_LAYER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VECTOR_LAYER_UNDO, ClayVectorLayerUndo))
#define CLAY_VECTOR_LAYER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VECTOR_LAYER_UNDO, ClayVectorLayerUndoClass))
#define CLAY_IS_VECTOR_LAYER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_VECTOR_LAYER_UNDO))
#define CLAY_IS_VECTOR_LAYER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VECTOR_LAYER_UNDO))
#define CLAY_VECTOR_LAYER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VECTOR_LAYER_UNDO, ClayVectorLayerUndoClass))


typedef struct _ClayVectorLayerUndoClass ClayVectorLayerUndoClass;

struct _ClayVectorLayerUndo
{
  ClayItemUndo            parent_instance;

  ClayVectorLayerOptions *vector_layer_options;
  const GParamSpec       *pspec;
  GValue                 *value;
};

struct _ClayVectorLayerUndoClass
{
  ClayItemClass  parent_class;
};


GType      clay_vector_layer_undo_get_type (void);


#endif /* __CLAY_VECTOR_LAYER_UNDO_H__ */
