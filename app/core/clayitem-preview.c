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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaymath/claymath.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "clay.h"
#include "clayimage.h"
#include "clayitem.h"
#include "clayitem-preview.h"


/*  public functions  */

void
clay_item_get_preview_size (ClayViewable *viewable,
                            gint          size,
                            gboolean      is_popup,
                            gboolean      dot_for_dot,
                            gint         *width,
                            gint         *height)
{
  ClayItem  *item  = CLAY_ITEM (viewable);
  ClayImage *image = clay_item_get_image (item);

  if (image && ! image->clay->config->layer_previews && ! is_popup)
    {
      *width  = size;
      *height = size;
      return;
    }

  if (image && ! is_popup)
    {
      gdouble xres;
      gdouble yres;

      clay_image_get_resolution (image, &xres, &yres);

      clay_viewable_calc_preview_size (clay_image_get_width  (image),
                                       clay_image_get_height (image),
                                       size,
                                       size,
                                       dot_for_dot,
                                       xres,
                                       yres,
                                       width,
                                       height,
                                       NULL);
    }
  else
    {
      clay_viewable_calc_preview_size (clay_item_get_width  (item),
                                       clay_item_get_height (item),
                                       size,
                                       size,
                                       dot_for_dot, 1.0, 1.0,
                                       width,
                                       height,
                                       NULL);
    }
}

gboolean
clay_item_get_popup_size (ClayViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  ClayItem  *item  = CLAY_ITEM (viewable);
  ClayImage *image = clay_item_get_image (item);

  if (image && ! image->clay->config->layer_previews)
    return FALSE;

  if (clay_item_get_width  (item) > width ||
      clay_item_get_height (item) > height)
    {
      gboolean scaling_up;
      gdouble  xres = 1.0;
      gdouble  yres = 1.0;

      if (image)
        clay_image_get_resolution (image, &xres, &yres);

      clay_viewable_calc_preview_size (clay_item_get_width  (item),
                                       clay_item_get_height (item),
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot,
                                       xres,
                                       yres,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = clay_item_get_width  (item);
          *popup_height = clay_item_get_height (item);
        }

      return TRUE;
    }

  return FALSE;
}
