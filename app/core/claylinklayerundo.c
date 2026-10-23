/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2019 Jehan
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
#include "claylink.h"
#include "claylinklayer.h"
#include "claylinklayerundo.h"


enum
{
  PROP_0,
  PROP_PREV_LINK
};


static void     clay_link_layer_undo_constructed  (GObject             *object);
static void     clay_link_layer_undo_finalize     (GObject             *object);
static void     clay_link_layer_undo_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     clay_link_layer_undo_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint64   clay_link_layer_undo_get_memsize  (ClayObject          *object,
                                                   gint64              *gui_size);

static void     clay_link_layer_undo_pop          (ClayUndo            *undo,
                                                   ClayUndoMode         undo_mode,
                                                   ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayLinkLayerUndo, clay_link_layer_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_link_layer_undo_parent_class


static void
clay_link_layer_undo_class_init (ClayLinkLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_link_layer_undo_constructed;
  object_class->finalize         = clay_link_layer_undo_finalize;
  object_class->set_property     = clay_link_layer_undo_set_property;
  object_class->get_property     = clay_link_layer_undo_get_property;

  clay_object_class->get_memsize = clay_link_layer_undo_get_memsize;

  undo_class->pop                = clay_link_layer_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_LINK,
                                   g_param_spec_object ("prev-link",
                                                        NULL, NULL,
                                                        CLAY_TYPE_LINK,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_link_layer_undo_init (ClayLinkLayerUndo *undo)
{
}

static void
clay_link_layer_undo_constructed (GObject *object)
{
  ClayLinkLayerUndo *undo = CLAY_LINK_LAYER_UNDO (object);
  ClayLinkLayer     *layer;
  ClayLink          *link;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_LINK_LAYER (CLAY_ITEM_UNDO (object)->item));

  layer = CLAY_LINK_LAYER (CLAY_ITEM_UNDO (undo)->item);

  link       = clay_link_layer_get_link (layer);
  undo->link = link ? clay_link_duplicate (link) : NULL;
  clay_link_layer_get_transform (layer,
                                 &undo->matrix,
                                 &undo->offset_x,
                                 &undo->offset_y,
                                 &undo->interpolation);
}

static void
clay_link_layer_undo_finalize (GObject *object)
{
  ClayLinkLayerUndo *undo = CLAY_LINK_LAYER_UNDO (object);

  g_clear_object (&undo->link);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_link_layer_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayLinkLayerUndo *undo = CLAY_LINK_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_LINK:
      g_clear_object (&undo->link);
      undo->link = g_value_get_object (value) ? clay_link_duplicate (g_value_get_object (value)) : NULL;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_link_layer_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayLinkLayerUndo *undo = CLAY_LINK_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_LINK:
      g_value_set_object (value, undo->link);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_link_layer_undo_get_memsize (ClayObject *object,
                                  gint64     *gui_size)
{
  ClayItemUndo *item_undo = CLAY_ITEM_UNDO (object);
  gint64        memsize   = 0;

  if (! clay_item_is_attached (item_undo->item))
    memsize += clay_object_get_memsize (CLAY_OBJECT (item_undo->item),
                                        gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_link_layer_undo_pop (ClayUndo            *undo,
                          ClayUndoMode         undo_mode,
                          ClayUndoAccumulator *accum)
{
  ClayLinkLayerUndo     *layer_undo = CLAY_LINK_LAYER_UNDO (undo);
  ClayLinkLayer         *layer      = CLAY_LINK_LAYER (CLAY_ITEM_UNDO (undo)->item);
  ClayLink              *link;
  ClayMatrix3            matrix;
  gint                   offset_x;
  gint                   offset_y;
  ClayInterpolationType  interpolation;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  link = clay_link_layer_get_link (layer);
  link = link ? g_object_ref (link) : NULL;

  clay_link_layer_get_transform (layer, &matrix, &offset_x, &offset_y, &interpolation);
  clay_link_layer_set_link_with_matrix (layer, layer_undo->link,
                                        &layer_undo->matrix,
                                        layer_undo->interpolation,
                                        layer_undo->offset_x,
                                        layer_undo->offset_y,
                                        FALSE);


  layer_undo->matrix        = matrix;
  layer_undo->interpolation = interpolation;
  layer_undo->offset_x      = offset_x;
  layer_undo->offset_y      = offset_y;

  g_clear_object (&layer_undo->link);
  layer_undo->link = link;
}
