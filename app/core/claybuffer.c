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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "clay-memsize.h"
#include "claybuffer.h"
#include "clayimage.h"
#include "claytempbuf.h"

#include "clay-intl.h"


static void          clay_color_managed_iface_init (ClayColorManagedInterface *iface);

static void          clay_buffer_finalize          (GObject           *object);

static gint64        clay_buffer_get_memsize       (ClayObject        *object,
                                                    gint64            *gui_size);

static gboolean      clay_buffer_get_size          (ClayViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void          clay_buffer_get_preview_size  (ClayViewable      *viewable,
                                                    gint               size,
                                                    gboolean           is_popup,
                                                    gboolean           dot_for_dot,
                                                    gint              *popup_width,
                                                    gint              *popup_height);
static gboolean      clay_buffer_get_popup_size    (ClayViewable      *viewable,
                                                    gint               width,
                                                    gint               height,
                                                    gboolean           dot_for_dot,
                                                    gint              *popup_width,
                                                    gint              *popup_height);
static ClayTempBuf * clay_buffer_get_new_preview   (ClayViewable      *viewable,
                                                    ClayContext       *context,
                                                    gint               width,
                                                    gint               height,
                                                    gint               scale_factor,
                                                    GeglColor         *fg_color);
static GdkPixbuf   * clay_buffer_get_new_pixbuf    (ClayViewable      *viewable,
                                                    ClayContext       *context,
                                                    gint               width,
                                                    gint               height,
                                                    gint               scale_factor,
                                                    GeglColor         *fg_color);
static gchar       * clay_buffer_get_description   (ClayViewable      *viewable,
                                                    gchar            **tooltip);

static const guint8 *
       clay_buffer_color_managed_get_icc_profile   (ClayColorManaged  *managed,
                                                    gsize             *len);
static ClayColorProfile *
       clay_buffer_color_managed_get_color_profile (ClayColorManaged  *managed);
static void
       clay_buffer_color_managed_profile_changed   (ClayColorManaged  *managed);


G_DEFINE_TYPE_WITH_CODE (ClayBuffer, clay_buffer, CLAY_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_COLOR_MANAGED,
                                                clay_color_managed_iface_init))

#define parent_class clay_buffer_parent_class


static void
clay_buffer_class_init (ClayBufferClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);

  object_class->finalize            = clay_buffer_finalize;

  clay_object_class->get_memsize    = clay_buffer_get_memsize;

  viewable_class->default_icon_name = "edit-paste";
  viewable_class->default_name      = _("Buffer");
  viewable_class->name_editable     = TRUE;
  viewable_class->get_size          = clay_buffer_get_size;
  viewable_class->get_preview_size  = clay_buffer_get_preview_size;
  viewable_class->get_popup_size    = clay_buffer_get_popup_size;
  viewable_class->get_new_preview   = clay_buffer_get_new_preview;
  viewable_class->get_new_pixbuf    = clay_buffer_get_new_pixbuf;
  viewable_class->get_description   = clay_buffer_get_description;
}

static void
clay_color_managed_iface_init (ClayColorManagedInterface *iface)
{
  iface->get_icc_profile   = clay_buffer_color_managed_get_icc_profile;
  iface->get_color_profile = clay_buffer_color_managed_get_color_profile;
  iface->profile_changed   = clay_buffer_color_managed_profile_changed;
}

static void
clay_buffer_init (ClayBuffer *buffer)
{
}

