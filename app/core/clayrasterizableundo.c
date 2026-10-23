/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrasterizableundo.c
 * Copyright (C) 2025 Jehan
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

#include "core-types.h"

#include "clayrasterizable.h"
#include "clayrasterizableundo.h"


struct _ClayRasterizableUndo
{
  ClayItemUndo      parent_instance;

  gboolean          rasterized;
};


static void     clay_rasterizable_undo_constructed  (GObject             *object);

static void     clay_rasterizable_undo_pop          (ClayUndo            *undo,
                                                     ClayUndoMode         undo_mode,
                                                     ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayRasterizableUndo, clay_rasterizable_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_rasterizable_undo_parent_class


static void
clay_rasterizable_undo_class_init (ClayRasterizableUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_rasterizable_undo_constructed;
  undo_class->pop           = clay_rasterizable_undo_pop;
}

static void
clay_rasterizable_undo_init (ClayRasterizableUndo *undo)
{
}

static void
clay_rasterizable_undo_constructed (GObject *object)
{
  ClayRasterizableUndo  *undo = CLAY_RASTERIZABLE_UNDO (object);
  ClayRasterizable      *rasterizable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_RASTERIZABLE (CLAY_ITEM_UNDO (undo)->item));

  rasterizable = CLAY_RASTERIZABLE (CLAY_ITEM_UNDO (undo)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_RASTERIZABLE:
      undo->rasterized = clay_rasterizable_is_rasterized (rasterizable);
      break;

    default:
      clay_assert_not_reached ();
    }
}

static void
clay_rasterizable_undo_pop (ClayUndo            *undo,
                            ClayUndoMode         undo_mode,
                            ClayUndoAccumulator *accum)
{
  ClayRasterizableUndo *rundo        = CLAY_RASTERIZABLE_UNDO (undo);
  ClayRasterizable     *rasterizable = CLAY_RASTERIZABLE (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_RASTERIZABLE:
      {
        gboolean rasterized = clay_rasterizable_is_rasterized (rasterizable);

        clay_rasterizable_set_undo_rasterized (rasterizable, rundo->rasterized);
        rundo->rasterized = rasterized;

        CLAY_RASTERIZABLE_GET_IFACE (rasterizable)->set_rasterized (rasterizable, ! rasterized);

        clay_viewable_invalidate_preview (CLAY_VIEWABLE (rasterizable));
      }
      break;

    default:
      clay_assert_not_reached ();
    }
}
