/* Clay - The GNU Image Manipulation Program
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

#include "path-types.h"

#include "core/clayimage.h"

#include "claypath.h"
#include "claypathpropundo.h"


static void   clay_path_prop_undo_constructed (GObject             *object);

static void   clay_path_prop_undo_pop         (ClayUndo            *undo,
                                               ClayUndoMode         undo_mode,
                                               ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayPathPropUndo, clay_path_prop_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_path_prop_undo_parent_class


static void
clay_path_prop_undo_class_init (ClayPathPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_path_prop_undo_constructed;

  undo_class->pop           = clay_path_prop_undo_pop;
}

static void
clay_path_prop_undo_init (ClayPathPropUndo *undo)
{
}

static void
clay_path_prop_undo_constructed (GObject *object)
{
  /* ClayPath *path; */

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_PATH (CLAY_ITEM_UNDO (object)->item));

  /* path = CLAY_PATH (CLAY_ITEM_UNDO (object)->item); */

  switch (CLAY_UNDO (object)->undo_type)
    {
    default:
      clay_assert_not_reached ();
    }
}

static void
clay_path_prop_undo_pop (ClayUndo            *undo,
                         ClayUndoMode         undo_mode,
                         ClayUndoAccumulator *accum)
{
#if 0
  ClayPathPropUndo *path_prop_undo = CLAY_PATH_PROP_UNDO (undo);
  ClayPath         *path           = CLAY_PATH (CLAY_ITEM_UNDO (undo)->item);
#endif

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    default:
      clay_assert_not_reached ();
    }
}