static void
clay_buffer_finalize (GObject *object)
{
  ClayBuffer *buffer = CLAY_BUFFER (object);

  g_clear_object (&buffer->buffer);

  clay_buffer_set_color_profile (buffer, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_buffer_get_memsize (ClayObject *object,
                         gint64     *gui_size)
{
  ClayBuffer *buffer  = CLAY_BUFFER (object);
  gint64      memsize = 0;

  memsize += clay_gegl_buffer_get_memsize (buffer->buffer);
  memsize += clay_g_object_get_memsize (G_OBJECT (buffer->color_profile));
  memsize += clay_g_object_get_memsize (G_OBJECT (buffer->format_profile));

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
clay_buffer_get_size (ClayViewable *viewable,
                      gint         *width,
                      gint         *height)
{
  ClayBuffer *buffer = CLAY_BUFFER (viewable);

  *width  = clay_buffer_get_width (buffer);
  *height = clay_buffer_get_height (buffer);

  return TRUE;
}

static void
clay_buffer_get_preview_size (ClayViewable *viewable,
                              gint          size,
                              gboolean      is_popup,
                              gboolean      dot_for_dot,
                              gint         *width,
                              gint         *height)
{
  ClayBuffer *buffer = CLAY_BUFFER (viewable);

  clay_viewable_calc_preview_size (clay_buffer_get_width (buffer),
                                   clay_buffer_get_height (buffer),
                                   size,
                                   size,
                                   dot_for_dot, 1.0, 1.0,
                                   width,
                                   height,
                                   NULL);
}

static gboolean
clay_buffer_get_popup_size (ClayViewable *viewable,
                            gint          width,
                            gint          height,
                            gboolean      dot_for_dot,
                            gint         *popup_width,
                            gint         *popup_height)
{
  ClayBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;

  buffer        = CLAY_BUFFER (viewable);
  buffer_width  = clay_buffer_get_width (buffer);
  buffer_height = clay_buffer_get_height (buffer);

  if (buffer_width > width || buffer_height > height)
    {
      gboolean scaling_up;

      clay_viewable_calc_preview_size (buffer_width,
                                       buffer_height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = buffer_width;
          *popup_height = buffer_height;
        }

      return TRUE;
    }

  return FALSE;
}

static ClayTempBuf *
clay_buffer_get_new_preview (ClayViewable *viewable,
                             ClayContext  *context,
                             gint          width,
                             gint          height,
                             gint          scale_factor,
                             GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayBuffer  *buffer = CLAY_BUFFER (viewable);
  const Babl  *format = clay_buffer_get_format (buffer);
  ClayTempBuf *preview;

  width  *= scale_factor;
  height *= scale_factor;

  if (babl_format_is_palette (format))
    format = clay_babl_format (CLAY_RGB,
                               CLAY_PRECISION_U8_NON_LINEAR,
                               babl_format_has_alpha (format),
                               babl_format_get_space (format));
  else
    format = clay_babl_format (clay_babl_format_get_base_type (format),
                               clay_babl_precision (CLAY_COMPONENT_TYPE_U8,
                                                    clay_babl_format_get_trc (format)),
                               babl_format_has_alpha (format),
                               babl_format_get_space (format));

  preview = clay_temp_buf_new (width, height, format);

  gegl_buffer_get (buffer->buffer, GEGL_RECTANGLE (0, 0, width, height),
                   MIN ((gdouble) width  / (gdouble) clay_buffer_get_width (buffer),
                        (gdouble) height / (gdouble) clay_buffer_get_height (buffer)),
                   format,
                   clay_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  return preview;
}

static GdkPixbuf *
clay_buffer_get_new_pixbuf (ClayViewable *viewable,
                            ClayContext  *context,
                            gint          width,
                            gint          height,
                            gint          scale_factor,
                            GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayBuffer *buffer = CLAY_BUFFER (viewable);
  GdkPixbuf  *pixbuf;
  gdouble     scale;

  width  *= scale_factor;
  height *= scale_factor;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  scale = MIN ((gdouble) width  / (gdouble) clay_buffer_get_width (buffer),
               (gdouble) height / (gdouble) clay_buffer_get_height (buffer));

  if (buffer->color_profile)
    {
      ClayColorProfile *srgb_profile;
      ClayTempBuf      *temp_buf;
      GeglBuffer       *src_buf;
      GeglBuffer       *dest_buf;

      srgb_profile = clay_color_profile_new_rgb_srgb ();

      temp_buf = clay_temp_buf_new (width, height,
                                    clay_buffer_get_format (buffer));

      gegl_buffer_get (buffer->buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       scale,
                       clay_temp_buf_get_format (temp_buf),
                       clay_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = clay_temp_buf_create_buffer (temp_buf);
      dest_buf = clay_pixbuf_create_buffer (pixbuf);

      clay_temp_buf_unref (temp_buf);

      clay_gegl_convert_color_profile (src_buf,
                                       GEGL_RECTANGLE (0, 0, width, height),
                                       buffer->color_profile,
                                       dest_buf,
                                       GEGL_RECTANGLE (0, 0, 0, 0),
                                       srgb_profile,
                                       CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                       TRUE,
                                       NULL);

      g_object_unref (src_buf);
      g_object_unref (dest_buf);

      g_object_unref (srgb_profile);
    }
  else
    {
      gegl_buffer_get (buffer->buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       scale,
                       clay_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static gchar *
clay_buffer_get_description (ClayViewable  *viewable,
                             gchar        **tooltip)
{
  ClayBuffer *buffer = CLAY_BUFFER (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          clay_object_get_name (buffer),
                          clay_buffer_get_width (buffer),
                          clay_buffer_get_height (buffer));
}

static const guint8 *
clay_buffer_color_managed_get_icc_profile (ClayColorManaged *managed,
                                           gsize            *len)
{
  ClayBuffer *buffer = CLAY_BUFFER (managed);

  if (buffer->color_profile)
    return clay_color_profile_get_icc_profile (buffer->color_profile, len);

  /* creates buffer->format_profile */
  clay_color_managed_get_color_profile (managed);

  return clay_color_profile_get_icc_profile (buffer->format_profile, len);
}

static ClayColorProfile *
clay_buffer_color_managed_get_color_profile (ClayColorManaged *managed)
{
  ClayBuffer *buffer = CLAY_BUFFER (managed);

  if (buffer->color_profile)
    return buffer->color_profile;

  if (! buffer->format_profile)
    buffer->format_profile =
      clay_babl_format_get_color_profile (clay_buffer_get_format (buffer));

  return buffer->format_profile;
}

static void
clay_buffer_color_managed_profile_changed (ClayColorManaged *managed)
{
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (managed));
}


/*  public functions  */

ClayBuffer *
clay_buffer_new (GeglBuffer    *buffer,
                 const gchar   *name,
                 gint           offset_x,
                 gint           offset_y,
                 gboolean       copy_pixels)
{
  ClayBuffer *clay_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  clay_buffer = g_object_new (CLAY_TYPE_BUFFER,
                              "name", name,
                              NULL);

  if (copy_pixels)
    clay_buffer->buffer = clay_gegl_buffer_dup (buffer, NULL);
  else
    clay_buffer->buffer = g_object_ref (buffer);

  clay_buffer->offset_x = offset_x;
  clay_buffer->offset_y = offset_y;

  return clay_buffer;
}

ClayBuffer *
clay_buffer_new_from_pixbuf (GdkPixbuf   *pixbuf,
                             const gchar *name,
                             gint         offset_x,
                             gint         offset_y)
{
  ClayBuffer       *clay_buffer;
  GeglBuffer       *buffer;
  guint8           *icc_data;
  gsize             icc_len;
  ClayColorProfile *profile = NULL;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  buffer = clay_pixbuf_create_buffer (pixbuf);

  clay_buffer = clay_buffer_new (buffer, name,
                                 offset_x, offset_y, FALSE);

  icc_data = clay_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      profile = clay_color_profile_new_from_icc_profile (icc_data, icc_len,
                                                         NULL);
      g_free (icc_data);
    }

  if (! profile && gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB)
    {
      profile = clay_color_profile_new_rgb_srgb ();
    }

  if (profile)
    {
      clay_buffer_set_color_profile (clay_buffer, profile);
      g_object_unref (profile);
    }

  g_object_unref (buffer);

  return clay_buffer;
}

gint
clay_buffer_get_width (ClayBuffer *buffer)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_width (buffer->buffer);
}

gint
clay_buffer_get_height (ClayBuffer *buffer)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_height (buffer->buffer);
}

const Babl *
clay_buffer_get_format (ClayBuffer *buffer)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), NULL);

  return gegl_buffer_get_format (buffer->buffer);
}

GeglBuffer *
clay_buffer_get_buffer (ClayBuffer *buffer)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), NULL);

  return buffer->buffer;
}

