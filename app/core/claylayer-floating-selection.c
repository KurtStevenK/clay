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

#include "clayboundary.h"
#include "claydrawable-filters.h"
#include "claydrawable-floating-selection.h"
#include "clayerror.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer.h"
#include "claylayer-floating-selection.h"
#include "claylayermask.h"
#include "clayselection.h"

#include "clay-intl.h"


/* public functions  */

void
floating_sel_attach (ClayLayer    *layer,
                     ClayDrawable *drawable)
{
  ClayImage *image;
  ClayLayer *floating_sel;
  ClayLayer *parent   = NULL;
  gint       position = 0;

  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (drawable != CLAY_DRAWABLE (layer));
  g_return_if_fail (clay_item_get_image (CLAY_ITEM (layer)) ==
                    clay_item_get_image (CLAY_ITEM (drawable)));

  image = clay_item_get_image (CLAY_ITEM (drawable));

  floating_sel = clay_image_get_floating_selection (image);

  /*  If there is already a floating selection, anchor it  */
  if (floating_sel)
    {
      floating_sel_anchor (floating_sel);

      /*  if we were pasting to the old floating selection, paste now
       *  to the drawable
       */
      if (drawable == (ClayDrawable *) floating_sel)
        {
          GList *drawables = clay_image_get_selected_drawables (image);

          g_return_if_fail (g_list_length (drawables) == 1);
          drawable = drawables->data;
          g_list_free (drawables);
        }
    }

  clay_layer_set_lock_alpha (layer, TRUE, FALSE);

  clay_layer_set_floating_sel_drawable (layer, drawable);

  /*  Floating selection layer placement, default to the top of the
   *  layers stack; parent and position are adapted according to the
   *  drawable associated with the floating selection.
   */

  if (CLAY_IS_LAYER_MASK (drawable))
    {
       ClayLayer *tmp = clay_layer_mask_get_layer (CLAY_LAYER_MASK (drawable));

       parent   = CLAY_LAYER (clay_item_get_parent (CLAY_ITEM (tmp)));
       position = clay_item_get_index (CLAY_ITEM (tmp));
    }
  else if (CLAY_IS_LAYER (drawable))
    {
      parent   = CLAY_LAYER (clay_item_get_parent (CLAY_ITEM (drawable)));
      position = clay_item_get_index (CLAY_ITEM (drawable));
    }

  clay_image_add_layer (image, layer, parent, position, TRUE);
}

void
floating_sel_anchor (ClayLayer *layer)
{
  ClayImage     *image;
  ClayDrawable  *drawable;
  ClayFilter    *filter = NULL;
  GeglRectangle  bounding_box;
  GeglRectangle  dr_bounding_box;
  gint           off_x, off_y;
  gint           dr_off_x, dr_off_y;

  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (clay_layer_is_floating_sel (layer));

  /* Don't let clay_image_remove_layer free the layer while we still need it */
  g_object_ref (layer);

  image = clay_item_get_image (CLAY_ITEM (layer));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_FS_ANCHOR,
                               C_("undo-type", "Anchor Floating Selection"));

  drawable = clay_layer_get_floating_sel_drawable (layer);

  clay_item_get_offset (CLAY_ITEM (layer), &off_x, &off_y);
  clay_item_get_offset (CLAY_ITEM (drawable), &dr_off_x, &dr_off_y);

  bounding_box    = clay_drawable_get_bounding_box (CLAY_DRAWABLE (layer));
  dr_bounding_box = clay_drawable_get_bounding_box (drawable);

  bounding_box.x    += off_x;
  bounding_box.y    += off_y;

  dr_bounding_box.x += dr_off_x;
  dr_bounding_box.y += dr_off_y;

  if (clay_item_get_visible (CLAY_ITEM (layer)) &&
      gegl_rectangle_intersect (NULL, &bounding_box, &dr_bounding_box))
    {
      filter = clay_drawable_get_floating_sel_filter (drawable);
    }

  if (filter)
    {
      clay_drawable_merge_filter (drawable, filter, NULL, NULL,
                                  NULL, FALSE, FALSE, FALSE);
    }

  clay_image_remove_layer (image, layer, TRUE, NULL);

  clay_image_undo_group_end (image);

  /*  invalidate the boundaries  */
  clay_drawable_invalidate_boundary (CLAY_DRAWABLE (clay_image_get_mask (image)));

  g_object_unref (layer);
}

