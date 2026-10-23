/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   clayvectorlayerundo.h
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gegl.h>

#include "libclayconfig/clayconfig.h"

#include "path-types.h"

#include "core/clay-memsize.h"
#include "core/clay-utils.h"
#include "core/clayitem.h"
#include "core/clayitemundo.h"

#include "clayvectorlayer.h"
#include "clayvectorlayeroptions.h"
#include "clayvectorlayerundo.h"

enum
{
  PROP_0,
  PROP_PARAM
};


static void     clay_vector_layer_undo_constructed  (GObject             *object);
static void     clay_vector_layer_undo_set_property (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     clay_vector_layer_undo_get_property (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);
static gint64   clay_vector_layer_undo_get_memsize  (ClayObject          *object,
                                                     gint64              *gui_size);
static void     clay_vector_layer_undo_pop          (ClayUndo            *undo,
                                                     ClayUndoMode         undo_mode,
                                                     ClayUndoAccumulator *accum);
static void     clay_vector_layer_undo_free         (ClayUndo            *undo,
                                                     ClayUndoMode         undo_mode);



G_DEFINE_TYPE (ClayVectorLayerUndo, clay_vector_layer_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_vector_layer_undo_parent_class


static void
clay_vector_layer_undo_class_init (ClayVectorLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed          = clay_vector_layer_undo_constructed;
  object_class->set_property         = clay_vector_layer_undo_set_property;
  object_class->get_property         = clay_vector_layer_undo_get_property;

  clay_object_class->get_memsize = clay_vector_layer_undo_get_memsize;

  undo_class->pop                    = clay_vector_layer_undo_pop;
  undo_class->free                   = clay_vector_layer_undo_free;

  g_object_class_install_property (object_class, PROP_PARAM,
                                   g_param_spec_param ("param", NULL, NULL,
                                                       G_TYPE_PARAM,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_vector_layer_undo_init (ClayVectorLayerUndo *undo)
{
}

static void
clay_vector_layer_undo_constructed (GObject *object)
{
  ClayVectorLayerUndo *vector_undo = CLAY_VECTOR_LAYER_UNDO (object);
  ClayVectorLayer     *vector_layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (CLAY_IS_VECTOR_LAYER (CLAY_ITEM_UNDO (vector_undo)->item));

  vector_layer = CLAY_VECTOR_LAYER (CLAY_ITEM_UNDO (vector_undo)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_VECTOR_LAYER:
      if (vector_undo->pspec)
        {
          g_assert (vector_undo->pspec->owner_type == CLAY_TYPE_VECTOR_LAYER_OPTIONS);

          vector_undo->value = g_slice_new0 (GValue);

          g_value_init (vector_undo->value, vector_undo->pspec->value_type);
          g_object_get_property (G_OBJECT (vector_layer->options),
                                 vector_undo->pspec->name, vector_undo->value);
        }
      else if (vector_layer->options)
        {
          vector_undo->vector_layer_options = clay_config_duplicate (CLAY_CONFIG (vector_layer->options));
        }
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
clay_vector_layer_undo_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayVectorLayerUndo *vector_undo = CLAY_VECTOR_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      vector_undo->pspec = g_value_get_param (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_vector_layer_undo_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayVectorLayerUndo *vector_undo = CLAY_VECTOR_LAYER_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      g_value_set_param (value, (GParamSpec *) vector_undo->pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_vector_layer_undo_get_memsize (ClayObject *object,
                                    gint64     *gui_size)
{
  ClayVectorLayerUndo *undo    = CLAY_VECTOR_LAYER_UNDO (object);
  gint64               memsize = 0;

  memsize += clay_g_value_get_memsize (undo->value);
  memsize += clay_object_get_memsize (CLAY_OBJECT (undo->vector_layer_options), NULL);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_vector_layer_undo_pop (ClayUndo            *undo,
                            ClayUndoMode         undo_mode,
                            ClayUndoAccumulator *accum)
{
  ClayVectorLayerUndo *vector_undo  = CLAY_VECTOR_LAYER_UNDO (undo);
  ClayVectorLayer     *vector_layer = CLAY_VECTOR_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_VECTOR_LAYER:
      if (vector_undo->pspec)
        {
          GValue *value;

          g_return_if_fail (vector_layer->options != NULL);

          value = g_slice_new0 (GValue);
          g_value_init (value, vector_undo->pspec->value_type);

          g_object_get_property (G_OBJECT (vector_layer->options),
                                 vector_undo->pspec->name, value);

          g_object_set_property (G_OBJECT (vector_layer->options),
                                 vector_undo->pspec->name, vector_undo->value);

          g_value_unset (vector_undo->value);
          g_slice_free (GValue, vector_undo->value);

          vector_undo->value = value;
        }
      else
        {
          ClayVectorLayerOptions *vector_layer_options;

          vector_layer_options = (vector_layer->options ?
                                  clay_config_duplicate (CLAY_CONFIG (vector_layer->options)) : NULL);

          if (vector_layer->options && vector_undo->vector_layer_options)
            clay_config_sync (G_OBJECT (vector_undo->vector_layer_options),
                              G_OBJECT (vector_layer->options), 0);
          else
            g_object_set (vector_layer,
                          "vector-layer-options", vector_undo->vector_layer_options,
                          NULL);

          if (vector_undo->vector_layer_options)
            g_object_unref (vector_undo->vector_layer_options);

          vector_undo->vector_layer_options = vector_layer_options;
        }
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
clay_vector_layer_undo_free (ClayUndo     *undo,
                             ClayUndoMode  undo_mode)
{
  ClayVectorLayerUndo *vector_undo = CLAY_VECTOR_LAYER_UNDO (undo);

  if (vector_undo->vector_layer_options)
    {
      g_object_unref (vector_undo->vector_layer_options);
      vector_undo->vector_layer_options = NULL;
    }

  if (vector_undo->pspec)
    {
      g_value_unset (vector_undo->value);
      g_slice_free (GValue, vector_undo->value);

      vector_undo->value = NULL;
      vector_undo->pspec = NULL;
    }

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
