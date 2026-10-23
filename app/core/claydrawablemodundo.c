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

#include "gegl/clay-gegl-utils.h"

#include "clay-memsize.h"
#include "clayimage.h"
#include "claydrawable.h"
#include "claydrawablemodundo.h"


enum
{
  PROP_0,
  PROP_COPY_BUFFER
};


static void     clay_drawable_mod_undo_constructed  (GObject             *object);
static void     clay_drawable_mod_undo_set_property (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     clay_drawable_mod_undo_get_property (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static gint64   clay_drawable_mod_undo_get_memsize  (ClayObject          *object,
                                                     gint64              *gui_size);

static void     clay_drawable_mod_undo_pop          (ClayUndo            *undo,
                                                     ClayUndoMode         undo_mode,
                                                     ClayUndoAccumulator *accum);
static void     clay_drawable_mod_undo_free         (ClayUndo            *undo,
                                                     ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayDrawableModUndo, clay_drawable_mod_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_drawable_mod_undo_parent_class


static void
clay_drawable_mod_undo_class_init (ClayDrawableModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_drawable_mod_undo_constructed;
  object_class->set_property     = clay_drawable_mod_undo_set_property;
  object_class->get_property     = clay_drawable_mod_undo_get_property;

  clay_object_class->get_memsize = clay_drawable_mod_undo_get_memsize;

  undo_class->pop                = clay_drawable_mod_undo_pop;
  undo_class->free               = clay_drawable_mod_undo_free;

  g_object_class_install_property (object_class, PROP_COPY_BUFFER,
                                   g_param_spec_boolean ("copy-buffer",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_drawable_mod_undo_init (ClayDrawableModUndo *undo)
{
}

static void
clay_drawable_mod_undo_constructed (GObject *object)
{
  ClayDrawableModUndo *drawable_mod_undo = CLAY_DRAWABLE_MOD_UNDO (object);
  ClayItem            *item;
  ClayDrawable        *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DRAWABLE (CLAY_ITEM_UNDO (object)->item));

  item     = CLAY_ITEM_UNDO (object)->item;
  drawable = CLAY_DRAWABLE (item);

  if (drawable_mod_undo->copy_buffer)
    {
      drawable_mod_undo->buffer =
        clay_gegl_buffer_dup (clay_drawable_get_buffer (drawable), drawable);
    }
  else
    {
      drawable_mod_undo->buffer =
        g_object_ref (clay_drawable_get_buffer (drawable));
    }

  clay_item_get_offset (item,
                        &drawable_mod_undo->offset_x,
                        &drawable_mod_undo->offset_y);
}

static void
clay_drawable_mod_undo_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayDrawableModUndo *drawable_mod_undo = CLAY_DRAWABLE_MOD_UNDO (object);

  switch (property_id)
    {
    case PROP_COPY_BUFFER:
      drawable_mod_undo->copy_buffer = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_mod_undo_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayDrawableModUndo *drawable_mod_undo = CLAY_DRAWABLE_MOD_UNDO (object);

  switch (property_id)
    {
    case PROP_COPY_BUFFER:
      g_value_set_boolean (value, drawable_mod_undo->copy_buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_drawable_mod_undo_get_memsize (ClayObject *object,
                                    gint64     *gui_size)
{
  ClayDrawableModUndo *drawable_mod_undo = CLAY_DRAWABLE_MOD_UNDO (object);
  gint64               memsize           = 0;

  memsize += clay_gegl_buffer_get_memsize (drawable_mod_undo->buffer);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_drawable_mod_undo_pop (ClayUndo            *undo,
                            ClayUndoMode         undo_mode,
                            ClayUndoAccumulator *accum)
{
  ClayDrawableModUndo *drawable_mod_undo = CLAY_DRAWABLE_MOD_UNDO (undo);
  ClayDrawable        *drawable          = CLAY_DRAWABLE (CLAY_ITEM_UNDO (undo)->item);
  GeglBuffer          *buffer;
  gint                 offset_x;
  gint                 offset_y;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  buffer   = drawable_mod_undo->buffer;
  offset_x = drawable_mod_undo->offset_x;
  offset_y = drawable_mod_undo->offset_y;

  drawable_mod_undo->buffer = g_object_ref (clay_drawable_get_buffer (drawable));

  clay_item_get_offset (CLAY_ITEM (drawable),
                        &drawable_mod_undo->offset_x,
                        &drawable_mod_undo->offset_y);

  clay_drawable_set_buffer_full (drawable, FALSE, NULL,
                                 buffer,
                                 GEGL_RECTANGLE (offset_x, offset_y, 0, 0),
                                 TRUE);
  g_object_unref (buffer);
}

static void
clay_drawable_mod_undo_free (ClayUndo     *undo,
                             ClayUndoMode  undo_mode)
{
  ClayDrawableModUndo *drawable_mod_undo = CLAY_DRAWABLE_MOD_UNDO (undo);

  g_clear_object (&drawable_mod_undo->buffer);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
