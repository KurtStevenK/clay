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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clay-gegl-loops.h"

#include "claypattern.h"
#include "claypattern-load.h"
#include "claypattern-save.h"
#include "claysavable.h"
#include "claytagged.h"
#include "claytempbuf.h"

#include "clay-intl.h"


static void          clay_pattern_tagged_iface_init  (ClayTaggedInterface  *iface);
static void          clay_pattern_savable_iface_init (ClaySavableInterface *iface);
static void          clay_pattern_finalize           (GObject              *object);

static gint64        clay_pattern_get_memsize        (ClayObject           *object,
                                                      gint64               *gui_size);

static gboolean      clay_pattern_get_size           (ClayViewable         *viewable,
                                                      gint                 *width,
                                                      gint                 *height);
static ClayTempBuf * clay_pattern_get_new_preview    (ClayViewable         *viewable,
                                                      ClayContext          *context,
                                                      gint                  width,
                                                      gint                  height,
                                                      gint                  scale_factor,
                                                      GeglColor            *fg_color);
static gchar       * clay_pattern_get_description    (ClayViewable         *viewable,
                                                      gchar               **tooltip);

static const gchar * clay_pattern_get_extension      (ClayData             *data);
static void          clay_pattern_copy               (ClayData             *data,
                                                      ClayData             *src_data);

static gchar       * clay_pattern_get_checksum       (ClayTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (ClayPattern, clay_pattern, CLAY_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_TAGGED,
                                                clay_pattern_tagged_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_pattern_savable_iface_init))

#define parent_class clay_pattern_parent_class


static void
clay_pattern_class_init (ClayPatternClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayDataClass     *data_class        = CLAY_DATA_CLASS (klass);

  object_class->finalize            = clay_pattern_finalize;

  clay_object_class->get_memsize    = clay_pattern_get_memsize;

  viewable_class->default_icon_name = "clay-tool-bucket-fill";
  viewable_class->default_name      = _("Pattern");
  viewable_class->get_size          = clay_pattern_get_size;
  viewable_class->get_new_preview   = clay_pattern_get_new_preview;
  viewable_class->get_description   = clay_pattern_get_description;

  data_class->save                  = clay_pattern_save;
  data_class->get_extension         = clay_pattern_get_extension;
  data_class->copy                  = clay_pattern_copy;
}

static void
clay_pattern_tagged_iface_init (ClayTaggedInterface *iface)
{
  iface->get_checksum = clay_pattern_get_checksum;
}

static void
clay_pattern_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag = "pattern";
}

static void
clay_pattern_init (ClayPattern *pattern)
{
  pattern->mask = NULL;
}

