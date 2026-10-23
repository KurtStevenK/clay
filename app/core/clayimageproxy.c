/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimageproxy.c
 * Copyright (C) 2019 Ell
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
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-preview.h"
#include "clayimageproxy.h"
#include "claypickable.h"
#include "clayprojectable.h"
#include "claytempbuf.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_SHOW_ALL,
  PROP_BUFFER
};


struct _ClayImageProxyPrivate
{
  ClayImage     *image;
  gboolean       show_all;

  GeglRectangle  bounding_box;
  gboolean       frozen;
};


/*  local function prototypes  */

static void               clay_image_proxy_pickable_iface_init      (ClayPickableInterface      *iface);
static void               clay_image_proxy_color_managed_iface_init (ClayColorManagedInterface  *iface);

static void               clay_image_proxy_finalize                 (GObject                    *object);
static void               clay_image_proxy_set_property             (GObject                    *object,
                                                                     guint                       property_id,
                                                                     const GValue               *value,
                                                                     GParamSpec                 *pspec);
static void               clay_image_proxy_get_property             (GObject                    *object,
                                                                     guint                       property_id,
                                                                     GValue                     *value,
                                                                     GParamSpec                 *pspec);

static gboolean           clay_image_proxy_get_size                 (ClayViewable               *viewable,
                                                                     gint                       *width,
                                                                     gint                       *height);
static void               clay_image_proxy_get_preview_size         (ClayViewable               *viewable,
                                                                     gint                        size,
                                                                     gboolean                    is_popup,
                                                                     gboolean                    dot_for_dot,
                                                                     gint                       *width,
                                                                     gint                       *height);
static gboolean           clay_image_proxy_get_popup_size           (ClayViewable               *viewable,
                                                                     gint                        width,
                                                                     gint                        height,
                                                                     gboolean                    dot_for_dot,
                                                                     gint                       *popup_width,
                                                                     gint                       *popup_height);
static ClayTempBuf      * clay_image_proxy_get_new_preview          (ClayViewable               *viewable,
                                                                     ClayContext                *context,
                                                                     gint                        width,
                                                                     gint                        height,
                                                                     gint                        scale_factor,
                                                                     GeglColor                  *fg_color);
static GdkPixbuf        * clay_image_proxy_get_new_pixbuf           (ClayViewable               *viewable,
                                                                     ClayContext                *context,
                                                                     gint                        width,
                                                                     gint                        height,
                                                                     gint                        scale_factor,
                                                                     GeglColor                  *fg_color);
static gchar            * clay_image_proxy_get_description          (ClayViewable               *viewable,
                                                                     gchar                     **tooltip);

static void               clay_image_proxy_flush                    (ClayPickable               *pickable);
static const Babl       * clay_image_proxy_get_format               (ClayPickable               *pickable);
static const Babl       * clay_image_proxy_get_format_with_alpha    (ClayPickable               *pickable);
static GeglBuffer       * clay_image_proxy_get_buffer               (ClayPickable               *pickable);
static gboolean           clay_image_proxy_get_pixel_at             (ClayPickable               *pickable,
                                                                     gint                        x,
                                                                     gint                        y,
                                                                     const Babl                 *format,
                                                                     gpointer                    pixel);
static gdouble            clay_image_proxy_get_opacity_at           (ClayPickable               *pickable,
                                                                     gint                        x,
                                                                     gint                        y);
static void               clay_image_proxy_get_pixel_average        (ClayPickable               *pickable,
                                                                     const GeglRectangle        *rect,
                                                                     const Babl                 *format,
                                                                     gpointer                    pixel);

static const guint8     * clay_image_proxy_get_icc_profile          (ClayColorManaged           *managed,
                                                                     gsize                      *len);
static ClayColorProfile * clay_image_proxy_get_color_profile        (ClayColorManaged           *managed);
static void               clay_image_proxy_profile_changed          (ClayColorManaged           *managed);

static void               clay_image_proxy_image_frozen_notify      (ClayImage                  *image,
                                                                     const GParamSpec           *pspec,
                                                                     ClayImageProxy             *image_proxy);
static void               clay_image_proxy_image_invalidate_preview (ClayImage                  *image,
                                                                     ClayImageProxy             *image_proxy);