gboolean
floating_sel_to_layer (ClayLayer  *layer,
                       GError    **error)
{
  ClayItem  *item;
  ClayImage *image;

  g_return_val_if_fail (CLAY_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (clay_layer_is_floating_sel (layer), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item  = CLAY_ITEM (layer);
  image = clay_item_get_image (item);

  /*  Check if the floating layer belongs to a channel  */
  if (CLAY_IS_CHANNEL (clay_layer_get_floating_sel_drawable (layer)))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot create a new layer from the floating "
                             "selection because it belongs to a layer mask "
                             "or channel."));
      return FALSE;
    }

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_FS_TO_LAYER,
                               C_("undo-type", "Floating Selection to Layer"));

  clay_image_undo_push_fs_to_layer (image, NULL, layer);

  clay_drawable_detach_floating_sel (clay_layer_get_floating_sel_drawable (layer));
  clay_layer_set_floating_sel_drawable (layer, NULL);

  clay_item_set_visible (item, TRUE, TRUE);
  clay_layer_set_lock_alpha (layer, FALSE, TRUE);

  clay_image_undo_group_end (image);

  /* When the floating selection is converted to/from a normal layer
   * it does something resembling a name change, so emit the
   * "name-changed" signal
   */
  clay_object_name_changed (CLAY_OBJECT (layer));

  clay_drawable_update (CLAY_DRAWABLE (layer),
                        0, 0,
                        clay_item_get_width  (item),
                        clay_item_get_height (item));

  return TRUE;
}

void
floating_sel_activate_drawable (ClayLayer *layer)
{
  ClayImage    *image;
  ClayDrawable *drawable;
  GList        *selected_drawables = NULL;

  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (clay_layer_is_floating_sel (layer));

  image = clay_item_get_image (CLAY_ITEM (layer));

  drawable = clay_layer_get_floating_sel_drawable (layer);

  /*  set the underlying drawable to active  */
  if (CLAY_IS_LAYER_MASK (drawable))
    {
      ClayLayerMask *mask = CLAY_LAYER_MASK (drawable);

      selected_drawables = g_list_prepend (NULL, clay_layer_mask_get_layer (mask));
      clay_image_set_selected_layers (image, selected_drawables);
    }
  else if (CLAY_IS_CHANNEL (drawable) && ! CLAY_IS_SELECTION (drawable))
    {
      selected_drawables = g_list_prepend (NULL, drawable);
      clay_image_set_selected_channels (image, selected_drawables);
    }
  else if (! CLAY_IS_SELECTION (drawable))
    {
      selected_drawables = g_list_prepend (NULL, drawable);
      clay_image_set_selected_layers (image, selected_drawables);
    }

  g_list_free (selected_drawables);
}

const ClayBoundSeg *
floating_sel_boundary (ClayLayer *layer,
                       gint      *n_segs)
{
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_layer_is_floating_sel (layer), NULL);
  g_return_val_if_fail (n_segs != NULL, NULL);

  if (layer->fs.boundary_known == FALSE)
    {
      gint width, height;
      gint off_x, off_y;

      width  = clay_item_get_width  (CLAY_ITEM (layer));
      height = clay_item_get_height (CLAY_ITEM (layer));
      clay_item_get_offset (CLAY_ITEM (layer), &off_x, &off_y);

      if (layer->fs.segs)
        g_free (layer->fs.segs);

      if (clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
        {
          GeglBuffer *buffer;
          gint        i;

          /*  find the segments  */
          buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

          layer->fs.segs = clay_boundary_find (buffer, NULL,
                                               babl_format ("A float"),
                                               CLAY_BOUNDARY_WITHIN_BOUNDS,
                                               0, 0, width, height,
                                               CLAY_BOUNDARY_HALF_WAY,
                                               &layer->fs.num_segs);

          /*  offset the segments  */
          for (i = 0; i < layer->fs.num_segs; i++)
            {
              layer->fs.segs[i].x1 += off_x;
              layer->fs.segs[i].y1 += off_y;
              layer->fs.segs[i].x2 += off_x;
              layer->fs.segs[i].y2 += off_y;
            }
        }
      else
        {
          layer->fs.num_segs = 4;
          layer->fs.segs     = g_new0 (ClayBoundSeg, 4);

          /* top */
          layer->fs.segs[0].x1 = off_x;
          layer->fs.segs[0].y1 = off_y;
          layer->fs.segs[0].x2 = off_x + width;
          layer->fs.segs[0].y2 = off_y;

          /* left */
          layer->fs.segs[1].x1 = off_x;
          layer->fs.segs[1].y1 = off_y;
          layer->fs.segs[1].x2 = off_x;
          layer->fs.segs[1].y2 = off_y + height;

          /* right */
          layer->fs.segs[2].x1 = off_x + width;
          layer->fs.segs[2].y1 = off_y;
          layer->fs.segs[2].x2 = off_x + width;
          layer->fs.segs[2].y2 = off_y + height;

          /* bottom */
          layer->fs.segs[3].x1 = off_x;
          layer->fs.segs[3].y1 = off_y + height;
          layer->fs.segs[3].x2 = off_x + width;
          layer->fs.segs[3].y2 = off_y + height;
        }

      layer->fs.boundary_known = TRUE;
    }

  *n_segs = layer->fs.num_segs;

  return layer->fs.segs;
}

void
floating_sel_invalidate (ClayLayer *layer)
{
  g_return_if_fail (CLAY_IS_LAYER (layer));
  g_return_if_fail (clay_layer_is_floating_sel (layer));

  /*  Invalidate the attached-to drawable's preview  */
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (clay_layer_get_floating_sel_drawable (layer)));

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}
