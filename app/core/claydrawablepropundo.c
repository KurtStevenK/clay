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

#include "core-types.h"

#include "gegl/clay-gegl-utils.h"

#include "clay-memsize.h"
#include "clayimage.h"
#include "claydrawable.h"
#include "claydrawablepropundo.h"


enum
{
  PROP_0
};


static void   clay_drawable_prop_undo_constructed  (GObject             *object);
static void   clay_drawable_prop_undo_set_property (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void   clay_drawable_prop_undo_get_property (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static void   clay_drawable_prop_undo_pop          (ClayUndo            *undo,
                                                    ClayUndoMode         undo_mode,
                                                    ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayDrawablePropUndo, clay_drawable_prop_undo,
               CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_drawable_prop_undo_parent_class


static void
clay_drawable_prop_undo_class_init (ClayDrawablePropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed  = clay_drawable_prop_undo_constructed;
  object_class->set_property = clay_drawable_prop_undo_set_property;
  object_class->get_property = clay_drawable_prop_undo_get_property;

  undo_class->pop            = clay_drawable_prop_undo_pop;
}

static void
clay_drawable_prop_undo_init (ClayDrawablePropUndo *undo)
{
}

static void
clay_drawable_prop_undo_constructed (GObject *object)
{
  ClayDrawablePropUndo *drawable_prop_undo = CLAY_DRAWABLE_PROP_UNDO (object);
  ClayDrawable         *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DRAWABLE (CLAY_ITEM_UNDO (object)->item));

  drawable = CLAY_DRAWABLE (CLAY_ITEM_UNDO (object)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_DRAWABLE_FORMAT:
      drawable_prop_undo->format = clay_drawable_get_format (drawable);
      break;

    default:
      g_return_if_reached ();
   }
}

static void
clay_drawable_prop_undo_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_prop_undo_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_prop_undo_pop (ClayUndo            *undo,
                             ClayUndoMode         undo_mode,
                             ClayUndoAccumulator *accum)
{
  ClayDrawablePropUndo *drawable_prop_undo = CLAY_DRAWABLE_PROP_UNDO (undo);
  ClayDrawable         *drawable;

  drawable = CLAY_DRAWABLE (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_DRAWABLE_FORMAT:
      {
        const Babl *format = clay_drawable_get_format (drawable);

        clay_drawable_set_format (drawable,
                                  drawable_prop_undo->format,
                                  TRUE, FALSE);

        drawable_prop_undo->format = format;
      }
      break;

    default:
      g_return_if_reached ();
    }
}