static void               clay_image_proxy_image_size_changed       (ClayImage                  *image,
                                                                     ClayImageProxy             *image_proxy);
static void               clay_image_proxy_image_bounds_changed     (ClayImage                  *image,
                                                                     gint                        old_x,
                                                                     gint                        old_y,
                                                                     ClayImageProxy             *image_proxy);
static void               clay_image_proxy_image_profile_changed    (ClayImage                  *image,
                                                                     ClayImageProxy             *image_proxy);

static void               clay_image_proxy_set_image                (ClayImageProxy             *image_proxy,
                                                                     ClayImage                  *image);
static ClayPickable     * clay_image_proxy_get_pickable             (ClayImageProxy             *image_proxy);
static void               clay_image_proxy_update_bounding_box      (ClayImageProxy             *image_proxy);
static void               clay_image_proxy_update_frozen            (ClayImageProxy             *image_proxy);


G_DEFINE_TYPE_WITH_CODE (ClayImageProxy, clay_image_proxy, CLAY_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (ClayImageProxy)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                clay_image_proxy_pickable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_COLOR_MANAGED,
                                                clay_image_proxy_color_managed_iface_init))

#define parent_class clay_image_proxy_parent_class


/*  private functions  */


static void
clay_image_proxy_class_init (ClayImageProxyClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->finalize            = clay_image_proxy_finalize;
  object_class->set_property        = clay_image_proxy_set_property;
  object_class->get_property        = clay_image_proxy_get_property;

  viewable_class->default_icon_name = "clay-image";
  viewable_class->get_size          = clay_image_proxy_get_size;
  viewable_class->get_preview_size  = clay_image_proxy_get_preview_size;
  viewable_class->get_popup_size    = clay_image_proxy_get_popup_size;
  viewable_class->get_new_preview   = clay_image_proxy_get_new_preview;
  viewable_class->get_new_pixbuf    = clay_image_proxy_get_new_pixbuf;
  viewable_class->get_description   = clay_image_proxy_get_description;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SHOW_ALL,
                                   g_param_spec_boolean ("show-all",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
clay_image_proxy_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->flush                 = clay_image_proxy_flush;
  iface->get_image             = (gpointer) clay_image_proxy_get_image;
  iface->get_format            = clay_image_proxy_get_format;
  iface->get_format_with_alpha = clay_image_proxy_get_format_with_alpha;
  iface->get_buffer            = clay_image_proxy_get_buffer;
  iface->get_pixel_at          = clay_image_proxy_get_pixel_at;
  iface->get_opacity_at        = clay_image_proxy_get_opacity_at;
  iface->get_pixel_average     = clay_image_proxy_get_pixel_average;
}

static void
clay_image_proxy_color_managed_iface_init (ClayColorManagedInterface *iface)
{
  iface->get_icc_profile   = clay_image_proxy_get_icc_profile;
  iface->get_color_profile = clay_image_proxy_get_color_profile;
  iface->profile_changed   = clay_image_proxy_profile_changed;
}

static void
clay_image_proxy_init (ClayImageProxy *image_proxy)
{
  image_proxy->priv = clay_image_proxy_get_instance_private (image_proxy);
}

static void
clay_image_proxy_finalize (GObject *object)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (object);

  clay_image_proxy_set_image (image_proxy, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_image_proxy_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      clay_image_proxy_set_image (image_proxy,
                                  g_value_get_object (value));
      break;

    case PROP_SHOW_ALL:
      clay_image_proxy_set_show_all (image_proxy,
                                     g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_image_proxy_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value,
                          clay_image_proxy_get_image (image_proxy));
      break;

    case PROP_SHOW_ALL:
      g_value_set_boolean (value,
                           clay_image_proxy_get_show_all (image_proxy));
      break;

    case PROP_BUFFER:
      g_value_set_object (value,
                          clay_pickable_get_buffer (
                            CLAY_PICKABLE (image_proxy)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_image_proxy_get_size (ClayViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (viewable);

  *width  = image_proxy->priv->bounding_box.width;
  *height = image_proxy->priv->bounding_box.height;

  return TRUE;
}

static void
clay_image_proxy_get_preview_size (ClayViewable *viewable,
                                   gint          size,
                                   gboolean      is_popup,
                                   gboolean      dot_for_dot,
                                   gint         *width,
                                   gint         *height)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (viewable);
  ClayImage      *image       = image_proxy->priv->image;
  gdouble         xres;
  gdouble         yres;
  gint            viewable_width;
  gint            viewable_height;

  clay_image_get_resolution (image, &xres, &yres);

  clay_viewable_get_size (viewable, &viewable_width, &viewable_height);

  clay_viewable_calc_preview_size (viewable_width,
                                   viewable_height,
                                   size,
                                   size,
                                   dot_for_dot,
                                   xres,
                                   yres,
                                   width,
                                   height,
                                   NULL);
}

static gboolean
clay_image_proxy_get_popup_size (ClayViewable *viewable,
                                 gint          width,
                                 gint          height,
                                 gboolean      dot_for_dot,
                                 gint         *popup_width,
                                 gint         *popup_height)
{
  gint viewable_width;
  gint viewable_height;

  clay_viewable_get_size (viewable, &viewable_width, &viewable_height);

  if (viewable_width > width || viewable_height > height)
    {
      gboolean scaling_up;

      clay_viewable_calc_preview_size (viewable_width,
                                       viewable_height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = viewable_width;
          *popup_height = viewable_height;
        }

      return TRUE;
    }

  return FALSE;
}

static ClayTempBuf *
clay_image_proxy_get_new_preview (ClayViewable *viewable,
                                  ClayContext  *context,
                                  gint          width,
                                  gint          height,
                                  gint          scale_factor,
                                  GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (viewable);
  ClayImage      *image       = image_proxy->priv->image;
  ClayPickable   *pickable;
  const Babl     *format;
  GeglRectangle   bounding_box;
  ClayTempBuf    *buf;
  gdouble         scale_x;
  gdouble         scale_y;
  gdouble         scale;

  pickable     = clay_image_proxy_get_pickable     (image_proxy);
  bounding_box = clay_image_proxy_get_bounding_box (image_proxy);

  width  *= scale_factor;
  height *= scale_factor;

  scale_x = (gdouble) width  / (gdouble) bounding_box.width;
  scale_y = (gdouble) height / (gdouble) bounding_box.height;

  scale   = MIN (scale_x, scale_y);

  format = clay_image_get_preview_format (image);

  buf = clay_temp_buf_new (width, height, format);

  gegl_buffer_get (clay_pickable_get_buffer (pickable),
                   GEGL_RECTANGLE (bounding_box.x * scale,
                                   bounding_box.y * scale,
                                   width,
                                   height),
                   scale,
                   clay_temp_buf_get_format (buf),
                   clay_temp_buf_get_data (buf),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return buf;
}

static GdkPixbuf *
clay_image_proxy_get_new_pixbuf (ClayViewable *viewable,
                                 ClayContext  *context,
                                 gint          width,
                                 gint          height,
                                 gint          scale_factor,
                                 GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayImageProxy     *image_proxy = CLAY_IMAGE_PROXY (viewable);
  ClayImage          *image       = image_proxy->priv->image;
  ClayPickable       *pickable;
  GeglRectangle       bounding_box;
  GdkPixbuf          *pixbuf;
  gdouble             scale_x;
  gdouble             scale_y;
  gdouble             scale;
  ClayColorTransform *transform;

  pickable     = clay_image_proxy_get_pickable     (image_proxy);
  bounding_box = clay_image_proxy_get_bounding_box (image_proxy);

  width  *= scale_factor;
  height *= scale_factor;

  scale_x = (gdouble) width  / (gdouble) bounding_box.width;
  scale_y = (gdouble) height / (gdouble) bounding_box.height;

  scale = MIN (scale_x, scale_y);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  transform = clay_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      ClayTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = clay_temp_buf_new (width, height,
                                    clay_pickable_get_format (pickable));

      gegl_buffer_get (clay_pickable_get_buffer (pickable),
                       GEGL_RECTANGLE (bounding_box.x * scale,
                                       bounding_box.y * scale,
                                       width,
                                       height),
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
                                                           width, height),
                                           dest_buf,
                                           GEGL_RECTANGLE (0, 0, 0, 0));

      g_object_unref (src_buf);
      g_object_unref (dest_buf);
    }
  else
    {
      gegl_buffer_get (clay_pickable_get_buffer (pickable),
                       GEGL_RECTANGLE (bounding_box.x * scale,
                                       bounding_box.y * scale,
                                       width,
                                       height),
                       scale,
                       clay_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static gchar *
clay_image_proxy_get_description (ClayViewable  *viewable,
                                  gchar        **tooltip)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (viewable);
  ClayImage      *image       = image_proxy->priv->image;

  if (tooltip)
    *tooltip = g_strdup (clay_image_get_display_path (image));

  return g_strdup_printf ("%s-%d",
                          clay_image_get_display_name (image),
                          clay_image_get_id (image));
}

static void
clay_image_proxy_flush (ClayPickable *pickable)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  clay_pickable_flush (proxy_pickable);
}

static const Babl *
clay_image_proxy_get_format (ClayPickable *pickable)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  return clay_pickable_get_format (proxy_pickable);
}

static const Babl *
clay_image_proxy_get_format_with_alpha (ClayPickable *pickable)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  return clay_pickable_get_format_with_alpha (proxy_pickable);
}

static GeglBuffer *
clay_image_proxy_get_buffer (ClayPickable *pickable)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  return clay_pickable_get_buffer (proxy_pickable);
}

