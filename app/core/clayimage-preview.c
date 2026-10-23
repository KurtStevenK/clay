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

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-preview.h"
#include "claypickable.h"
#include "clayprojectable.h"
#include "clayprojection.h"
#include "claytempbuf.h"


const Babl *
clay_image_get_preview_format (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  switch (clay_image_get_base_type (image))
    {
    case CLAY_RGB:
    case CLAY_GRAY:
      return clay_projectable_get_format (CLAY_PROJECTABLE (image));

    case CLAY_INDEXED:
      return babl_format ("R'G'B'A u8");
    }

  g_return_val_if_reached (NULL);
}

void
clay_image_get_preview_size (ClayViewable *viewable,
                             gint          size,
                             gboolean      is_popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  ClayImage *image = CLAY_IMAGE (viewable);
  gdouble    xres;
  gdouble    yres;

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

gboolean
clay_image_get_popup_size (ClayViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  ClayImage *image = CLAY_IMAGE (viewable);

  if (clay_image_get_width  (image) > width ||
      clay_image_get_height (image) > height)
    {
      gboolean scaling_up;

      clay_viewable_calc_preview_size (clay_image_get_width  (image),
                                       clay_image_get_height (image),
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = clay_image_get_width  (image);
          *popup_height = clay_image_get_height (image);
        }

      return TRUE;
    }

  return FALSE;
}

ClayTempBuf *
clay_image_get_new_preview (ClayViewable *viewable,
                            ClayContext  *context,
                            gint          width,
                            gint          height,
                            gint          scale_factor,
                            GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayImage   *image = CLAY_IMAGE (viewable);
  const Babl  *format;
  ClayTempBuf *buf;
  gdouble      scale_x;
  gdouble      scale_y;

  width  *= scale_factor;
  height *= scale_factor;

  scale_x = (gdouble) width  / (gdouble) clay_image_get_width  (image);
  scale_y = (gdouble) height / (gdouble) clay_image_get_height (image);

  format = clay_image_get_preview_format (image);

  buf = clay_temp_buf_new (width, height, format);

  gegl_buffer_get (clay_pickable_get_buffer (CLAY_PICKABLE (image)),
                   GEGL_RECTANGLE (0, 0, width, height),
                   MIN (scale_x, scale_y),
                   clay_temp_buf_get_format (buf),
                   clay_temp_buf_get_data (buf),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return buf;
}

GdkPixbuf *
clay_image_get_new_pixbuf (ClayViewable *viewable,
                           ClayContext  *context,
                           gint          width,
                           gint          height,
                           gint          scale_factor,
                           GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayImage          *image = CLAY_IMAGE (viewable);
  GdkPixbuf          *pixbuf;
  gdouble             scale_x;
  gdouble             scale_y;
  ClayColorTransform *transform;

  width  *= scale_factor;
  height *= scale_factor;

  scale_x = (gdouble) width  / (gdouble) clay_image_get_width  (image);
  scale_y = (gdouble) height / (gdouble) clay_image_get_height (image);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  transform = clay_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      ClayTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = clay_temp_buf_new (width, height,
                                    clay_pickable_get_format (CLAY_PICKABLE (image)));

      gegl_buffer_get (clay_pickable_get_buffer (CLAY_PICKABLE (image)),
                       GEGL_RECTANGLE (0, 0, width, height),
                       MIN (scale_x, scale_y),
                       clay_temp_buf_get_format (temp_buf),
                       clay_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = clay_temp_buf_create_buffer (temp_buf);
      dest_buf = clay_pixbuf_create_buffer (pixbuf);

      clay_temp_buf_unref (temp_buf);

      clay_color_transform_process_buffer (transform,
                                           src_buf,
                                           GEGL_RECTANGLE (0, 0,
                                                           width, height),
                                           dest_buf,
                                           GEGL_RECTANGLE (0, 0, 0, 0));

      g_object_unref (src_buf);
      g_object_unref (dest_buf);
    }
  else
    {
      gegl_buffer_get (clay_pickable_get_buffer (CLAY_PICKABLE (image)),
                       GEGL_RECTANGLE (0, 0, width, height),
                       MIN (scale_x, scale_y),
                       clay_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}
