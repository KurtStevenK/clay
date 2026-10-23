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

#include "claydrawable.h"
#include "claydrawable-equalize.h"
#include "claydrawable-histogram.h"
#include "claydrawable-operation.h"
#include "clayhistogram.h"
#include "clayimage.h"
#include "clayselection.h"

#include "clay-intl.h"


void
clay_drawable_equalize (ClayDrawable *drawable,
                        gboolean      mask_only)
{
  ClayImage     *image;
  ClayChannel   *selection;
  ClayHistogram *histogram;
  GeglNode      *equalize;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));

  image = clay_item_get_image (CLAY_ITEM (drawable));
  selection = clay_image_get_mask (image);

  histogram = clay_histogram_new (CLAY_TRC_NON_LINEAR);
  clay_drawable_calculate_histogram (drawable, histogram, FALSE);

  equalize = gegl_node_new_child (NULL,
                                  "operation", "clay:equalize",
                                  "histogram", histogram,
                                  NULL);

  if (! mask_only)
    clay_selection_suspend (CLAY_SELECTION (selection));

  clay_drawable_apply_operation (drawable, NULL,
                                 C_("undo-type", "Equalize"),
                                 equalize);

  if (! mask_only)
    clay_selection_resume (CLAY_SELECTION (selection));

  g_object_unref (equalize);
  g_object_unref (histogram);
}
