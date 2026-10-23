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
#include "claylayer.h"
#include "claylayerpropundo.h"


static void   clay_layer_prop_undo_constructed (GObject             *object);

static void   clay_layer_prop_undo_pop         (ClayUndo            *undo,
                                                ClayUndoMode         undo_mode,
                                                ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayLayerPropUndo, clay_layer_prop_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_layer_prop_undo_parent_class


static void
clay_layer_prop_undo_class_init (ClayLayerPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_layer_prop_undo_constructed;

  undo_class->pop           = clay_layer_prop_undo_pop;
}

static void
clay_layer_prop_undo_init (ClayLayerPropUndo *undo)
{
}

static void
clay_layer_prop_undo_constructed (GObject *object)
{
  ClayLayerPropUndo *layer_prop_undo = CLAY_LAYER_PROP_UNDO (object);
  ClayLayer         *layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_LAYER (CLAY_ITEM_UNDO (object)->item));

  layer = CLAY_LAYER (CLAY_ITEM_UNDO (object)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_LAYER_MODE:
      layer_prop_undo->mode            = clay_layer_get_mode (layer);
      layer_prop_undo->blend_space     = clay_layer_get_blend_space (layer);
      layer_prop_undo->composite_space = clay_layer_get_composite_space (layer);
      layer_prop_undo->composite_mode  = clay_layer_get_composite_mode (layer);
      break;

    case CLAY_UNDO_LAYER_OPACITY:
      layer_prop_undo->opacity = clay_layer_get_opacity (layer);
      break;

    case CLAY_UNDO_LAYER_LOCK_ALPHA:
      layer_prop_undo->lock_alpha = clay_layer_get_lock_alpha (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_layer_prop_undo_pop (ClayUndo            *undo,
                          ClayUndoMode         undo_mode,
                          ClayUndoAccumulator *accum)
{
  ClayLayerPropUndo *layer_prop_undo = CLAY_LAYER_PROP_UNDO (undo);
  ClayLayer         *layer           = CLAY_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_LAYER_MODE:
      {
        ClayLayerMode          mode;
        ClayLayerColorSpace    blend_space;
        ClayLayerColorSpace    composite_space;
        ClayLayerCompositeMode composite_mode;

        mode            = clay_layer_get_mode (layer);
        blend_space     = clay_layer_get_blend_space (layer);
        composite_space = clay_layer_get_composite_space (layer);
        composite_mode  = clay_layer_get_composite_mode (layer);

        clay_layer_set_mode            (layer, layer_prop_undo->mode,
                                        FALSE);
        clay_layer_set_blend_space     (layer, layer_prop_undo->blend_space,
                                        FALSE);
        clay_layer_set_composite_space (layer, layer_prop_undo->composite_space,
                                        FALSE);
        clay_layer_set_composite_mode  (layer, layer_prop_undo->composite_mode,
                                        FALSE);

        layer_prop_undo->mode            = mode;
        layer_prop_undo->blend_space     = blend_space;
        layer_prop_undo->composite_space = composite_space;
        layer_prop_undo->composite_mode  = composite_mode;
      }
      break;

    case CLAY_UNDO_LAYER_OPACITY:
      {
        gdouble opacity;

        opacity = clay_layer_get_opacity (layer);
        clay_layer_set_opacity (layer, layer_prop_undo->opacity, FALSE);
        layer_prop_undo->opacity = opacity;
      }
      break;

    case CLAY_UNDO_LAYER_LOCK_ALPHA:
      {
        gboolean lock_alpha;

        lock_alpha = clay_layer_get_lock_alpha (layer);
        clay_layer_set_lock_alpha (layer, layer_prop_undo->lock_alpha, FALSE);
        layer_prop_undo->lock_alpha = lock_alpha;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