static gboolean
clay_image_proxy_get_pixel_at (ClayPickable *pickable,
                               gint          x,
                               gint          y,
                               const Babl   *format,
                               gpointer      pixel)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  return clay_pickable_get_pixel_at (proxy_pickable, x, y, format, pixel);
}

static gdouble
clay_image_proxy_get_opacity_at (ClayPickable *pickable,
                                 gint          x,
                                 gint          y)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  return clay_pickable_get_opacity_at (proxy_pickable, x, y);
}

static void
clay_image_proxy_get_pixel_average (ClayPickable        *pickable,
                                    const GeglRectangle *rect,
                                    const Babl          *format,
                                    gpointer             pixel)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (pickable);
  ClayPickable   *proxy_pickable;

  proxy_pickable = clay_image_proxy_get_pickable (image_proxy);

  clay_pickable_get_pixel_average (proxy_pickable, rect, format, pixel);
}

static const guint8 *
clay_image_proxy_get_icc_profile (ClayColorManaged *managed,
                                  gsize            *len)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (managed);

  return clay_color_managed_get_icc_profile (
    CLAY_COLOR_MANAGED (image_proxy->priv->image),
    len);
}

static ClayColorProfile *
clay_image_proxy_get_color_profile (ClayColorManaged *managed)
{
  ClayImageProxy *image_proxy = CLAY_IMAGE_PROXY (managed);

  return clay_color_managed_get_color_profile (
    CLAY_COLOR_MANAGED (image_proxy->priv->image));
}

