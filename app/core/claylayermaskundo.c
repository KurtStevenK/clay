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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clayimage.h"
#include "claylayer.h"
#include "claylayermask.h"
#include "claylayermaskundo.h"


enum
{
  PROP_0,
  PROP_LAYER_MASK,
  PROP_EDIT_MASK
};


static void     clay_layer_mask_undo_constructed  (GObject             *object);
static void     clay_layer_mask_undo_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     clay_layer_mask_undo_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint64   clay_layer_mask_undo_get_memsize  (ClayObject          *object,
                                                   gint64              *gui_size);

static void     clay_layer_mask_undo_pop          (ClayUndo            *undo,
                                                   ClayUndoMode         undo_mode,
                                                   ClayUndoAccumulator *accum);
static void     clay_layer_mask_undo_free         (ClayUndo            *undo,
                                                   ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayLayerMaskUndo, clay_layer_mask_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_layer_mask_undo_parent_class


static void
clay_layer_mask_undo_class_init (ClayLayerMaskUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_layer_mask_undo_constructed;
  object_class->set_property     = clay_layer_mask_undo_set_property;
  object_class->get_property     = clay_layer_mask_undo_get_property;

  clay_object_class->get_memsize = clay_layer_mask_undo_get_memsize;

  undo_class->pop                = clay_layer_mask_undo_pop;
  undo_class->free               = clay_layer_mask_undo_free;

  g_object_class_install_property (object_class, PROP_LAYER_MASK,
                                   g_param_spec_object ("layer-mask", NULL, NULL,
                                                        CLAY_TYPE_LAYER_MASK,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_EDIT_MASK,
                                   g_param_spec_boolean ("edit-mask", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_layer_mask_undo_init (ClayLayerMaskUndo *undo)
{
}

static void
clay_layer_mask_undo_constructed (GObject *object)
{
  ClayLayerMaskUndo *layer_mask_undo = CLAY_LAYER_MASK_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_LAYER (CLAY_ITEM_UNDO (object)->item));
  clay_assert (CLAY_IS_LAYER_MASK (layer_mask_undo->layer_mask));
}

static void
clay_layer_mask_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayLayerMaskUndo *layer_mask_undo = CLAY_LAYER_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_LAYER_MASK:
      layer_mask_undo->layer_mask = g_value_dup_object (value);
      break;
    case PROP_EDIT_MASK:
      layer_mask_undo->edit_mask = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_layer_mask_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayLayerMaskUndo *layer_mask_undo = CLAY_LAYER_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_LAYER_MASK:
      g_value_set_object (value, layer_mask_undo->layer_mask);
      break;
    case PROP_EDIT_MASK:
      g_value_set_boolean (value, layer_mask_undo->edit_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_layer_mask_undo_get_memsize (ClayObject *object,
                                  gint64     *gui_size)
{
  ClayLayerMaskUndo *layer_mask_undo = CLAY_LAYER_MASK_UNDO (object);
  ClayLayer         *layer           = CLAY_LAYER (CLAY_ITEM_UNDO (object)->item);
  gint64             memsize         = 0;

  /* don't use !clay_item_is_attached() here */
  if (clay_layer_get_mask (layer) != layer_mask_undo->layer_mask)
    memsize += clay_object_get_memsize (CLAY_OBJECT (layer_mask_undo->layer_mask),
                                        gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_layer_mask_undo_pop (ClayUndo            *undo,
                          ClayUndoMode         undo_mode,
                          ClayUndoAccumulator *accum)
{
  ClayLayerMaskUndo *layer_mask_undo = CLAY_LAYER_MASK_UNDO (undo);
  ClayLayer         *layer           = CLAY_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
       undo->undo_type == CLAY_UNDO_LAYER_MASK_ADD) ||
      (undo_mode       == CLAY_UNDO_MODE_REDO &&
       undo->undo_type == CLAY_UNDO_LAYER_MASK_REMOVE))
    {
      /*  remove layer mask  */

      layer_mask_undo->edit_mask = clay_layer_get_edit_mask (layer);
      clay_layer_apply_mask (layer, CLAY_MASK_DISCARD, FALSE);
    }
  else
    {
      /*  restore layer mask  */

      clay_layer_add_mask (layer, layer_mask_undo->layer_mask,
                           layer_mask_undo->edit_mask, FALSE, NULL);
    }
}

static void
clay_layer_mask_undo_free (ClayUndo     *undo,
                           ClayUndoMode  undo_mode)
{
  ClayLayerMaskUndo *layer_mask_undo = CLAY_LAYER_MASK_UNDO (undo);

  g_clear_object (&layer_mask_undo->layer_mask);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
