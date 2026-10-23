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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clayapplicator.h"

#include "clay.h"
#include "claychannel.h"
#include "claychunkiterator.h"
#include "claydrawable-combine.h"
#include "clayimage.h"


void
clay_drawable_real_apply_buffer (ClayDrawable           *drawable,
                                 GeglBuffer             *buffer,
                                 const GeglRectangle    *buffer_region,
                                 gboolean                push_undo,
                                 const gchar            *undo_desc,
                                 gdouble                 opacity,
                                 ClayLayerMode           mode,
                                 ClayLayerColorSpace     blend_space,
                                 ClayLayerColorSpace     composite_space,
                                 ClayLayerCompositeMode  composite_mode,
                                 GeglBuffer             *base_buffer,
                                 gint                    base_x,
                                 gint                    base_y)
{
  ClayItem          *item  = CLAY_ITEM (drawable);
  ClayImage         *image = clay_item_get_image (item);
  ClayChannel       *mask  = clay_image_get_mask (image);
  ClayApplicator    *applicator;
  ClayChunkIterator *iter;
  gint               x, y, width, height;
  gint               offset_x, offset_y;

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (CLAY_DRAWABLE (mask) == drawable || clay_channel_is_empty (mask))
    mask = NULL;

  if (! base_buffer)
    base_buffer = clay_drawable_get_buffer (drawable);

  /*  get the layer offsets  */
  clay_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  if (! clay_rectangle_intersect (base_x, base_y,
                                  buffer_region->width, buffer_region->height,
                                  0, 0,
                                  clay_item_get_width  (item),
                                  clay_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      return;
    }

  if (mask)
    {
      ClayItem *mask_item = CLAY_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      if (! clay_rectangle_intersect (x, y, width, height,
                                      -offset_x, -offset_y,
                                      clay_item_get_width  (mask_item),
                                      clay_item_get_height (mask_item),
                                      &x, &y, &width, &height))
        {
          return;
        }
    }

  if (push_undo)
    {
      clay_drawable_push_undo (drawable, undo_desc,
                               NULL, x, y, width, height);
    }

  applicator = clay_applicator_new (NULL);

  if (mask)
    {
      GeglBuffer *mask_buffer;

      mask_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));

      clay_applicator_set_mask_buffer (applicator, mask_buffer);
      clay_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
    }

  clay_applicator_set_src_buffer (applicator, base_buffer);
  clay_applicator_set_dest_buffer (applicator,
                                   clay_drawable_get_buffer (drawable));

  clay_applicator_set_apply_buffer (applicator, buffer);
  clay_applicator_set_apply_offset (applicator,
                                    base_x - buffer_region->x,
                                    base_y - buffer_region->y);

  clay_applicator_set_opacity (applicator, opacity);
  clay_applicator_set_mode (applicator, mode,
                            blend_space, composite_space, composite_mode);
  clay_applicator_set_affect (applicator,
                              clay_drawable_get_active_mask (drawable));

  iter = clay_chunk_iterator_new (cairo_region_create_rectangle (
    &(cairo_rectangle_int_t) {x, y, width, height}));

  while (clay_chunk_iterator_next (iter))
    {
      GeglRectangle rect;

      while (clay_chunk_iterator_get_rect (iter, &rect))
        clay_applicator_blit (applicator, &rect);
    }

  g_object_unref (applicator);
}
