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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "clayink.h"
#include "clayink-blob.h"
#include "clayinkundo.h"


static void   clay_ink_undo_constructed (GObject             *object);

static void   clay_ink_undo_pop         (ClayUndo            *undo,
                                         ClayUndoMode         undo_mode,
                                         ClayUndoAccumulator *accum);
static void   clay_ink_undo_free        (ClayUndo            *undo,
                                         ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayInkUndo, clay_ink_undo, CLAY_TYPE_PAINT_CORE_UNDO)

#define parent_class clay_ink_undo_parent_class


static void
clay_ink_undo_class_init (ClayInkUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_ink_undo_constructed;

  undo_class->pop           = clay_ink_undo_pop;
  undo_class->free          = clay_ink_undo_free;
}

static void
clay_ink_undo_init (ClayInkUndo *undo)
{
  undo->last_blobs = NULL;
}

static void
clay_ink_undo_constructed (GObject *object)
{
  ClayInkUndo *ink_undo = CLAY_INK_UNDO (object);
  ClayInk     *ink;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_INK (CLAY_PAINT_CORE_UNDO (ink_undo)->paint_core));

  ink = CLAY_INK (CLAY_PAINT_CORE_UNDO (ink_undo)->paint_core);

  if (ink->start_blobs)
    {
      gint      i;
      ClayBlob *blob;

      for (i = 0; i < g_list_length (ink->start_blobs); i++)
        {
          blob = g_list_nth_data (ink->start_blobs, i);

          ink_undo->last_blobs = g_list_prepend (ink_undo->last_blobs,
                                                 clay_blob_duplicate (blob));
        }
      ink_undo->last_blobs = g_list_reverse (ink_undo->last_blobs);
    }
}

static void
clay_ink_undo_pop (ClayUndo              *undo,
                   ClayUndoMode           undo_mode,
                   ClayUndoAccumulator   *accum)
{
  ClayInkUndo *ink_undo = CLAY_INK_UNDO (undo);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (CLAY_PAINT_CORE_UNDO (ink_undo)->paint_core)
    {
      ClayInk  *ink = CLAY_INK (CLAY_PAINT_CORE_UNDO (ink_undo)->paint_core);
      GList    *tmp_blobs;

      tmp_blobs = ink->last_blobs;
      ink->last_blobs = ink_undo->last_blobs;
      ink_undo->last_blobs = tmp_blobs;
    }
}

static void
clay_ink_undo_free (ClayUndo     *undo,
                    ClayUndoMode  undo_mode)
{
  ClayInkUndo *ink_undo = CLAY_INK_UNDO (undo);

  if (ink_undo->last_blobs)
    {
      g_list_free_full (ink_undo->last_blobs, g_free);
      ink_undo->last_blobs = NULL;
    }

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
