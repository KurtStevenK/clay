/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2019 Jehan
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


#define CLAY_TYPE_LINK_LAYER_UNDO            (clay_link_layer_undo_get_type ())
#define CLAY_LINK_LAYER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LINK_LAYER_UNDO, ClayLinkLayerUndo))
#define CLAY_LINK_LAYER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LINK_LAYER_UNDO, ClayLinkLayerUndoClass))
#define CLAY_IS_LINK_LAYER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LINK_LAYER_UNDO))
#define CLAY_IS_LINK_LAYER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LINK_LAYER_UNDO))
#define CLAY_LINK_LAYER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LINK_LAYER_UNDO, ClayLinkLayerUndoClass))


typedef struct _ClayLinkLayerUndo      ClayLinkLayerUndo;
typedef struct _ClayLinkLayerUndoClass ClayLinkLayerUndoClass;

struct _ClayLinkLayerUndo
{
  ClayItemUndo           parent_instance;

  ClayLink              *link;
  ClayMatrix3            matrix;
  gint                   offset_x;
  gint                   offset_y;
  ClayInterpolationType  interpolation;
};

struct _ClayLinkLayerUndoClass
{
  ClayItemUndoClass      parent_class;
};


GType   clay_link_layer_undo_get_type (void);
