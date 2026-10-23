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

#include "clay-memsize.h"
#include "clayimage.h"
#include "claygrouplayer.h"
#include "claygrouplayerundo.h"


static void     clay_group_layer_undo_constructed (GObject             *object);

static gint64   clay_group_layer_undo_get_memsize (ClayObject          *object,
                                                   gint64              *gui_size);

static void     clay_group_layer_undo_pop         (ClayUndo            *undo,
                                                   ClayUndoMode         undo_mode,
                                                   ClayUndoAccumulator *accum);
static void     clay_group_layer_undo_free        (ClayUndo            *undo,
                                                   ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayGroupLayerUndo, clay_group_layer_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_group_layer_undo_parent_class


static void
clay_group_layer_undo_class_init (ClayGroupLayerUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_group_layer_undo_constructed;

  clay_object_class->get_memsize = clay_group_layer_undo_get_memsize;

  undo_class->pop                = clay_group_layer_undo_pop;
  undo_class->free               = clay_group_layer_undo_free;
}

static void
clay_group_layer_undo_init (ClayGroupLayerUndo *undo)
{
}

static void
clay_group_layer_undo_constructed (GObject *object)
{
  ClayGroupLayerUndo *group_layer_undo = CLAY_GROUP_LAYER_UNDO (object);
  ClayGroupLayer     *group;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_return_if_fail (CLAY_IS_GROUP_LAYER (CLAY_ITEM_UNDO (object)->item));

  group = CLAY_GROUP_LAYER (CLAY_ITEM_UNDO (object)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE:
    case CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE:
    case CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK:
    case CLAY_UNDO_GROUP_LAYER_START_TRANSFORM:
    case CLAY_UNDO_GROUP_LAYER_END_TRANSFORM:
      break;

    case CLAY_UNDO_GROUP_LAYER_RESUME_MASK:
      group_layer_undo->mask_buffer =
        _clay_group_layer_get_suspended_mask (group,
                                              &group_layer_undo->mask_bounds);

      if (group_layer_undo->mask_buffer)
        g_object_ref (group_layer_undo->mask_buffer);
      break;

    case CLAY_UNDO_GROUP_LAYER_CONVERT:
      group_layer_undo->prev_type      = clay_drawable_get_base_type (CLAY_DRAWABLE (group));
      group_layer_undo->prev_precision = clay_drawable_get_precision (CLAY_DRAWABLE (group));
      group_layer_undo->prev_has_alpha = clay_drawable_has_alpha (CLAY_DRAWABLE (group));
      break;

    default:
      g_return_if_reached ();
    }
}

static gint64
clay_group_layer_undo_get_memsize (ClayObject *object,
                                   gint64     *gui_size)
{
  ClayGroupLayerUndo *group_layer_undo = CLAY_GROUP_LAYER_UNDO (object);
  gint64              memsize       = 0;

  memsize += clay_gegl_buffer_get_memsize (group_layer_undo->mask_buffer);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_group_layer_undo_pop (ClayUndo            *undo,
                           ClayUndoMode         undo_mode,
                           ClayUndoAccumulator *accum)
{
  ClayGroupLayerUndo *group_layer_undo = CLAY_GROUP_LAYER_UNDO (undo);
  ClayGroupLayer     *group;

  group = CLAY_GROUP_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE:
    case CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE:
      if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
           undo->undo_type == CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE) ||
          (undo_mode       == CLAY_UNDO_MODE_REDO &&
           undo->undo_type == CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE))
        {
          /*  resume group layer auto-resizing  */

          clay_group_layer_resume_resize (group, FALSE);
        }
      else
        {
          /*  suspend group layer auto-resizing  */

          clay_group_layer_suspend_resize (group, FALSE);

          if (undo->undo_type == CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE &&
              group_layer_undo->mask_buffer)
            {
              ClayLayerMask *mask = clay_layer_get_mask (CLAY_LAYER (group));

              clay_drawable_set_buffer_full (CLAY_DRAWABLE (mask),
                                             FALSE, NULL,
                                             group_layer_undo->mask_buffer,
                                             &group_layer_undo->mask_bounds,
                                             TRUE);
            }
        }
      break;

    case CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK:
    case CLAY_UNDO_GROUP_LAYER_RESUME_MASK:
      if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
           undo->undo_type == CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK) ||
          (undo_mode       == CLAY_UNDO_MODE_REDO &&
           undo->undo_type == CLAY_UNDO_GROUP_LAYER_RESUME_MASK))
        {
          /*  resume group layer mask auto-resizing  */

          clay_group_layer_resume_mask (group, FALSE);
        }
      else
        {
          /*  suspend group layer mask auto-resizing  */

          clay_group_layer_suspend_mask (group, FALSE);

          if (undo->undo_type == CLAY_UNDO_GROUP_LAYER_RESUME_MASK &&
              group_layer_undo->mask_buffer)
            {
              _clay_group_layer_set_suspended_mask (
                group,
                group_layer_undo->mask_buffer,
                &group_layer_undo->mask_bounds);
            }
        }
      break;

    case CLAY_UNDO_GROUP_LAYER_START_TRANSFORM:
    case CLAY_UNDO_GROUP_LAYER_END_TRANSFORM:
      if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
           undo->undo_type == CLAY_UNDO_GROUP_LAYER_START_TRANSFORM) ||
          (undo_mode       == CLAY_UNDO_MODE_REDO &&
           undo->undo_type == CLAY_UNDO_GROUP_LAYER_END_TRANSFORM))
        {
          /*  end group layer transform operation  */

          _clay_group_layer_end_transform (group, FALSE);
        }
      else
        {
          /*  start group layer transform operation  */

          _clay_group_layer_start_transform (group, FALSE);
        }
      break;

    case CLAY_UNDO_GROUP_LAYER_CONVERT:
      {
        ClayImageBaseType type;
        ClayPrecision     precision;
        gboolean          has_alpha;

        type      = clay_drawable_get_base_type (CLAY_DRAWABLE (group));
        precision = clay_drawable_get_precision (CLAY_DRAWABLE (group));
        has_alpha = clay_drawable_has_alpha (CLAY_DRAWABLE (group));

        clay_drawable_convert_type (CLAY_DRAWABLE (group),
                                    clay_item_get_image (CLAY_ITEM (group)),
                                    group_layer_undo->prev_type,
                                    group_layer_undo->prev_precision,
                                    group_layer_undo->prev_has_alpha,
                                    NULL, NULL,
                                    0, 0,
                                    FALSE, NULL);

        group_layer_undo->prev_type      = type;
        group_layer_undo->prev_precision = precision;
        group_layer_undo->prev_has_alpha = has_alpha;
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_group_layer_undo_free (ClayUndo     *undo,
                            ClayUndoMode  undo_mode)
{
  ClayGroupLayerUndo *group_layer_undo = CLAY_GROUP_LAYER_UNDO (undo);

  g_clear_object (&group_layer_undo->mask_buffer);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
