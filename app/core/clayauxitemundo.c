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

#include "clayauxitem.h"
#include "clayauxitemundo.h"


enum
{
  PROP_0,
  PROP_AUX_ITEM
};


static void   clay_aux_item_undo_constructed  (GObject      *object);
static void   clay_aux_item_undo_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   clay_aux_item_undo_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static void   clay_aux_item_undo_free         (ClayUndo     *undo,
                                               ClayUndoMode  undo_mode);


G_DEFINE_ABSTRACT_TYPE (ClayAuxItemUndo, clay_aux_item_undo, CLAY_TYPE_UNDO)

#define parent_class clay_aux_item_undo_parent_class


static void
clay_aux_item_undo_class_init (ClayAuxItemUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed  = clay_aux_item_undo_constructed;
  object_class->set_property = clay_aux_item_undo_set_property;
  object_class->get_property = clay_aux_item_undo_get_property;

  undo_class->free           = clay_aux_item_undo_free;

  g_object_class_install_property (object_class, PROP_AUX_ITEM,
                                   g_param_spec_object ("aux-item", NULL, NULL,
                                                        CLAY_TYPE_AUX_ITEM,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_aux_item_undo_init (ClayAuxItemUndo *undo)
{
}

static void
clay_aux_item_undo_constructed (GObject *object)
{
  ClayAuxItemUndo *aux_item_undo = CLAY_AUX_ITEM_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_AUX_ITEM (aux_item_undo->aux_item));
}

static void
clay_aux_item_undo_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayAuxItemUndo *aux_item_undo = CLAY_AUX_ITEM_UNDO (object);

  switch (property_id)
    {
    case PROP_AUX_ITEM:
      aux_item_undo->aux_item = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_aux_item_undo_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayAuxItemUndo *aux_item_undo = CLAY_AUX_ITEM_UNDO (object);

  switch (property_id)
    {
    case PROP_AUX_ITEM:
      g_value_set_object (value, aux_item_undo->aux_item);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_aux_item_undo_free (ClayUndo     *undo,
                         ClayUndoMode  undo_mode)
{
  ClayAuxItemUndo *aux_item_undo = CLAY_AUX_ITEM_UNDO (undo);

  g_clear_object (&aux_item_undo->aux_item);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
