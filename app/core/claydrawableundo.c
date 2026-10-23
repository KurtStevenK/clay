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

#include "clay-memsize.h"
#include "clayimage.h"
#include "claydrawable.h"
#include "claydrawable-filters.h"
#include "claydrawableundo.h"


enum
{
  PROP_0,
  PROP_BUFFER,
  PROP_X,
  PROP_Y
};


static void     clay_drawable_undo_constructed  (GObject             *object);
static void     clay_drawable_undo_set_property (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void     clay_drawable_undo_get_property (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static gint64   clay_drawable_undo_get_memsize  (ClayObject          *object,
                                                 gint64              *gui_size);

static void     clay_drawable_undo_pop          (ClayUndo            *undo,
                                                 ClayUndoMode         undo_mode,
                                                 ClayUndoAccumulator *accum);
static void     clay_drawable_undo_free         (ClayUndo            *undo,
                                                 ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayDrawableUndo, clay_drawable_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_drawable_undo_parent_class


static void
clay_drawable_undo_class_init (ClayDrawableUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_drawable_undo_constructed;
  object_class->set_property     = clay_drawable_undo_set_property;
  object_class->get_property     = clay_drawable_undo_get_property;

  clay_object_class->get_memsize = clay_drawable_undo_get_memsize;

  undo_class->pop                = clay_drawable_undo_pop;
  undo_class->free               = clay_drawable_undo_free;

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer", NULL, NULL,
                                                        GEGL_TYPE_BUFFER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x", NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE, CLAY_MAX_IMAGE_SIZE, 0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y", NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE, CLAY_MAX_IMAGE_SIZE, 0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_drawable_undo_init (ClayDrawableUndo *undo)
{
}

static void
clay_drawable_undo_constructed (GObject *object)
{
  ClayDrawableUndo *drawable_undo = CLAY_DRAWABLE_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DRAWABLE (CLAY_ITEM_UNDO (object)->item));
  clay_assert (GEGL_IS_BUFFER (drawable_undo->buffer));
}

static void
clay_drawable_undo_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayDrawableUndo *drawable_undo = CLAY_DRAWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      drawable_undo->buffer = g_value_dup_object (value);
      break;
    case PROP_X:
      drawable_undo->x = g_value_get_int (value);
      break;
    case PROP_Y:
      drawable_undo->y = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_undo_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayDrawableUndo *drawable_undo = CLAY_DRAWABLE_UNDO (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, drawable_undo->buffer);
      break;
    case PROP_X:
      g_value_set_int (value, drawable_undo->x);
      break;
    case PROP_Y:
      g_value_set_int (value, drawable_undo->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_drawable_undo_get_memsize (ClayObject *object,
                                gint64     *gui_size)
{
  ClayDrawableUndo *drawable_undo = CLAY_DRAWABLE_UNDO (object);
  gint64            memsize       = 0;

  memsize += clay_gegl_buffer_get_memsize (drawable_undo->buffer);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_drawable_undo_pop (ClayUndo            *undo,
                        ClayUndoMode         undo_mode,
                        ClayUndoAccumulator *accum)
{
  ClayDrawableUndo *drawable_undo = CLAY_DRAWABLE_UNDO (undo);
  ClayDrawable     *drawable      = CLAY_DRAWABLE (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  clay_drawable_swap_pixels (drawable,
                             drawable_undo->buffer,
                             drawable_undo->x,
                             drawable_undo->y);

  if (clay_drawable_has_visible_filters (drawable))
    clay_drawable_update (drawable, 0, 0, -1, -1);
}

static void
clay_drawable_undo_free (ClayUndo     *undo,
                         ClayUndoMode  undo_mode)
{
  ClayDrawableUndo *drawable_undo = CLAY_DRAWABLE_UNDO (undo);

  g_clear_object (&drawable_undo->buffer);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
