/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "claygrid.h"
#include "clayimage.h"
#include "clayimage-grid.h"
#include "clayimage-private.h"
#include "clayimage-undo-push.h"

#include "clay-intl.h"


ClayGrid *
clay_image_get_grid (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->grid;
}

void
clay_image_set_grid (ClayImage *image,
                     ClayGrid  *grid,
                     gboolean   push_undo)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_GRID (grid));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (clay_config_is_equal_to (CLAY_CONFIG (private->grid), CLAY_CONFIG (grid)))
    return;

  if (push_undo)
    clay_image_undo_push_image_grid (image,
                                     C_("undo-type", "Grid"), private->grid);

  clay_config_sync (G_OBJECT (grid), G_OBJECT (private->grid), 0);
}
