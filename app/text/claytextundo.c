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
#include "libclayconfig/clayconfig.h"

#include "text-types.h"

#include "gegl/clay-babl.h"

#include "core/clay-memsize.h"
#include "core/clayitem.h"
#include "core/clayitemundo.h"

#include "claytext.h"
#include "claytextlayer.h"
#include "claytextundo.h"


enum
{
  PROP_0,
  PROP_PARAM
};


static void     clay_text_undo_constructed  (GObject             *object);
static void     clay_text_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     clay_text_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   clay_text_undo_get_memsize  (ClayObject          *object,
                                             gint64              *gui_size);

static void     clay_text_undo_pop          (ClayUndo            *undo,
                                             ClayUndoMode         undo_mode,
                                             ClayUndoAccumulator *accum);
static void     clay_text_undo_free         (ClayUndo            *undo,
                                             ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayTextUndo, clay_text_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_text_undo_parent_class


static void
clay_text_undo_class_init (ClayTextUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_text_undo_constructed;
  object_class->set_property     = clay_text_undo_set_property;
  object_class->get_property     = clay_text_undo_get_property;

  clay_object_class->get_memsize = clay_text_undo_get_memsize;

  undo_class->pop                = clay_text_undo_pop;
  undo_class->free               = clay_text_undo_free;

  g_object_class_install_property (object_class, PROP_PARAM,
                                   g_param_spec_param ("param", NULL, NULL,
                                                       G_TYPE_PARAM,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_text_undo_init (ClayTextUndo *undo)
{
}

static void
clay_text_undo_constructed (GObject *object)
{
  ClayTextUndo  *text_undo = CLAY_TEXT_UNDO (object);
  ClayTextLayer *layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_TEXT_LAYER (CLAY_ITEM_UNDO (text_undo)->item));

  layer = CLAY_TEXT_LAYER (CLAY_ITEM_UNDO (text_undo)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_TEXT_LAYER:
      if (text_undo->pspec)
        {
          clay_assert (text_undo->pspec->owner_type == CLAY_TYPE_TEXT);

          text_undo->value = g_slice_new0 (GValue);

          g_value_init (text_undo->value, text_undo->pspec->value_type);
          g_object_get_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, text_undo->value);
        }
      else if (layer->text)
        {
          text_undo->text = clay_config_duplicate (CLAY_CONFIG (layer->text));
        }
      break;

    case CLAY_UNDO_TEXT_LAYER_CONVERT:
      text_undo->format = clay_drawable_get_format (CLAY_DRAWABLE (layer));
      break;

    default:
      clay_assert_not_reached ();
    }
}

static void
clay_text_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayTextUndo *text_undo = CLAY_TEXT_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      text_undo->pspec = g_value_get_param (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayTextUndo *text_undo = CLAY_TEXT_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      g_value_set_param (value, (GParamSpec *) text_undo->pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_text_undo_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayTextUndo *undo    = CLAY_TEXT_UNDO (object);
  gint64        memsize = 0;

  memsize += clay_g_value_get_memsize (undo->value);
  memsize += clay_object_get_memsize (CLAY_OBJECT (undo->text), NULL);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_text_undo_pop (ClayUndo            *undo,
                    ClayUndoMode         undo_mode,
                    ClayUndoAccumulator *accum)
{
  ClayTextUndo  *text_undo = CLAY_TEXT_UNDO (undo);
  ClayTextLayer *layer     = CLAY_TEXT_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_TEXT_LAYER:
      if (text_undo->pspec)
        {
          GValue *value;

          g_return_if_fail (layer->text != NULL);

          value = g_slice_new0 (GValue);
          g_value_init (value, text_undo->pspec->value_type);

          g_object_get_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, value);

          g_object_set_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, text_undo->value);

          g_value_unset (text_undo->value);
          g_slice_free (GValue, text_undo->value);

          text_undo->value = value;
        }
      else
        {
          ClayText *text;

          text = (layer->text ?
                  clay_config_duplicate (CLAY_CONFIG (layer->text)) : NULL);

          if (layer->text && text_undo->text)
            clay_config_sync (G_OBJECT (text_undo->text),
                              G_OBJECT (layer->text), 0);
          else
            clay_text_layer_set_text (layer, text_undo->text);

          if (text_undo->text)
            g_object_unref (text_undo->text);

          text_undo->text = text;
        }
      break;

    case CLAY_UNDO_TEXT_LAYER_CONVERT:
      {
        const Babl *format;

        format = clay_drawable_get_format (CLAY_DRAWABLE (layer));
        clay_drawable_convert_type (CLAY_DRAWABLE (layer),
                                    clay_item_get_image (CLAY_ITEM (layer)),
                                    clay_babl_format_get_base_type (text_undo->format),
                                    clay_babl_format_get_precision (text_undo->format),
                                    babl_format_has_alpha (text_undo->format),
                                    NULL, NULL,
                                    GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                    FALSE, NULL);
        text_undo->format = format;
      }
      break;

    default:
      clay_assert_not_reached ();
    }
}

static void
clay_text_undo_free (ClayUndo     *undo,
                     ClayUndoMode  undo_mode)
{
  ClayTextUndo *text_undo = CLAY_TEXT_UNDO (undo);

  g_clear_object (&text_undo->text);

  if (text_undo->pspec)
    {
      g_value_unset (text_undo->value);
      g_slice_free (GValue, text_undo->value);

      text_undo->value = NULL;
      text_undo->pspec = NULL;
    }

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
