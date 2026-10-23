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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/claytilehandlervalidate.h"

#include "clay.h"
#include "clay-parallel.h"
#include "clay-utils.h"
#include "clayasync.h"
#include "claychannel.h"
#include "claychunkiterator.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "claydrawable-preview.h"
#include "claydrawable-private.h"
#include "claylayer.h"
#include "claytempbuf.h"

#include "clay-priorities.h"


typedef struct
{
  const Babl        *format;
  GeglBuffer        *buffer;
  GeglRectangle      rect;
  gdouble            scale;

  ClayChunkIterator *iter;
} SubPreviewData;


/*  local function prototypes  */

static SubPreviewData * sub_preview_data_new  (const Babl          *format,
                                               GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               gdouble              scale);
static void             sub_preview_data_free (SubPreviewData      *data);



/*  private functions  */


static SubPreviewData *
sub_preview_data_new (const Babl          *format,
                      GeglBuffer          *buffer,
                      const GeglRectangle *rect,
                      gdouble              scale)
{
  SubPreviewData *data = g_slice_new (SubPreviewData);

  data->format = format;
  /* We take ownership if the buffer reference. */
  data->buffer = buffer;
  data->rect   = *rect;
  data->scale  = scale;

  data->iter   = NULL;

  return data;
}

static void
sub_preview_data_free (SubPreviewData *data)
{
  g_object_unref (data->buffer);

  if (data->iter)
    clay_chunk_iterator_stop (data->iter, TRUE);

  g_slice_free (SubPreviewData, data);
}


/*  public functions  */


ClayTempBuf *
clay_drawable_get_new_preview (ClayViewable *viewable,
                               ClayContext  *context,
                               gint          width,
                               gint          height,
                               gint          scale_factor,
                               GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayItem  *item  = CLAY_ITEM (viewable);
  ClayImage *image = clay_item_get_image (item);

  if (! image->clay->config->layer_previews)
    return NULL;

  return clay_drawable_get_sub_preview (CLAY_DRAWABLE (viewable),
                                        0, 0,
                                        clay_item_get_width  (item),
                                        clay_item_get_height (item),
                                        width,
                                        height,
                                        scale_factor);
}

GdkPixbuf *
clay_drawable_get_new_pixbuf (ClayViewable *viewable,
                              ClayContext  *context,
                              gint          width,
                              gint          height,
                              gint          scale_factor,
                              GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayItem  *item  = CLAY_ITEM (viewable);
  ClayImage *image = clay_item_get_image (item);

  if (! image->clay->config->layer_previews)
    return NULL;

  return clay_drawable_get_sub_pixbuf (CLAY_DRAWABLE (viewable),
                                       0, 0,
                                       clay_item_get_width  (item),
                                       clay_item_get_height (item),
                                       width,
                                       height,
                                       scale_factor);
}

const Babl *
clay_drawable_get_preview_format (ClayDrawable *drawable)
{
  const Babl  *space;
  gboolean     alpha;
  ClayTRCType  trc;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  space = clay_drawable_get_space (drawable);
  alpha = clay_drawable_has_alpha (drawable);
  trc   = clay_drawable_get_trc (drawable);

  switch (clay_drawable_get_base_type (drawable))
    {
    case CLAY_RGB:
    case CLAY_GRAY:
      return clay_drawable_get_format (drawable);

    case CLAY_INDEXED:
      return clay_babl_format (CLAY_RGB,
                               clay_babl_precision (CLAY_COMPONENT_TYPE_U8,
                                                    trc),
                               alpha, space);
    }

  g_return_val_if_reached (NULL);
}

ClayTempBuf *
clay_drawable_get_sub_preview (ClayDrawable *drawable,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height,
                               gint          dest_scale_factor)
{
  ClayItem    *item;
  ClayImage   *image;
  GeglBuffer  *buffer;
  ClayTempBuf *preview;
  gdouble      scale;
  gint         scaled_x;
  gint         scaled_y;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);
  g_return_val_if_fail (dest_scale_factor > 0, NULL);

  item = CLAY_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= clay_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= clay_item_get_height (item), NULL);

  image = clay_item_get_image (item);

  if (! image->clay->config->layer_previews)
    return NULL;

  dest_width  *= dest_scale_factor;
  dest_height *= dest_scale_factor;

  buffer = clay_drawable_get_buffer_with_effects (drawable);

  preview = clay_temp_buf_new (dest_width, dest_height,
                               clay_drawable_get_preview_format (drawable));

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (scaled_x, scaled_y, dest_width, dest_height),
                   scale,
                   clay_temp_buf_get_format (preview),
                   clay_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  g_object_unref (buffer);

  return preview;
}

