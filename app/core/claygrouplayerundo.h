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

#include "clayitemundo.h"


#define CLAY_TYPE_GROUP_LAYER_UNDO            (clay_group_layer_undo_get_type ())
#define CLAY_GROUP_LAYER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GROUP_LAYER_UNDO, ClayGroupLayerUndo))
#define CLAY_GROUP_LAYER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GROUP_LAYER_UNDO, ClayGroupLayerUndoClass))
#define CLAY_IS_GROUP_LAYER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GROUP_LAYER_UNDO))
#define CLAY_IS_GROUP_LAYER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GROUP_LAYER_UNDO))
#define CLAY_GROUP_LAYER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GROUP_LAYER_UNDO, ClayGroupLayerUndoClass))


typedef struct _ClayGroupLayerUndo      ClayGroupLayerUndo;
typedef struct _ClayGroupLayerUndoClass ClayGroupLayerUndoClass;

struct _ClayGroupLayerUndo
{
  ClayItemUndo       parent_instance;

  GeglBuffer        *mask_buffer;
  GeglRectangle      mask_bounds;

  ClayImageBaseType  prev_type;
  ClayPrecision      prev_precision;
  gboolean           prev_has_alpha;
};

struct _ClayGroupLayerUndoClass
{
  ClayItemUndoClass  parent_class;
};


GType   clay_group_layer_undo_get_type (void);
