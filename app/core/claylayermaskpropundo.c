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

#include "core-types.h"

#include "claylayer.h"
#include "claylayermaskpropundo.h"


static void   clay_layer_mask_prop_undo_constructed (GObject             *object);

static void   clay_layer_mask_prop_undo_pop         (ClayUndo            *undo,
                                                     ClayUndoMode         undo_mode,
                                                     ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayLayerMaskPropUndo, clay_layer_mask_prop_undo,
               CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_layer_mask_prop_undo_parent_class


static void
clay_layer_mask_prop_undo_class_init (ClayLayerMaskPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_layer_mask_prop_undo_constructed;

  undo_class->pop           = clay_layer_mask_prop_undo_pop;
}

static void
clay_layer_mask_prop_undo_init (ClayLayerMaskPropUndo *undo)
{
}

static void
clay_layer_mask_prop_undo_constructed (GObject *object)
{
  ClayLayerMaskPropUndo *layer_mask_prop_undo;
  ClayLayer             *layer;

  layer_mask_prop_undo = CLAY_LAYER_MASK_PROP_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_LAYER (CLAY_ITEM_UNDO (object)->item));

  layer = CLAY_LAYER (CLAY_ITEM_UNDO (object)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_LAYER_MASK_APPLY:
      layer_mask_prop_undo->apply = clay_layer_get_apply_mask (layer);
      break;

    case CLAY_UNDO_LAYER_MASK_SHOW:
      layer_mask_prop_undo->show = clay_layer_get_show_mask (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_layer_mask_prop_undo_pop (ClayUndo            *undo,
                               ClayUndoMode         undo_mode,
                               ClayUndoAccumulator *accum)
{
  ClayLayerMaskPropUndo *layer_mask_prop_undo = CLAY_LAYER_MASK_PROP_UNDO (undo);
  ClayLayer             *layer                = CLAY_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_LAYER_MASK_APPLY:
      {
        gboolean apply;

        apply = clay_layer_get_apply_mask (layer);
        clay_layer_set_apply_mask (layer, layer_mask_prop_undo->apply, FALSE);
        layer_mask_prop_undo->apply = apply;
      }
      break;

    case CLAY_UNDO_LAYER_MASK_SHOW:
      {
        gboolean show;

        show = clay_layer_get_show_mask (layer);
        clay_layer_set_show_mask (layer, layer_mask_prop_undo->show, FALSE);
        layer_mask_prop_undo->show = show;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
