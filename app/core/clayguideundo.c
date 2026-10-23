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
#include "clayimage-guides.h"
#include "clayguide.h"
#include "clayguideundo.h"


static void   clay_guide_undo_constructed  (GObject            *object);

static void   clay_guide_undo_pop          (ClayUndo            *undo,
                                            ClayUndoMode         undo_mode,
                                            ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayGuideUndo, clay_guide_undo,
               CLAY_TYPE_AUX_ITEM_UNDO)

#define parent_class clay_guide_undo_parent_class


static void
clay_guide_undo_class_init (ClayGuideUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_guide_undo_constructed;

  undo_class->pop           = clay_guide_undo_pop;
}

static void
clay_guide_undo_init (ClayGuideUndo *undo)
{
}

static void
clay_guide_undo_constructed (GObject *object)
{
  ClayGuideUndo *guide_undo = CLAY_GUIDE_UNDO (object);
  ClayGuide     *guide;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  guide = CLAY_GUIDE (CLAY_AUX_ITEM_UNDO (object)->aux_item);

  clay_assert (CLAY_IS_GUIDE (guide));

  guide_undo->orientation = clay_guide_get_orientation (guide);
  guide_undo->position    = clay_guide_get_position (guide);
}

static void
clay_guide_undo_pop (ClayUndo              *undo,
                     ClayUndoMode           undo_mode,
                     ClayUndoAccumulator   *accum)
{
  ClayGuideUndo       *guide_undo = CLAY_GUIDE_UNDO (undo);
  ClayGuide           *guide;
  ClayOrientationType  orientation;
  gint                 position;
  gboolean             moved = FALSE;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  guide = CLAY_GUIDE (CLAY_AUX_ITEM_UNDO (undo)->aux_item);

  orientation = clay_guide_get_orientation (guide);
  position    = clay_guide_get_position (guide);

  if (position == CLAY_GUIDE_POSITION_UNDEFINED)
    {
      clay_image_add_guide (undo->image, guide, guide_undo->position);
    }
  else if (guide_undo->position == CLAY_GUIDE_POSITION_UNDEFINED)
    {
      clay_image_remove_guide (undo->image, guide, FALSE);
    }
  else
    {
      clay_guide_set_position (guide, guide_undo->position);

      moved = TRUE;
    }

  clay_guide_set_orientation (guide, guide_undo->orientation);

  if (moved || guide_undo->orientation != orientation)
    clay_image_guide_moved (undo->image, guide);

  guide_undo->position    = position;
  guide_undo->orientation = orientation;
}
