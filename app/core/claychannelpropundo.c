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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clayimage.h"
#include "claychannel.h"
#include "claychannelpropundo.h"


static void   clay_channel_prop_undo_constructed (GObject             *object);
static void   clay_channel_prop_undo_finalize    (GObject             *object);

static void   clay_channel_prop_undo_pop         (ClayUndo            *undo,
                                                  ClayUndoMode         undo_mode,
                                                  ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayChannelPropUndo, clay_channel_prop_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_channel_prop_undo_parent_class


static void
clay_channel_prop_undo_class_init (ClayChannelPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_channel_prop_undo_constructed;
  object_class->finalize    = clay_channel_prop_undo_finalize;

  undo_class->pop           = clay_channel_prop_undo_pop;
}

static void
clay_channel_prop_undo_init (ClayChannelPropUndo *undo)
{
}

static void
clay_channel_prop_undo_constructed (GObject *object)
{
  ClayChannelPropUndo *channel_prop_undo = CLAY_CHANNEL_PROP_UNDO (object);
  ClayChannel         *channel;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CHANNEL (CLAY_ITEM_UNDO (object)->item));

  channel = CLAY_CHANNEL (CLAY_ITEM_UNDO (object)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_CHANNEL_COLOR:
      channel_prop_undo->color = gegl_color_duplicate (clay_channel_get_color (channel));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_channel_prop_undo_finalize (GObject *object)
{
  ClayChannelPropUndo *channel_prop_undo = CLAY_CHANNEL_PROP_UNDO (object);

  g_clear_object (&channel_prop_undo->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_channel_prop_undo_pop (ClayUndo            *undo,
                            ClayUndoMode         undo_mode,
                            ClayUndoAccumulator *accum)
{
  ClayChannelPropUndo *channel_prop_undo = CLAY_CHANNEL_PROP_UNDO (undo);
  ClayChannel         *channel           = CLAY_CHANNEL (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_CHANNEL_COLOR:
      {
        GeglColor *color;

        color = gegl_color_duplicate (clay_channel_get_color (channel));
        clay_channel_set_color (channel, channel_prop_undo->color, FALSE);
        g_clear_object (&channel_prop_undo->color);
        channel_prop_undo->color = color;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