GdkPixbuf *
clay_drawable_get_sub_pixbuf (ClayDrawable *drawable,
                              gint          src_x,
                              gint          src_y,
                              gint          src_width,
                              gint          src_height,
                              gint          dest_width,
                              gint          dest_height,
                              gint          dest_scale_factor)
{
  ClayItem           *item;
  ClayImage          *image;
  GeglBuffer         *buffer;
  GdkPixbuf          *pixbuf;
  gdouble             scale;
  gint                scaled_x;
  gint                scaled_y;
  ClayColorTransform *transform;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);
  g_return_val_if_fail (dest_scale_factor > 0, NULL);

  item = CLAY_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= clay_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= clay_item_get_height (item), NULL);

  image = clay_item_get_image (item);

  if (! image->clay->config->layer_previews)
    return NULL;

  dest_width  *= dest_scale_factor;
  dest_height *= dest_scale_factor;

  buffer = clay_drawable_get_buffer_with_effects (drawable);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           dest_width, dest_height);

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  transform = clay_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      ClayTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = clay_temp_buf_new (dest_width, dest_height,
                                    clay_drawable_get_format (drawable));

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (scaled_x, scaled_y,
                                       dest_width, dest_height),
                       scale,
                       clay_temp_buf_get_format (temp_buf),
                       clay_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = clay_temp_buf_create_buffer (temp_buf);
      dest_buf = clay_pixbuf_create_buffer (pixbuf);

      clay_temp_buf_unref (temp_buf);

      clay_color_transform_process_buffer (transform,
                                           src_buf,
                                           GEGL_RECTANGLE (0, 0,
                                                           dest_width, dest_height),
                                           dest_buf,
                                           GEGL_RECTANGLE (0, 0, 0, 0));

      g_object_unref (src_buf);
      g_object_unref (dest_buf);
    }
  else
    {
      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (scaled_x, scaled_y,
                                       dest_width, dest_height),
                       scale,
                       clay_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  g_object_unref (buffer);

  return pixbuf;
}

static void
clay_drawable_get_sub_preview_async_func (ClayAsync      *async,
                                          SubPreviewData *data)
{
  ClayTempBuf             *preview;
  ClayTileHandlerValidate *validate;

  preview = clay_temp_buf_new (data->rect.width, data->rect.height,
                               data->format);

  validate = clay_tile_handler_validate_get_assigned (data->buffer);

  if (validate)
    {
      if (! data->iter)
        {
          cairo_region_t        *region;
          cairo_rectangle_int_t  rect;

          rect.x      = floor (data->rect.x / data->scale);
          rect.y      = floor (data->rect.y / data->scale);
          rect.width  = ceil ((data->rect.x + data->rect.width)  /
                              data->scale) - rect.x;
          rect.height = ceil ((data->rect.x + data->rect.height) /
                              data->scale) - rect.y;

          region = cairo_region_copy (validate->dirty_region);

          cairo_region_intersect_rectangle (region, &rect);

          data->iter = clay_chunk_iterator_new (region);
        }

      if (clay_chunk_iterator_next (data->iter))
        {
          GeglRectangle rect;

          clay_tile_handler_validate_begin_validate (validate);

          while (clay_chunk_iterator_get_rect (data->iter, &rect))
            {
              clay_tile_handler_validate_validate (validate,
                                                   data->buffer, &rect,
                                                   FALSE, FALSE);
            }

          clay_tile_handler_validate_end_validate (validate);

          return;
        }

      data->iter = NULL;
    }

  gegl_buffer_get (data->buffer, &data->rect, data->scale,
                   clay_temp_buf_get_format (preview),
                   clay_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  sub_preview_data_free (data);

  clay_async_finish_full (async,
                          preview,
                          (GDestroyNotify) clay_temp_buf_unref);
}

ClayAsync *
clay_drawable_get_sub_preview_async (ClayDrawable *drawable,
                                     gint          src_x,
                                     gint          src_y,
                                     gint          src_width,
                                     gint          src_height,
                                     gint          dest_width,
                                     gint          dest_height,
                                     gint          dest_scale_factor)
{
  ClayItem       *item;
  ClayImage      *image;
  GeglBuffer     *buffer;
  SubPreviewData *data;
  gdouble         scale;
  gint            scaled_x;
  gint            scaled_y;
  static gint     no_async_drawable_previews = -1;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);
  g_return_val_if_fail (dest_scale_factor > 0, NULL);

  item = CLAY_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= clay_item_get_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= clay_item_get_height (item), NULL);

  image = clay_item_get_image (item);

  if (! image->clay->config->layer_previews)
    return NULL;

  if (no_async_drawable_previews < 0)
    {
      no_async_drawable_previews =
        (g_getenv ("CLAY_NO_ASYNC_DRAWABLE_PREVIEWS") != NULL);
    }

  if (no_async_drawable_previews)
    {
      ClayAsync *async = clay_async_new ();

      clay_async_finish_full (async,
                              clay_drawable_get_sub_preview (drawable,
                                                             src_x,
                                                             src_y,
                                                             src_width,
                                                             src_height,
                                                             dest_width,
                                                             dest_height,
                                                             dest_scale_factor),
                              (GDestroyNotify) clay_temp_buf_unref);

      return async;
    }

  dest_width  *= dest_scale_factor;
  dest_height *= dest_scale_factor;

  scale = MIN ((gdouble) dest_width  / (gdouble) src_width,
               (gdouble) dest_height / (gdouble) src_height);

  scaled_x = RINT ((gdouble) src_x * scale);
  scaled_y = RINT ((gdouble) src_y * scale);

  buffer = clay_drawable_get_buffer_with_effects (drawable);

  data = sub_preview_data_new (clay_drawable_get_preview_format (drawable),
                               buffer,
                               GEGL_RECTANGLE (scaled_x, scaled_y,
                                               dest_width, dest_height),
                               scale);

  if (clay_tile_handler_validate_get_assigned (buffer))
    {
      return clay_idle_run_async_full (
        CLAY_PRIORITY_VIEWABLE_IDLE,
        (ClayRunAsyncFunc) clay_drawable_get_sub_preview_async_func,
        data,
        (GDestroyNotify) sub_preview_data_free);
    }
  else
    {
      return clay_parallel_run_async_full (
        +1,
        (ClayRunAsyncFunc) clay_drawable_get_sub_preview_async_func,
        data,
        (GDestroyNotify) sub_preview_data_free);
    }
}