static void
clay_pattern_finalize (GObject *object)
{
  ClayPattern *pattern = CLAY_PATTERN (object);

  g_clear_pointer (&pattern->mask, clay_temp_buf_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_pattern_get_memsize (ClayObject *object,
                          gint64     *gui_size)
{
  ClayPattern *pattern = CLAY_PATTERN (object);
  gint64       memsize = 0;

  memsize += clay_temp_buf_get_memsize (pattern->mask);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
clay_pattern_get_size (ClayViewable *viewable,
                       gint         *width,
                       gint         *height)
{
  ClayPattern *pattern = CLAY_PATTERN (viewable);

  *width  = clay_temp_buf_get_width  (pattern->mask);
  *height = clay_temp_buf_get_height (pattern->mask);

  return TRUE;
}

static ClayTempBuf *
clay_pattern_get_new_preview (ClayViewable *viewable,
                              ClayContext  *context,
                              gint          width,
                              gint          height,
                              gint          scale_factor,
                              GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayPattern *pattern = CLAY_PATTERN (viewable);
  ClayTempBuf *temp_buf;
  GeglBuffer  *src_buffer;
  gint         true_width;
  gint         true_height;
  gint         render_width;
  gint         render_height;
  gdouble      scale_x;
  gdouble      scale_y;
  gdouble      scale;

  if (pattern == NULL || pattern->mask == NULL)
    return NULL;

  if (width <= 0 || height <= 0)
    return NULL;

  if (scale_factor < 1)
    scale_factor = 1;

  render_width  = width * scale_factor;
  render_height = height * scale_factor;

  true_width  = clay_temp_buf_get_width (pattern->mask);
  true_height = clay_temp_buf_get_height (pattern->mask);

  if (true_width <= 0 || true_height <= 0)
    return NULL;

  /* Clamp scale to 1.0 to prevent upscaling and blurring small patterns */
  scale_x = (gdouble) render_width / (gdouble) true_width;
  scale_y = (gdouble) render_height / (gdouble) true_height;
  scale   = MIN (1.0, MIN (scale_x, scale_y));

  temp_buf = clay_temp_buf_new (render_width, render_height,
                                clay_temp_buf_get_format (pattern->mask));

  if (temp_buf == NULL)
    return NULL;

  src_buffer = clay_temp_buf_create_buffer (pattern->mask);

  if (src_buffer != NULL)
    {
      /* Use GEGL_ABYSS_LOOP to tile the pattern seamlessly if it
       * doesn't fill the requested render area.
       */
      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (0, 0, render_width, render_height),
                       scale, clay_temp_buf_get_format (temp_buf),
                       clay_temp_buf_get_data (temp_buf), GEGL_AUTO_ROWSTRIDE,
                       GEGL_ABYSS_LOOP);

      g_object_unref (src_buffer);
    }

  return temp_buf;
}

static gchar *
clay_pattern_get_description (ClayViewable  *viewable,
                              gchar        **tooltip)
{
  ClayPattern *pattern = CLAY_PATTERN (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          clay_object_get_name (pattern),
                          clay_temp_buf_get_width  (pattern->mask),
                          clay_temp_buf_get_height (pattern->mask));
}

static const gchar *
clay_pattern_get_extension (ClayData *data)
{
  return CLAY_PATTERN_FILE_EXTENSION;
}

static void
clay_pattern_copy (ClayData *data,
                   ClayData *src_data)
{
  ClayPattern *pattern     = CLAY_PATTERN (data);
  ClayPattern *src_pattern = CLAY_PATTERN (src_data);

  g_clear_pointer (&pattern->mask, clay_temp_buf_unref);
  pattern->mask = clay_temp_buf_copy (src_pattern->mask);

  clay_data_dirty (data);
}

static gchar *
clay_pattern_get_checksum (ClayTagged *tagged)
{
  ClayPattern *pattern         = CLAY_PATTERN (tagged);
  gchar       *checksum_string = NULL;

  if (pattern->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum, clay_temp_buf_get_data (pattern->mask),
                         clay_temp_buf_get_data_size (pattern->mask));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

ClayData *
clay_pattern_new (ClayContext *context,
                  const gchar *name)
{
  ClayPattern *pattern;
  guchar      *data;
  gint         row, col;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\n', NULL);

  pattern = g_object_new (CLAY_TYPE_PATTERN,
                          "name", name,
                          NULL);

  pattern->mask = clay_temp_buf_new (32, 32, babl_format ("R'G'B' u8"));

  data = clay_temp_buf_get_data (pattern->mask);

  for (row = 0; row < clay_temp_buf_get_height (pattern->mask); row++)
    for (col = 0; col < clay_temp_buf_get_width (pattern->mask); col++)
      {
        memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
        data += 3;
      }

  return CLAY_DATA (pattern);
}

ClayData *
clay_pattern_get_standard (ClayContext *context)
{
  static ClayData *standard_pattern = NULL;

  if (! standard_pattern)
    {
      g_set_weak_pointer (&standard_pattern,
                          clay_pattern_new (context, "Standard"));

      clay_data_clean (standard_pattern);
      clay_data_make_internal (standard_pattern, "clay-pattern-standard");
    }

  return standard_pattern;
}

ClayTempBuf *
clay_pattern_get_mask (ClayPattern *pattern)
{
  g_return_val_if_fail (CLAY_IS_PATTERN (pattern), NULL);

  return pattern->mask;
}

GeglBuffer *
clay_pattern_create_buffer (ClayPattern *pattern)
{
  g_return_val_if_fail (CLAY_IS_PATTERN (pattern), NULL);

  return clay_temp_buf_create_buffer (pattern->mask);
}
