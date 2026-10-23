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

#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "clay-memsize.h"
#include "claychannel.h"
#include "claymaskundo.h"


enum
{
  PROP_0,
  PROP_CONVERT_FORMAT
};


static void     clay_mask_undo_constructed  (GObject             *object);
static void     clay_mask_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     clay_mask_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   clay_mask_undo_get_memsize  (ClayObject          *object,
                                             gint64              *gui_size);

static void     clay_mask_undo_pop          (ClayUndo            *undo,
                                             ClayUndoMode         undo_mode,
                                             ClayUndoAccumulator *accum);
static void     clay_mask_undo_free         (ClayUndo            *undo,
                                             ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayMaskUndo, clay_mask_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_mask_undo_parent_class


static void
clay_mask_undo_class_init (ClayMaskUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_mask_undo_constructed;
  object_class->set_property     = clay_mask_undo_set_property;
  object_class->get_property     = clay_mask_undo_get_property;

  clay_object_class->get_memsize = clay_mask_undo_get_memsize;

  undo_class->pop                = clay_mask_undo_pop;
  undo_class->free               = clay_mask_undo_free;

  g_object_class_install_property (object_class, PROP_CONVERT_FORMAT,
                                   g_param_spec_boolean ("convert-format",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_mask_undo_init (ClayMaskUndo *undo)
{
}

static void
clay_mask_undo_constructed (GObject *object)
{
  ClayMaskUndo *mask_undo = CLAY_MASK_UNDO (object);
  ClayItem     *item;
  ClayDrawable *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CHANNEL (CLAY_ITEM_UNDO (object)->item));

  item     = CLAY_ITEM_UNDO (object)->item;
  drawable = CLAY_DRAWABLE (item);

  mask_undo->format = clay_drawable_get_format (drawable);

  if (clay_item_bounds (item,
                        &mask_undo->bounds.x,
                        &mask_undo->bounds.y,
                        &mask_undo->bounds.width,
                        &mask_undo->bounds.height))
    {
      GeglBuffer    *buffer = clay_drawable_get_buffer (drawable);
      GeglRectangle  rect;

      gegl_rectangle_align_to_buffer (&rect, &mask_undo->bounds, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      mask_undo->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                           rect.width,
                                                           rect.height),
                                           mask_undo->format);

      clay_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                             mask_undo->buffer, GEGL_RECTANGLE (0, 0, 0, 0));

      mask_undo->x = rect.x;
      mask_undo->y = rect.y;
    }
}

static void
clay_mask_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayMaskUndo *mask_undo = CLAY_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_CONVERT_FORMAT:
      mask_undo->convert_format = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_mask_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayMaskUndo *mask_undo = CLAY_MASK_UNDO (object);

  switch (property_id)
    {
    case PROP_CONVERT_FORMAT:
      g_value_set_boolean (value, mask_undo->convert_format);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_mask_undo_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayMaskUndo *mask_undo = CLAY_MASK_UNDO (object);
  gint64        memsize   = 0;

  memsize += clay_gegl_buffer_get_memsize (mask_undo->buffer);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_mask_undo_pop (ClayUndo            *undo,
                    ClayUndoMode         undo_mode,
                    ClayUndoAccumulator *accum)
{
  ClayMaskUndo  *mask_undo  = CLAY_MASK_UNDO (undo);
  ClayItem      *item       = CLAY_ITEM_UNDO (undo)->item;
  ClayDrawable  *drawable   = CLAY_DRAWABLE (item);
  ClayChannel   *channel    = CLAY_CHANNEL (item);
  GeglBuffer    *new_buffer = NULL;
  GeglRectangle  bounds     = {};
  GeglRectangle  rect       = {};
  const Babl    *format;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  format = clay_drawable_get_format (drawable);

  if (clay_item_bounds (item,
                        &bounds.x,
                        &bounds.y,
                        &bounds.width,
                        &bounds.height))
    {
      GeglBuffer *buffer = clay_drawable_get_buffer (drawable);

      gegl_rectangle_align_to_buffer (&rect, &bounds, buffer,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    rect.width, rect.height),
                                    format);

      clay_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                             new_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

      gegl_buffer_clear (buffer, &rect);
    }

  if (mask_undo->convert_format)
    {
      GeglBuffer *buffer;
      gint        width  = clay_item_get_width  (item);
      gint        height = clay_item_get_height (item);

      buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                     mask_undo->format, drawable);

      clay_drawable_set_buffer (drawable, FALSE, NULL, buffer);
      g_object_unref (buffer);
    }

  if (mask_undo->buffer)
    {
      clay_gegl_buffer_copy (mask_undo->buffer,
                             NULL,
                             GEGL_ABYSS_NONE,
                             clay_drawable_get_buffer (drawable),
                             GEGL_RECTANGLE (mask_undo->x, mask_undo->y, 0, 0));

      g_object_unref (mask_undo->buffer);
    }

  /* invalidate the current bounds and boundary of the mask */
  clay_drawable_invalidate_boundary (drawable);

  if (mask_undo->buffer)
    {
      channel->empty = FALSE;
      channel->x1    = mask_undo->bounds.x;
      channel->y1    = mask_undo->bounds.y;
      channel->x2    = mask_undo->bounds.x + mask_undo->bounds.width;
      channel->y2    = mask_undo->bounds.y + mask_undo->bounds.height;
    }
  else
    {
      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = clay_item_get_width  (item);
      channel->y2    = clay_item_get_height (item);
    }

  /* we know the bounds */
  channel->bounds_known = TRUE;

  /*  set the new mask undo parameters  */
  mask_undo->format = format;
  mask_undo->buffer = new_buffer;
  mask_undo->bounds = bounds;
  mask_undo->x      = rect.x;
  mask_undo->y      = rect.y;

  clay_drawable_update (drawable, 0, 0, -1, -1);
}

static void
clay_mask_undo_free (ClayUndo     *undo,
                     ClayUndoMode  undo_mode)
{
  ClayMaskUndo *mask_undo = CLAY_MASK_UNDO (undo);

  g_clear_object (&mask_undo->buffer);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
