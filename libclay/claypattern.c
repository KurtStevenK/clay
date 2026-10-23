/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypattern.c
 * Copyright (C) 2023 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"

#include "claypattern.h"

struct _ClayPattern
{
  ClayResource  parent_instance;

  /* Native size buffer of the pattern contents. */
  GeglBuffer   *buffer;
};

static void         clay_pattern_finalize        (GObject     *object);
static void         clay_pattern_get_data        (ClayPattern *pattern);
static GeglBuffer * clay_pattern_scale           (GeglBuffer  *buffer,
                                                  gint         max_width,
                                                  gint         max_height);


G_DEFINE_TYPE (ClayPattern, clay_pattern, CLAY_TYPE_RESOURCE);


static void  clay_pattern_class_init (ClayPatternClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_pattern_finalize;
}

static void  clay_pattern_init (ClayPattern *pattern)
{
  pattern->buffer = NULL;
}

static void
clay_pattern_finalize (GObject *object)
{
  ClayPattern *pattern = CLAY_PATTERN (object);

  g_clear_object (&pattern->buffer);

  G_OBJECT_CLASS (clay_pattern_parent_class)->finalize (object);
}


/**
 * clay_pattern_get_buffer:
 * @pattern:    a [class@Pattern].
 * @max_width:  a maximum width for the returned buffer.
 * @max_height: a maximum height for the returned buffer.
 * @format:     an optional Babl format.
 *
 * Gets pixel data of the pattern within the bounding box specified by @max_width
 * and @max_height. The data will be scaled down so that it fits within this
 * size without changing its ratio. If the pattern is smaller than this size to
 * begin with, it will not be scaled up.
 *
 * If @max_width or @max_height are 0, the buffer is returned in the pattern's
 * native size.
 *
 * Make sure you called [func@Gegl.init] before calling any function using
 * `GEGL`.
 *
 * Returns: (transfer full): a [class@Gegl.Buffer].
*/
GeglBuffer *
clay_pattern_get_buffer (ClayPattern *pattern,
                         gint         max_width,
                         gint         max_height,
                         const Babl  *format)
{
  clay_pattern_get_data (pattern);

  g_return_val_if_fail (pattern->buffer != NULL, NULL);

  if (max_width == 0 || max_height == 0 ||
      (gegl_buffer_get_width (pattern->buffer) <= max_width &&
       gegl_buffer_get_height (pattern->buffer) <= max_height))
    return gegl_buffer_dup (pattern->buffer);

  return clay_pattern_scale (pattern->buffer, max_width, max_height);
}

static void
clay_pattern_get_data (ClayPattern *pattern)
{
  gint          width;
  gint          height;
  gint          bpp;
  GBytes       *bytes;
  const guchar *pixels;
  gsize         pixels_size;
  const Babl   *format;

  /*
   * This check assumes that the pattern contents doesn't change, which is not a
   * perfect assumption. We could maybe add a PDB call which would return
   * the new pattern data only if it changed since last call (which can be
   * verified with some kind of internal runtime version to pass from one call
   * to another). TODO
   */
  if (pattern->buffer != NULL)
    return;

  g_clear_object (&pattern->buffer);

  _clay_pattern_get_pixels (pattern, &width, &height, &bpp, &bytes);
  pixels = g_bytes_unref_to_data (bytes, &pixels_size);

  /* It's an ugly way to determine the proper format but clay_pattern_get_pixels()
   * doesn't give more info.
   */
  switch (bpp)
    {
    case 1:
      format = babl_format ("Y' u8");
      break;
    case 2:
      format = babl_format ("Y'A u8");
      break;
    case 3:
      format = babl_format ("R'G'B' u8");
      break;
    case 4:
      format = babl_format ("R'G'B'A u8");
      break;
    default:
      g_return_if_reached ();
    }

  pattern->buffer = gegl_buffer_linear_new_from_data ((const gpointer) pixels, format,
                                                      GEGL_RECTANGLE (0, 0, width, height),
                                                      0, g_free, NULL);
}

static GeglBuffer *
clay_pattern_scale (GeglBuffer *buffer,
                    gint        max_width,
                    gint        max_height)
{
  GeglBuffer *scaled = NULL;
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *op;
  GeglNode   *sink;
  gdouble     width;
  gdouble     height;
  gdouble     scale;

  height = (gdouble) max_height;
  width  = (gdouble) gegl_buffer_get_width (buffer) / gegl_buffer_get_height (buffer) * height;
  if (width > (gdouble) max_width)
    {
      width  = (gdouble) max_width;
      height = (gdouble) gegl_buffer_get_height (buffer) / gegl_buffer_get_width (buffer) * width;
    }
  scale = width / gegl_buffer_get_width (buffer);

  graph = gegl_node_new ();
  source = gegl_node_new_child (graph,
                                "operation", "gegl:buffer-source",
                                "buffer",    buffer,
                                NULL);
  op = gegl_node_new_child (graph,
                            "operation",   "gegl:scale-ratio",
                            "origin-x",     0.0,
                            "origin-y",     0.0,
                            "sampler",      CLAY_INTERPOLATION_LINEAR,
                            "abyss-policy", GEGL_ABYSS_CLAMP,
                            "x",            scale,
                            "y",            scale,
                            NULL);
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:buffer-sink",
                              "buffer",    &scaled,
                              "format",    gegl_buffer_get_format (buffer),
                              NULL);
  gegl_node_link_many (source, op, sink, NULL);
  gegl_node_process (sink);

  g_object_unref (graph);

  return scaled;
}
