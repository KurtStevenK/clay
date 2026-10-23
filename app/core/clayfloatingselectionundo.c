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

#include "claydrawable-floating-selection.h"
#include "clayfloatingselectionundo.h"
#include "clayimage.h"
#include "claylayer.h"
#include "claylayer-floating-selection.h"


static void   clay_floating_selection_undo_constructed (GObject             *object);

static void   clay_floating_selection_undo_pop         (ClayUndo            *undo,
                                                        ClayUndoMode         undo_mode,
                                                        ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayFloatingSelectionUndo, clay_floating_selection_undo,
               CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_floating_selection_undo_parent_class


static void
clay_floating_selection_undo_class_init (ClayFloatingSelectionUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed = clay_floating_selection_undo_constructed;

  undo_class->pop           = clay_floating_selection_undo_pop;
}

static void
clay_floating_selection_undo_init (ClayFloatingSelectionUndo *undo)
{
}

static void
clay_floating_selection_undo_constructed (GObject *object)
{
  ClayFloatingSelectionUndo *floating_sel_undo;
  ClayLayer                 *layer;

  floating_sel_undo = CLAY_FLOATING_SELECTION_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_LAYER (CLAY_ITEM_UNDO (object)->item));

  layer = CLAY_LAYER (CLAY_ITEM_UNDO (object)->item);

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_FS_TO_LAYER:
      floating_sel_undo->drawable = clay_layer_get_floating_sel_drawable (layer);
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_floating_selection_undo_pop (ClayUndo            *undo,
                                  ClayUndoMode         undo_mode,
                                  ClayUndoAccumulator *accum)
{
  ClayFloatingSelectionUndo *floating_sel_undo;
  ClayLayer                 *floating_layer;

  floating_sel_undo = CLAY_FLOATING_SELECTION_UNDO (undo);
  floating_layer    = CLAY_LAYER (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_FS_TO_LAYER:
      if (undo_mode == CLAY_UNDO_MODE_UNDO)
        {
          GList *layers;

          /*  Update the preview for the floating selection  */
          clay_viewable_invalidate_preview (CLAY_VIEWABLE (floating_layer));

          clay_layer_set_floating_sel_drawable (floating_layer,
                                                floating_sel_undo->drawable);
          layers = g_list_prepend (NULL, floating_layer);
          clay_image_set_selected_layers (undo->image, layers);
          g_list_free (layers);

          clay_drawable_attach_floating_sel (clay_layer_get_floating_sel_drawable (floating_layer),
                                             floating_layer);
        }
      else
        {
          clay_drawable_detach_floating_sel (clay_layer_get_floating_sel_drawable (floating_layer));
          clay_layer_set_floating_sel_drawable (floating_layer, NULL);
        }

      /* When the floating selection is converted to/from a normal
       * layer it does something resembling a name change, so emit the
       * "name-changed" signal
       */
      clay_object_name_changed (CLAY_OBJECT (floating_layer));

      clay_drawable_update (CLAY_DRAWABLE (floating_layer),
                            0, 0,
                            clay_item_get_width  (CLAY_ITEM (floating_layer)),
                            clay_item_get_height (CLAY_ITEM (floating_layer)));
      break;

    default:
      g_return_if_reached ();
    }
}
