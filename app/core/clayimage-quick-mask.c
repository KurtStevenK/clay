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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "clay.h"
#include "claychannel.h"
#include "claydrawable-filters.h"
#include "clayimage.h"
#include "clayimage-private.h"
#include "clayimage-quick-mask.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer.h"
#include "claylayer-floating-selection.h"

#include "clay-intl.h"


/*  public functions  */

void
clay_image_set_quick_mask_state (ClayImage *image,
                                 gboolean   active)
{
  ClayImagePrivate *private;
  ClayChannel      *selection;
  ClayChannel      *mask;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  if (active == clay_image_get_quick_mask_state (image))
    return;

  private = CLAY_IMAGE_GET_PRIVATE (image);

  /*  Set private->quick_mask_state early so we can return early when
   *  being called recursively.
   */
  private->quick_mask_state = active;

  selection = clay_image_get_mask (image);
  mask      = clay_image_get_quick_mask (image);

  if (active)
    {
      private->quick_mask_selected = clay_image_get_selected_drawables (image);

      if (! mask)
        {
          ClayLayer *floating_sel;

          clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       C_("undo-type", "Enable Quick Mask"));

          floating_sel = clay_image_get_floating_selection (image);

          if (floating_sel)
            floating_sel_to_layer (floating_sel, NULL);

          mask = CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (selection),
                                                    CLAY_TYPE_CHANNEL));

          if (! clay_channel_is_empty (selection))
            clay_channel_clear (selection, NULL, TRUE);

          clay_channel_set_color (mask, private->quick_mask_color, FALSE);
          clay_item_rename (CLAY_ITEM (mask), CLAY_IMAGE_QUICK_MASK_NAME,
                            NULL);

          if (private->quick_mask_inverted)
            clay_channel_invert (mask, FALSE);

          clay_image_add_channel (image, mask, NULL, 0, TRUE);

          clay_image_undo_group_end (image);
        }
    }
  else
    {
      if (mask)
        {
          ClayLayer *floating_sel  = clay_image_get_floating_selection (image);
          GType      selected_type = CLAY_TYPE_LAYER;

          clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       C_("undo-type", "Disable Quick Mask"));

          if (private->quick_mask_inverted)
            clay_channel_invert (mask, TRUE);

          if (floating_sel &&
              clay_layer_get_floating_sel_drawable (floating_sel) == CLAY_DRAWABLE (mask))
            floating_sel_anchor (floating_sel);

          clay_drawable_merge_filters (CLAY_DRAWABLE (mask));

          clay_item_to_selection (CLAY_ITEM (mask),
                                  CLAY_CHANNEL_OP_REPLACE,
                                  TRUE, FALSE, 0.0, 0.0);
          clay_image_remove_channel (image, mask, TRUE, NULL);

          if (private->quick_mask_selected)
            selected_type = G_TYPE_FROM_INSTANCE (private->quick_mask_selected->data);

          clay_image_set_selected_items (image, selected_type,
                                         private->quick_mask_selected);

          clay_image_undo_group_end (image);
        }

      g_clear_pointer (&private->quick_mask_selected, g_list_free);
    }

  clay_image_quick_mask_changed (image);
}

gboolean
clay_image_get_quick_mask_state (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_state;
}

void
clay_image_set_quick_mask_color (ClayImage *image,
                                 GeglColor *color)
{
  ClayChannel *quick_mask;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (GEGL_IS_COLOR (color));

  g_clear_object (&(CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_color));
  CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_color = gegl_color_duplicate (color);

  quick_mask = clay_image_get_quick_mask (image);
  if (quick_mask)
    clay_channel_set_color (quick_mask, color, TRUE);
}

GeglColor *
clay_image_get_quick_mask_color (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_color;
}

ClayChannel *
clay_image_get_quick_mask (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_get_channel_by_name (image, CLAY_IMAGE_QUICK_MASK_NAME);
}

void
clay_image_quick_mask_invert (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->quick_mask_state)
    {
      ClayChannel *quick_mask = clay_image_get_quick_mask (image);

      if (quick_mask)
        clay_channel_invert (quick_mask, TRUE);
    }

  private->quick_mask_inverted = ! private->quick_mask_inverted;
}

gboolean
clay_image_get_quick_mask_inverted (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  return CLAY_IMAGE_GET_PRIVATE (image)->quick_mask_inverted;
}
