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

#include "clayauxitemundo.h"


#define CLAY_TYPE_SAMPLE_POINT_UNDO            (clay_sample_point_undo_get_type ())
#define CLAY_SAMPLE_POINT_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SAMPLE_POINT_UNDO, ClaySamplePointUndo))
#define CLAY_SAMPLE_POINT_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SAMPLE_POINT_UNDO, ClaySamplePointUndoClass))
#define CLAY_IS_SAMPLE_POINT_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SAMPLE_POINT_UNDO))
#define CLAY_IS_SAMPLE_POINT_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SAMPLE_POINT_UNDO))
#define CLAY_SAMPLE_POINT_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SAMPLE_POINT_UNDO, ClaySamplePointUndoClass))


typedef struct _ClaySamplePointUndo      ClaySamplePointUndo;
typedef struct _ClaySamplePointUndoClass ClaySamplePointUndoClass;

struct _ClaySamplePointUndo
{
  ClayAuxItemUndo    parent_instance;

  gint               x;
  gint               y;
  ClayColorPickMode  pick_mode;
};

struct _ClaySamplePointUndoClass
{
  ClayAuxItemUndoClass  parent_class;
};


GType   clay_sample_point_undo_get_type (void);
