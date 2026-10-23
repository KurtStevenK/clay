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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clayimage.h"
#include "clayimage-sample-points.h"
#include "claysamplepoint.h"
#include "claysamplepointundo.h"


static void   clay_sample_point_undo_constructed  (GObject             *object);

static void   clay_sample_point_undo_pop          (ClayUndo            *undo,
                                                   ClayUndoMode         undo_mode,
                                                   ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClaySamplePointUndo, clay_sample_point_undo,
               CLAY_TYPE_AUX_ITEM_UNDO)

#define parent_class clay_sample_point_undo_parent_class


static void
clay_sample_point_undo_class_init (ClaySamplePointUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_sample_point_undo_constructed;

  undo_class->pop           = clay_sample_point_undo_pop;
}

static void
clay_sample_point_undo_init (ClaySamplePointUndo *undo)
{
}

static void
clay_sample_point_undo_constructed (GObject *object)
{
  ClaySamplePointUndo *sample_point_undo = CLAY_SAMPLE_POINT_UNDO (object);
  ClaySamplePoint     *sample_point;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  sample_point = CLAY_SAMPLE_POINT (CLAY_AUX_ITEM_UNDO (object)->aux_item);

  clay_assert (CLAY_IS_SAMPLE_POINT (sample_point));

  clay_sample_point_get_position (sample_point,
                                  &sample_point_undo->x,
                                  &sample_point_undo->y);
  sample_point_undo->pick_mode = clay_sample_point_get_pick_mode (sample_point);
}

static void
clay_sample_point_undo_pop (ClayUndo              *undo,
                            ClayUndoMode           undo_mode,
                            ClayUndoAccumulator   *accum)
{
  ClaySamplePointUndo *sample_point_undo = CLAY_SAMPLE_POINT_UNDO (undo);
  ClaySamplePoint     *sample_point;
  gint                 x;
  gint                 y;
  ClayColorPickMode    pick_mode;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  sample_point = CLAY_SAMPLE_POINT (CLAY_AUX_ITEM_UNDO (undo)->aux_item);

  clay_sample_point_get_position (sample_point, &x, &y);
  pick_mode = clay_sample_point_get_pick_mode (sample_point);

  if (x == CLAY_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      clay_image_add_sample_point (undo->image,
                                   sample_point,
                                   sample_point_undo->x,
                                   sample_point_undo->y);
    }
  else if (sample_point_undo->x == CLAY_SAMPLE_POINT_POSITION_UNDEFINED)
    {
      clay_image_remove_sample_point (undo->image, sample_point, FALSE);
    }
  else
    {
      clay_sample_point_set_position (sample_point,
                                      sample_point_undo->x,
                                      sample_point_undo->y);
      clay_sample_point_set_pick_mode (sample_point,
                                       sample_point_undo->pick_mode);

      clay_image_sample_point_moved (undo->image, sample_point);
    }

  sample_point_undo->x         = x;
  sample_point_undo->y         = y;
  sample_point_undo->pick_mode = pick_mode;
}