void
clay_buffer_set_resolution (ClayBuffer *buffer,
                            gdouble     resolution_x,
                            gdouble     resolution_y)
{
  g_return_if_fail (CLAY_IS_BUFFER (buffer));
  g_return_if_fail (resolution_x >= 0.0 && resolution_x <= CLAY_MAX_RESOLUTION);
  g_return_if_fail (resolution_y >= 0.0 && resolution_y <= CLAY_MAX_RESOLUTION);

  buffer->resolution_x = resolution_x;
  buffer->resolution_y = resolution_y;
}

gboolean
clay_buffer_get_resolution (ClayBuffer *buffer,
                            gdouble    *resolution_x,
                            gdouble    *resolution_y)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), FALSE);

  if (buffer->resolution_x > 0.0 &&
      buffer->resolution_y > 0.0)
    {
      if (resolution_x) *resolution_x = buffer->resolution_x;
      if (resolution_y) *resolution_y = buffer->resolution_y;

      return TRUE;
    }

  return FALSE;
}

void
clay_buffer_set_unit (ClayBuffer *buffer,
                      ClayUnit   *unit)
{
  g_return_if_fail (CLAY_IS_BUFFER (buffer));
  g_return_if_fail (CLAY_IS_UNIT (unit));

  buffer->unit = unit;
}

ClayUnit *
clay_buffer_get_unit (ClayBuffer *buffer)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), clay_unit_pixel ());

  return buffer->unit;
}

void
clay_buffer_set_color_profile (ClayBuffer       *buffer,
                               ClayColorProfile *profile)
{
  g_return_if_fail (CLAY_IS_BUFFER (buffer));
  g_return_if_fail (profile == NULL || CLAY_IS_COLOR_PROFILE (profile));

  if (profile != buffer->color_profile)
    {
      g_set_object (&buffer->color_profile, profile);
    }

  g_clear_object (&buffer->format_profile);
}

ClayColorProfile *
clay_buffer_get_color_profile (ClayBuffer *buffer)
{
  g_return_val_if_fail (CLAY_IS_BUFFER (buffer), NULL);

  return buffer->color_profile;
}