static void
clay_image_proxy_profile_changed (ClayColorManaged *managed)
{
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (managed));
}

static void
clay_image_proxy_image_frozen_notify (ClayImage        *image,
                                      const GParamSpec *pspec,
                                      ClayImageProxy   *image_proxy)
{
  clay_image_proxy_update_frozen (image_proxy);
}

static void
clay_image_proxy_image_invalidate_preview (ClayImage      *image,
                                           ClayImageProxy *image_proxy)
{
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (image_proxy));
}

static void
clay_image_proxy_image_size_changed (ClayImage      *image,
                                     ClayImageProxy *image_proxy)
{
  clay_image_proxy_update_bounding_box (image_proxy);
}

static void
clay_image_proxy_image_bounds_changed (ClayImage      *image,
                                       gint            old_x,
                                       gint            old_y,
                                       ClayImageProxy *image_proxy)
{
  clay_image_proxy_update_bounding_box (image_proxy);
}

static void
clay_image_proxy_image_profile_changed (ClayImage      *image,
                                        ClayImageProxy *image_proxy)
{
  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (image_proxy));
}

static void
clay_image_proxy_set_image (ClayImageProxy *image_proxy,
                            ClayImage      *image)
{
  if (image_proxy->priv->image)
    {
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        clay_image_proxy_image_frozen_notify,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        clay_image_proxy_image_invalidate_preview,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        clay_image_proxy_image_size_changed,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        clay_image_proxy_image_bounds_changed,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        clay_image_proxy_image_profile_changed,
        image_proxy);

      g_object_unref (image_proxy->priv->image);
    }

  image_proxy->priv->image = image;

  if (image_proxy->priv->image)
    {
      g_object_ref (image_proxy->priv->image);

      g_signal_connect (
        image_proxy->priv->image, "notify::frozen",
        G_CALLBACK (clay_image_proxy_image_frozen_notify),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "invalidate-preview",
        G_CALLBACK (clay_image_proxy_image_invalidate_preview),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "size-changed",
        G_CALLBACK (clay_image_proxy_image_size_changed),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "bounds-changed",
        G_CALLBACK (clay_image_proxy_image_bounds_changed),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "profile-changed",
        G_CALLBACK (clay_image_proxy_image_profile_changed),
        image_proxy);

      clay_image_proxy_update_bounding_box (image_proxy);
      clay_image_proxy_update_frozen (image_proxy);

      clay_viewable_invalidate_preview (CLAY_VIEWABLE (image_proxy));
    }
}

static ClayPickable *
clay_image_proxy_get_pickable (ClayImageProxy *image_proxy)
{
  ClayImage *image = image_proxy->priv->image;

  if (! image_proxy->priv->show_all)
    return CLAY_PICKABLE (image);
  else
    return CLAY_PICKABLE (clay_image_get_projection (image));
}

static void
clay_image_proxy_update_bounding_box (ClayImageProxy *image_proxy)
{
  ClayImage     *image = image_proxy->priv->image;
  GeglRectangle  bounding_box;

  if (clay_viewable_preview_is_frozen (CLAY_VIEWABLE (image_proxy)))
    return;

  if (! image_proxy->priv->show_all)
    {
      bounding_box.x      = 0;
      bounding_box.y      = 0;
      bounding_box.width  = clay_image_get_width  (image);
      bounding_box.height = clay_image_get_height (image);
    }
  else
    {
      bounding_box = clay_projectable_get_bounding_box (
        CLAY_PROJECTABLE (image));
    }

  if (! gegl_rectangle_equal (&bounding_box,
                              &image_proxy->priv->bounding_box))
    {
      image_proxy->priv->bounding_box = bounding_box;

      clay_viewable_size_changed (CLAY_VIEWABLE (image_proxy));
    }
}

static void
clay_image_proxy_update_frozen (ClayImageProxy *image_proxy)
{
  gboolean frozen;

  frozen = clay_viewable_preview_is_frozen (
    CLAY_VIEWABLE (image_proxy->priv->image));

  if (frozen != image_proxy->priv->frozen)
    {
      image_proxy->priv->frozen = frozen;

      if (frozen)
        {
          clay_viewable_preview_freeze (CLAY_VIEWABLE (image_proxy));
        }
      else
        {
          clay_viewable_preview_thaw (CLAY_VIEWABLE (image_proxy));

          clay_image_proxy_update_bounding_box (image_proxy);
        }
    }
}


/*  public functions  */


ClayImageProxy *
clay_image_proxy_new (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return g_object_new (CLAY_TYPE_IMAGE_PROXY,
                       "image", image,
                       NULL);
}

ClayImage *
clay_image_proxy_get_image (ClayImageProxy *image_proxy)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_PROXY (image_proxy), NULL);

  return image_proxy->priv->image;
}

void
clay_image_proxy_set_show_all (ClayImageProxy *image_proxy,
                               gboolean        show_all)
{
  g_return_if_fail (CLAY_IS_IMAGE_PROXY (image_proxy));

  if (show_all != image_proxy->priv->show_all)
    {
      image_proxy->priv->show_all = show_all;

      clay_image_proxy_update_bounding_box (image_proxy);
    }
}

gboolean
clay_image_proxy_get_show_all (ClayImageProxy *image_proxy)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_PROXY (image_proxy), FALSE);

  return image_proxy->priv->show_all;
}

GeglRectangle
clay_image_proxy_get_bounding_box (ClayImageProxy *image_proxy)
{
  g_return_val_if_fail (CLAY_IS_IMAGE_PROXY (image_proxy),
                        *GEGL_RECTANGLE (0, 0, 0, 0));

  return image_proxy->priv->bounding_box;
}
