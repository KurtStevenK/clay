/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaycolor/claycolor.h"

#include "claywidgetstypes.h"

#include "claypreviewarea.h"
#include "claywidgetsutils.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claypreviewarea
 * @title: ClayPreviewArea
 * @short_description: A general purpose preview widget which caches
 *                     its pixel data.
 *
 * A general purpose preview widget which caches its pixel data.
 **/


enum
{
  PROP_0,
  PROP_CHECK_SIZE,
  PROP_CHECK_TYPE,
  PROP_CHECK_CUSTOM_COLOR1,
  PROP_CHECK_CUSTOM_COLOR2
};


#define DEFAULT_CHECK_SIZE  CLAY_CHECK_SIZE_MEDIUM_CHECKS
#define DEFAULT_CHECK_TYPE  CLAY_CHECK_TYPE_GRAY_CHECKS

#define CHECK_R(are, row, col)        \
  (((((area)->offset_y + (row)) & size) ^  \
    (((area)->offset_x + (col)) & size)) ? rgb1[0] : rgb2[0])

#define CHECK_G(area, row, col)        \
  (((((area)->offset_y + (row)) & size) ^  \
    (((area)->offset_x + (col)) & size)) ? rgb1[1] : rgb2[1])

#define CHECK_B(area, row, col)        \
  (((((area)->offset_y + (row)) & size) ^  \
    (((area)->offset_x + (col)) & size)) ? rgb1[2] : rgb2[2])


struct _ClayPreviewArea
{
  GtkDrawingArea      parent_instance;

  ClayCheckSize       check_size;
  ClayCheckType       check_type;
  GeglColor          *check_custom_color1;
  GeglColor          *check_custom_color2;
  gint                width;
  gint                height;
  gint                rowstride;
  gint                offset_x;
  gint                offset_y;
  gint                max_width;
  gint                max_height;
  guchar             *buf;
  guchar             *colormap;

  ClayColorConfig    *config;
  ClayColorTransform *transform;
};


static void      clay_preview_area_dispose           (GObject          *object);
static void      clay_preview_area_finalize          (GObject          *object);
static void      clay_preview_area_set_property      (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void      clay_preview_area_get_property      (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void      clay_preview_area_size_allocate     (GtkWidget        *widget,
                                                      GtkAllocation    *allocation);
static gboolean  clay_preview_area_widget_draw       (GtkWidget        *widget,
                                                      cairo_t          *cr);

static void      clay_preview_area_queue_draw        (ClayPreviewArea  *area,
                                                      gint              x,
                                                      gint              y,
                                                      gint              width,
                                                      gint              height);
static gint      clay_preview_area_image_type_bytes  (ClayImageType     type);

static void      clay_preview_area_create_transform  (ClayPreviewArea  *area);
static void      clay_preview_area_destroy_transform (ClayPreviewArea  *area);


G_DEFINE_TYPE (ClayPreviewArea, clay_preview_area, GTK_TYPE_DRAWING_AREA)

#define parent_class clay_preview_area_parent_class


static void
clay_preview_area_class_init (ClayPreviewAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GeglColor      *color1_default;
  GeglColor      *color2_default;

  color1_default = gegl_color_new (NULL);
  gegl_color_set_pixel (color1_default, babl_format ("R'G'B'A double"), CLAY_CHECKS_CUSTOM_COLOR1);
  color2_default = gegl_color_new (NULL);
  gegl_color_set_pixel (color2_default, babl_format ("R'G'B'A double"), CLAY_CHECKS_CUSTOM_COLOR2);

  object_class->dispose       = clay_preview_area_dispose;
  object_class->finalize      = clay_preview_area_finalize;
  object_class->set_property  = clay_preview_area_set_property;
  object_class->get_property  = clay_preview_area_get_property;

  widget_class->size_allocate = clay_preview_area_size_allocate;
  widget_class->draw          = clay_preview_area_widget_draw;

  g_object_class_install_property (object_class, PROP_CHECK_SIZE,
                                   g_param_spec_enum ("check-size",
                                                      _("Check Size"),
                                                      "The size of the checkerboard pattern indicating transparency",
                                                      CLAY_TYPE_CHECK_SIZE,
                                                      DEFAULT_CHECK_SIZE,
                                                      CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CHECK_TYPE,
                                   g_param_spec_enum ("check-type",
                                                      _("Check Style"),
                                                      "The colors of the checkerboard pattern indicating transparency",
                                                      CLAY_TYPE_CHECK_TYPE,
                                                      DEFAULT_CHECK_TYPE,
                                                      CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CHECK_CUSTOM_COLOR1,
                                   clay_param_spec_color ("check-custom-color1",
                                                          _("Custom Checks Color 1"),
                                                          "The first color of the checkerboard pattern indicating transparency",
                                                          FALSE, color1_default,
                                                          CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CHECK_CUSTOM_COLOR2,
                                   clay_param_spec_color ("check-custom-color2",
                                                          _("Custom Checks Color 2"),
                                                          "The second color of the checkerboard pattern indicating transparency",
                                                          FALSE, color2_default,
                                                          CLAY_PARAM_READWRITE));

  g_object_unref (color1_default);
  g_object_unref (color2_default);
}

static void
clay_preview_area_init (ClayPreviewArea *area)
{
  area->check_size          = DEFAULT_CHECK_SIZE;
  area->check_type          = DEFAULT_CHECK_TYPE;
  area->check_custom_color1 = gegl_color_new (NULL);
  gegl_color_set_pixel (area->check_custom_color1, babl_format ("R'G'B'A double"), CLAY_CHECKS_CUSTOM_COLOR1);
  area->check_custom_color2 = gegl_color_new (NULL);
  gegl_color_set_pixel (area->check_custom_color2, babl_format ("R'G'B'A double"), CLAY_CHECKS_CUSTOM_COLOR2);
  area->max_width           = -1;
  area->max_height          = -1;

  clay_widget_track_monitor (GTK_WIDGET (area),
                             G_CALLBACK (clay_preview_area_destroy_transform),
                             NULL, NULL);
}

static void
clay_preview_area_dispose (GObject *object)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (object);

  clay_preview_area_set_color_config (area, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_preview_area_finalize (GObject *object)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (object);

  g_clear_pointer (&area->buf,      g_free);
  g_clear_pointer (&area->colormap, g_free);
  g_clear_object (&area->check_custom_color1);
  g_clear_object (&area->check_custom_color2);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_preview_area_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (object);

  switch (property_id)
    {
    case PROP_CHECK_SIZE:
      area->check_size = g_value_get_enum (value);
      break;
    case PROP_CHECK_TYPE:
      area->check_type = g_value_get_enum (value);
      break;
    case PROP_CHECK_CUSTOM_COLOR1:
      g_clear_object (&area->check_custom_color1);
      area->check_custom_color1 = gegl_color_duplicate (g_value_get_object (value));
      break;
    case PROP_CHECK_CUSTOM_COLOR2:
      g_clear_object (&area->check_custom_color2);
      area->check_custom_color2 = gegl_color_duplicate (g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_preview_area_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (object);

  switch (property_id)
    {
    case PROP_CHECK_SIZE:
      g_value_set_enum (value, area->check_size);
      break;
    case PROP_CHECK_TYPE:
      g_value_set_enum (value, area->check_type);
      break;
    case PROP_CHECK_CUSTOM_COLOR1:
      g_value_set_object (value, area->check_custom_color1);
      break;
    case PROP_CHECK_CUSTOM_COLOR2:
      g_value_set_object (value, area->check_custom_color2);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (widget);
  gint             width;
  gint             height;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  width  = (area->max_width > 0 ?
            MIN (allocation->width, area->max_width) : allocation->width);
  height = (area->max_height > 0 ?
            MIN (allocation->height, area->max_height) : allocation->height);

  if (width  != area->width || height != area->height)
    {
      if (area->buf)
        {
          g_free (area->buf);

          area->buf       = NULL;
          area->rowstride = 0;
        }

      area->width  = width;
      area->height = height;
    }
}

static gboolean
clay_preview_area_widget_draw (GtkWidget *widget,
                               cairo_t   *cr)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (widget);
  GtkAllocation    allocation;
  GdkPixbuf       *pixbuf;
  GdkRectangle     rect;

  if (! area->buf)
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  rect.x      = (allocation.width  - area->width)  / 2;
  rect.y      = (allocation.height - area->height) / 2;
  rect.width  = area->width;
  rect.height = area->height;

  if (! area->transform)
    clay_preview_area_create_transform (area);

  if (area->transform)
    {
      const Babl *format    = babl_format ("R'G'B' u8");
      gint        rowstride = ((area->width * 3) + 3) & ~3;
      guchar     *buf       = g_new (guchar, rowstride * area->height);
      guchar     *src       = area->buf;
      guchar     *dest      = buf;
      gint        i;

      for (i = 0; i < area->height; i++)
        {
          clay_color_transform_process_pixels (area->transform,
                                               format, src,
                                               format, dest,
                                               area->width);

          src  += area->rowstride;
          dest += rowstride;
        }

      pixbuf = gdk_pixbuf_new_from_data (buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         rect.width,
                                         rect.height,
                                         rowstride,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
    }
  else
    {
      pixbuf = gdk_pixbuf_new_from_data (area->buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         rect.width,
                                         rect.height,
                                         area->rowstride,
                                         NULL, NULL);
    }

  gdk_cairo_set_source_pixbuf (cr, pixbuf, rect.x, rect.y);
  cairo_paint (cr);

  g_object_unref (pixbuf);

  return FALSE;
}

static void
clay_preview_area_queue_draw (ClayPreviewArea *area,
                              gint             x,
                              gint             y,
                              gint             width,
                              gint             height)
{
  GtkWidget     *widget = GTK_WIDGET (area);
  GtkAllocation  allocation;

  gtk_widget_get_allocation (widget, &allocation);

  x += (allocation.width  - area->width)  / 2;
  y += (allocation.height - area->height) / 2;

  gtk_widget_queue_draw_area (widget, x, y, width, height);
}

static gint
clay_preview_area_image_type_bytes (ClayImageType type)
{
  switch (type)
    {
    case CLAY_GRAY_IMAGE:
    case CLAY_INDEXED_IMAGE:
      return 1;

    case CLAY_GRAYA_IMAGE:
    case CLAY_INDEXEDA_IMAGE:
      return 2;

    case CLAY_RGB_IMAGE:
      return 3;

    case CLAY_RGBA_IMAGE:
      return 4;

    default:
      g_return_val_if_reached (0);
      break;
    }
}

static void
clay_preview_area_create_transform (ClayPreviewArea *area)
{
  if (area->config)
    {
      static ClayColorProfile *profile = NULL;

      const Babl *format = babl_format ("R'G'B' u8");

      if (G_UNLIKELY (! profile))
        profile = clay_color_profile_new_rgb_srgb ();

      area->transform = clay_widget_get_color_transform (GTK_WIDGET (area),
                                                         area->config,
                                                         profile,
                                                         format,
                                                         format,
                                                         NULL,
                                                         CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                         FALSE);
    }
}

static void
clay_preview_area_destroy_transform (ClayPreviewArea *area)
{
  if (area->transform)
    {
      g_object_unref (area->transform);
      area->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (area));
}


/**
 * clay_preview_area_new:
 *
 * Creates a new #ClayPreviewArea widget.
 *
 * If the preview area is used to draw an image with transparency, you
 * might want to default the checkboard size and colors to user-set
 * Preferences. To do this, you may set the following properties on the
 * newly created #ClayPreviewArea:
 *
 * |[<!-- language="C" -->
 * g_object_set (area,
 *               "check-size",          clay_check_size (),
 *               "check-type",          clay_check_type (),
 *               "check-custom-color1", clay_check_custom_color1 (),
 *               "check-custom-color2", clay_check_custom_color2 (),
 *               NULL);
 * ]|
 *
 * Returns: a new #ClayPreviewArea widget.
 *
 * Since Clay 2.2
 **/
GtkWidget *
clay_preview_area_new (void)
{
  return g_object_new (CLAY_TYPE_PREVIEW_AREA, NULL);
}

/**
 * clay_preview_area_draw:
 * @area:      a #ClayPreviewArea widget.
 * @x:         x offset in preview
 * @y:         y offset in preview
 * @width:     buffer width
 * @height:    buffer height
 * @type:      the #ClayImageType of @buf
 * @buf: (array): a #guchar buffer that contains the preview pixel data.
 * @rowstride: rowstride of @buf
 *
 * Draws @buf on @area and queues a redraw on the given rectangle.
 *
 * Since Clay 2.2
 **/
void
clay_preview_area_draw (ClayPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        ClayImageType    type,
                        const guchar    *buf,
                        gint             rowstride)
{
  const guchar *src;
  guchar       *dest;
  guint         size;
  GeglColor    *color1;
  GeglColor    *color2;
  guchar        rgb1[3];
  guchar        rgb2[3];
  gint          row;
  gint          col;

  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  if (width == 0 || height == 0)
    return;

  g_return_if_fail (buf != NULL);
  g_return_if_fail (rowstride > 0);

  if (x + width < 0 || x >= area->width)
    return;

  if (y + height < 0 || y >= area->height)
    return;

  if (x < 0)
    {
      gint bpp = clay_preview_area_image_type_bytes (type);

      buf -= x * bpp;
      width += x;

      x = 0;
    }

  if (x + width > area->width)
    width = area->width - x;

  if (y < 0)
    {
      buf -= y * rowstride;
      height += y;

      y = 0;
    }

  if (y + height > area->height)
    height = area->height - y;

  if (! area->buf)
    {
      area->rowstride = ((area->width * 3) + 3) & ~3;
      area->buf = g_new0 (guchar, area->rowstride * area->height);
    }

  size = 1 << (2 + area->check_size);
  color1 = area->check_custom_color1;
  color2 = area->check_custom_color2;
  clay_checks_get_colors (area->check_type, &color1, &color2);
  gegl_color_get_pixel (color1, babl_format ("R'G'B' u8"), rgb1);
  gegl_color_get_pixel (color2, babl_format ("R'G'B' u8"), rgb2);
  g_object_unref (color1);
  g_object_unref (color2);

  src  = buf;
  dest = area->buf + x * 3 + y * area->rowstride;

  switch (type)
    {
    case CLAY_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          memcpy (dest, src, 3 * width);

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case CLAY_RGBA_IMAGE:
       for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 4, d+= 3)
            {
              switch (s[3])
                {
                case 0:
                  d[0] = CHECK_R (area, row, col);
                  d[1] = CHECK_G (area, row, col);
                  d[2] = CHECK_B (area, row, col);
                  break;

                case 255:
                  d[0] = s[0];
                  d[1] = s[1];
                  d[2] = s[2];
                  break;

                default:
                  {
                    register guint alpha   = s[3] + 1;
                    register guint check_r = CHECK_R (area, row, col);
                    register guint check_g = CHECK_G (area, row, col);
                    register guint check_b = CHECK_B (area, row, col);

                    d[0] = ((check_r << 8) + (s[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (s[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (s[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += area->rowstride;
        }
       break;

    case CLAY_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = 0; col < width; col++, s++, d += 3)
            {
              d[0] = d[1] = d[2] = s[0];
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case CLAY_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 2, d+= 3)
            {
              switch (s[1])
                {
                case 0:
                  d[0] = CHECK_R (area, row, col);
                  d[1] = CHECK_G (area, row, col);
                  d[2] = CHECK_B (area, row, col);
                  break;

                case 255:
                  d[0] = d[1] = d[2] = s[0];
                  break;

                default:
                  {
                    register guint alpha   = s[1] + 1;
                    register guint check_r = CHECK_R (area, row, col);
                    register guint check_g = CHECK_G (area, row, col);
                    register guint check_b = CHECK_B (area, row, col);

                    d[0] = ((check_r << 8) + (s[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (s[0] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (s[0] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case CLAY_INDEXED_IMAGE:
      g_return_if_fail (area->colormap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = 0; col < width; col++, s++, d += 3)
            {
              const guchar *colormap = area->colormap + 3 * s[0];

              d[0] = colormap[0];
              d[1] = colormap[1];
              d[2] = colormap[2];
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case CLAY_INDEXEDA_IMAGE:
      g_return_if_fail (area->colormap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 2, d += 3)
            {
              const guchar *colormap  = area->colormap + 3 * s[0];

              switch (s[1])
                {
                case 0:
                  d[0] = CHECK_R (area, row, col);
                  d[1] = CHECK_G (area, row, col);
                  d[2] = CHECK_B (area, row, col);
                  break;

                case 255:
                  d[0] = colormap[0];
                  d[1] = colormap[1];
                  d[2] = colormap[2];
                  break;

                default:
                  {
                    register guint alpha   = s[3] + 1;
                    register guint check_r = CHECK_R (area, row, col);
                    register guint check_g = CHECK_G (area, row, col);
                    register guint check_b = CHECK_B (area, row, col);

                    d[0] = ((check_r << 8) + (colormap[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (colormap[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (colormap[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;
    }

  clay_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * clay_preview_area_blend:
 * @area:       a #ClayPreviewArea widget.
 * @x:          x offset in preview
 * @y:          y offset in preview
 * @width:      buffer width
 * @height:     buffer height
 * @type:       the #ClayImageType of @buf1 and @buf2
 * @buf1: (array): a #guchar buffer that contains the pixel data for
 *                 the lower layer
 * @rowstride1: rowstride of @buf1
 * @buf2: (array): a #guchar buffer that contains the pixel data for
 *                 the upper layer
 * @rowstride2: rowstride of @buf2
 * @opacity:    The opacity of the first layer.
 *
 * Composites @buf1 on @buf2 with the given @opacity, draws the result
 * to @area and queues a redraw on the given rectangle.
 *
 * Since Clay 2.2
 **/
void
clay_preview_area_blend (ClayPreviewArea *area,
                         gint             x,
                         gint             y,
                         gint             width,
                         gint             height,
                         ClayImageType    type,
                         const guchar    *buf1,
                         gint             rowstride1,
                         const guchar    *buf2,
                         gint             rowstride2,
                         guchar           opacity)
{
  const guchar *src1;
  const guchar *src2;
  guchar       *dest;
  guint         size;
  GeglColor    *color1;
  GeglColor    *color2;
  guchar        rgb1[3];
  guchar        rgb2[3];
  gint          row;
  gint          col;
  gint          i;

  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  if (width == 0 || height == 0)
    return;

  g_return_if_fail (buf1 != NULL);
  g_return_if_fail (buf2 != NULL);
  g_return_if_fail (rowstride1 > 0);
  g_return_if_fail (rowstride2 > 0);

  switch (opacity)
    {
    case 0:
      clay_preview_area_draw (area, x, y, width, height,
                              type, buf1, rowstride1);
      return;

    case 255:
      clay_preview_area_draw (area, x, y, width, height,
                              type, buf2, rowstride2);
      return;

    default:
      break;
    }

  if (x + width < 0 || x >= area->width)
    return;

  if (y + height < 0 || y >= area->height)
    return;

  if (x < 0)
    {
      gint bpp = clay_preview_area_image_type_bytes (type);

      buf1 -= x * bpp;
      buf2 -= x * bpp;
      width += x;

      x = 0;
    }

  if (x + width > area->width)
    width = area->width - x;

  if (y < 0)
    {
      buf1 -= y * rowstride1;
      buf2 -= y * rowstride2;
      height += y;

      y = 0;
    }

  if (y + height > area->height)
    height = area->height - y;

  if (! area->buf)
    {
      area->rowstride = ((area->width * 3) + 3) & ~3;
      area->buf = g_new0 (guchar, area->rowstride * area->height);
    }

  size = 1 << (2 + area->check_size);
  color1 = area->check_custom_color1;
  color2 = area->check_custom_color2;
  clay_checks_get_colors (area->check_type, &color1, &color2);
  gegl_color_get_pixel (color1, babl_format ("R'G'B' u8"), rgb1);
  gegl_color_get_pixel (color2, babl_format ("R'G'B' u8"), rgb2);
  g_object_unref (color1);
  g_object_unref (color2);

  src1 = buf1;
  src2 = buf2;
  dest = area->buf + x * 3 + y * area->rowstride;

  switch (type)
    {
    case CLAY_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 3, s2 += 3, d+= 3)
            {
              d[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
              d[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;
              d[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * opacity) >> 8;
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += area->rowstride;
        }
      break;

    case CLAY_RGBA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 4, s2 += 4, d+= 3)
            {
              guchar inter[4];

              if (s1[3] == s2[3])
                {
                  inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
                  inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;
                  inter[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * opacity) >> 8;
                  inter[3] = s1[3];
                }
              else
                {
                  inter[3] = ((s1[3] << 8) + (s2[3] - s1[3]) * opacity) >> 8;

                  if (inter[3])
                    {
                      for (i = 0; i < 3; i++)
                        {
                          gushort a = s1[i] * s1[3];
                          gushort b = s2[i] * s2[3];

                          inter[i] =
                            (((a << 8) + (b  - a) * opacity) >> 8) / inter[3];
                        }
                    }
                }

              switch (inter[3])
                {
                case 0:
                  d[0] = CHECK_R (area, row, col);
                  d[1] = CHECK_G (area, row, col);
                  d[2] = CHECK_B (area, row, col);
                  break;

                case 255:
                  d[0] = inter[0];
                  d[1] = inter[1];
                  d[2] = inter[2];
                  break;

                default:
                  {
                    register guint alpha   = inter[3] + 1;
                    register guint check_r = CHECK_R (area, row, col);
                    register guint check_g = CHECK_G (area, row, col);
                    register guint check_b = CHECK_B (area, row, col);

                    d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (inter[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (inter[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += area->rowstride;
        }
      break;

    case CLAY_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, d += 3)
            {
              d[0] = d[1] = d[2] =
                ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += area->rowstride;
        }
      break;

    case CLAY_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, d+= 3)
            {
              guchar inter[2] = { 0, };

              if (s1[1] == s2[1])
                {
                  inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
                  inter[1] = s1[1];
                }
              else
                {
                  inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;

                  if (inter[1])
                    {
                      gushort a = s1[0] * s1[1];
                      gushort b = s2[0] * s2[1];

                      inter[0] =
                        (((a << 8) + (b  - a) * opacity) >> 8) / inter[1];
                    }
                }

              switch (inter[1])
                {
                case 0:
                  d[0] = CHECK_R (area, row, col);
                  d[1] = CHECK_G (area, row, col);
                  d[2] = CHECK_B (area, row, col);
                  break;

                case 255:
                  d[0] = d[1] = d[2] = inter[0];
                  break;

                default:
                  {
                    register guint alpha   = inter[1] + 1;
                    register guint check_r = CHECK_R (area, row, col);
                    register guint check_g = CHECK_G (area, row, col);
                    register guint check_b = CHECK_B (area, row, col);

                    d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (inter[0] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (inter[0] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += area->rowstride;
        }
      break;

    case CLAY_INDEXED_IMAGE:
      g_return_if_fail (area->colormap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, d += 3)
            {
              const guchar *cmap1 = area->colormap + 3 * s1[0];
              const guchar *cmap2 = area->colormap + 3 * s2[0];

              d[0] = ((cmap1[0] << 8) + (cmap2[0] - cmap1[0]) * opacity) >> 8;
              d[1] = ((cmap1[1] << 8) + (cmap2[1] - cmap1[1]) * opacity) >> 8;
              d[2] = ((cmap1[2] << 8) + (cmap2[2] - cmap1[2]) * opacity) >> 8;
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += area->rowstride;
        }
      break;

    case CLAY_INDEXEDA_IMAGE:
      g_return_if_fail (area->colormap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, d += 3)
            {
              const guchar *cmap1  = area->colormap + 3 * s1[0];
              const guchar *cmap2  = area->colormap + 3 * s2[0];
              guchar        inter[4];

              if (s1[1] == s2[1])
                {
                  inter[0] = (((cmap1[0] << 8) +
                               (cmap2[0] - cmap1[0]) * opacity) >> 8);
                  inter[1] = (((cmap1[1] << 8) +
                               (cmap2[1] - cmap1[1]) * opacity) >> 8);
                  inter[2] = (((cmap1[2] << 8) +
                               (cmap2[2] - cmap1[2]) * opacity) >> 8);
                  inter[3] = s1[1];
                }
              else
                {
                  inter[3] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;

                  if (inter[3])
                    {
                      for (i = 0; i < 3; i++)
                        {
                          gushort a = cmap1[i] * s1[1];
                          gushort b = cmap2[i] * s2[1];

                          inter[i] =
                            (((a << 8) + (b  - a) * opacity) >> 8) / inter[3];
                        }
                    }
                }

              switch (inter[3])
                {
                case 0:
                  d[0] = CHECK_R (area, row, col);
                  d[1] = CHECK_G (area, row, col);
                  d[2] = CHECK_B (area, row, col);
                  break;

                case 255:
                  d[0] = inter[0];
                  d[1] = inter[1];
                  d[2] = inter[2];
                  break;

                default:
                  {
                    register guint alpha   = inter[3] + 1;
                    register guint check_r = CHECK_R (area, row, col);
                    register guint check_g = CHECK_G (area, row, col);
                    register guint check_b = CHECK_B (area, row, col);

                    d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (inter[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (inter[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += area->rowstride;
        }
      break;
    }

  clay_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * clay_preview_area_mask:
 * @area:           a #ClayPreviewArea widget.
 * @x:              x offset in preview
 * @y:              y offset in preview
 * @width:          buffer width
 * @height:         buffer height
 * @type:           the #ClayImageType of @buf1 and @buf2
 * @buf1: (array):  a #guchar buffer that contains the pixel data for
 *                  the lower layer
 * @rowstride1:     rowstride of @buf1
 * @buf2: (array):  a #guchar buffer that contains the pixel data for
 *                  the upper layer
 * @rowstride2:     rowstride of @buf2
 * @mask: (array):  a #guchar buffer representing the mask of the second
 *                  layer.
 * @rowstride_mask: rowstride for the mask.
 *
 * Composites @buf1 on @buf2 with the given @mask, draws the result on
 * @area and queues a redraw on the given rectangle.
 *
 * Since Clay 2.2
 **/
void
clay_preview_area_mask (ClayPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        ClayImageType    type,
                        const guchar    *buf1,
                        gint             rowstride1,
                        const guchar    *buf2,
                        gint             rowstride2,
                        const guchar    *mask,
                        gint             rowstride_mask)
{
  const guchar *src1;
  const guchar *src2;
  const guchar *src_mask;
  guchar       *dest;
  guint         size;
  GeglColor    *color1;
  GeglColor    *color2;
  guchar        rgb1[3];
  guchar        rgb2[3];
  gint          row;
  gint          col;
  gint          i;

  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  if (width == 0 || height == 0)
    return;

  g_return_if_fail (buf1 != NULL);
  g_return_if_fail (buf2 != NULL);
  g_return_if_fail (mask != NULL);
  g_return_if_fail (rowstride1 > 0);
  g_return_if_fail (rowstride2 > 0);
  g_return_if_fail (rowstride_mask > 0);

  if (x + width < 0 || x >= area->width)
    return;

  if (y + height < 0 || y >= area->height)
    return;

  if (x < 0)
    {
      gint bpp = clay_preview_area_image_type_bytes (type);

      buf1 -= x * bpp;
      buf2 -= x * bpp;
      mask -= x;
      width += x;

      x = 0;
    }

  if (x + width > area->width)
    width = area->width - x;

  if (y < 0)
    {
      buf1 -= y * rowstride1;
      buf2 -= y * rowstride2;
      mask -= y * rowstride_mask;
      height += y;

      y = 0;
    }

  if (y + height > area->height)
    height = area->height - y;

  if (! area->buf)
    {
      area->rowstride = ((area->width * 3) + 3) & ~3;
      area->buf = g_new0 (guchar, area->rowstride * area->height);
    }

  size = 1 << (2 + area->check_size);
  color1 = area->check_custom_color1;
  color2 = area->check_custom_color2;
  clay_checks_get_colors (area->check_type, &color1, &color2);
  gegl_color_get_pixel (color1, babl_format ("R'G'B' u8"), rgb1);
  gegl_color_get_pixel (color2, babl_format ("R'G'B' u8"), rgb2);
  g_object_unref (color1);
  g_object_unref (color2);

  src1     = buf1;
  src2     = buf2;
  src_mask = mask;
  dest     = area->buf + x * 3 + y * area->rowstride;

  switch (type)
    {
    case CLAY_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 3, s2 += 3, m++, d+= 3)
            {
               d[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;
               d[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;
               d[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * m[0]) >> 8;
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += area->rowstride;
        }
      break;

    case CLAY_RGBA_IMAGE:
       for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 4, s2 += 4, m++, d+= 3)
            {
              switch (m[0])
                {
                case 0:
                  switch (s1[3])
                    {
                    case 0:
                      d[0] = CHECK_R (area, row, col);
                      d[1] = CHECK_G (area, row, col);
                      d[2] = CHECK_B (area, row, col);
                      break;

                    case 255:
                      d[0] = s1[0];
                      d[1] = s1[1];
                      d[2] = s1[2];
                      break;

                    default:
                      {
                        register guint alpha   = s1[3] + 1;
                        register guint check_r = CHECK_R (area, row, col);
                        register guint check_g = CHECK_G (area, row, col);
                        register guint check_b = CHECK_B (area, row, col);

                        d[0] = ((check_r << 8) + (s1[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s1[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s1[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                case 255:
                  switch (s2[3])
                    {
                    case 0:
                      d[0] = CHECK_R (area, row, col);
                      d[1] = CHECK_G (area, row, col);
                      d[2] = CHECK_B (area, row, col);
                      break;

                    case 255:
                      d[0] = s2[0];
                      d[1] = s2[1];
                      d[2] = s2[2];
                      break;

                    default:
                      {
                        register guint alpha   = s2[3] + 1;
                        register guint check_r = CHECK_R (area, row, col);
                        register guint check_g = CHECK_G (area, row, col);
                        register guint check_b = CHECK_B (area, row, col);

                        d[0] = ((check_r << 8) + (s2[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s2[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s2[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                default:
                  {
                    guchar inter[4];

                    if (s1[3] == s2[3])
                      {
                        inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;
                        inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;
                        inter[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * m[0]) >> 8;
                        inter[3] = s1[3];
                     }
                    else
                      {
                        inter[3] = ((s1[3] << 8) + (s2[3] - s1[3]) * m[0]) >> 8;

                        if (inter[3])
                          {
                            for (i = 0; i < 3; i++)
                             {
                               gushort a = s1[i] * s1[3];
                               gushort b = s2[i] * s2[3];

                               inter[i] =
                                 (((a << 8) + (b  - a) * m[0]) >> 8) / inter[3];
                             }
                         }
                      }

                    switch (inter[3])
                      {
                      case 0:
                        d[0] = CHECK_R (area, row, col);
                        d[1] = CHECK_G (area, row, col);
                        d[2] = CHECK_B (area, row, col);
                        break;

                      case 255:
                        d[0] = inter[0];
                        d[1] = inter[1];
                        d[2] = inter[2];
                        break;

                      default:
                        {
                          register guint alpha   = inter[3] + 1;
                          register guint check_r = CHECK_R (area, row, col);
                          register guint check_g = CHECK_G (area, row, col);
                          register guint check_b = CHECK_B (area, row, col);

                          d[0] = (((check_r << 8) +
                                   (inter[0] - check_r) * alpha) >> 8);
                          d[1] = (((check_g << 8) +
                                   (inter[1] - check_g) * alpha) >> 8);
                          d[2] = (((check_b << 8) +
                                   (inter[2] - check_b) * alpha) >> 8);
                        }
                        break;
                      }
                  }
                  break;
                }
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += area->rowstride;
        }
       break;

    case CLAY_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, m++, d += 3)
            d[0] = d[1] = d[2] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += area->rowstride;
        }
      break;

    case CLAY_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, m++, d+= 3)
            {
              switch (m[0])
                {
                case 0:
                  switch (s1[1])
                    {
                    case 0:
                      d[0] = CHECK_R (area, row, col);
                      d[1] = CHECK_G (area, row, col);
                      d[2] = CHECK_B (area, row, col);
                      break;

                    case 255:
                      d[0] = d[1] = d[2] = s1[0];
                      break;

                    default:
                      {
                        register guint alpha   = s1[1] + 1;
                        register guint check_r = CHECK_R (area, row, col);
                        register guint check_g = CHECK_G (area, row, col);
                        register guint check_b = CHECK_B (area, row, col);

                        d[0] = ((check_r << 8) + (s1[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s1[0] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s1[0] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                case 255:
                  switch (s2[1])
                    {
                    case 0:
                      d[0] = CHECK_R (area, row, col);
                      d[1] = CHECK_G (area, row, col);
                      d[2] = CHECK_B (area, row, col);
                      break;

                    case 255:
                      d[0] = d[1] = d[2] = s2[0];
                      break;

                    default:
                      {
                        register guint alpha   = s2[1] + 1;
                        register guint check_r = CHECK_R (area, row, col);
                        register guint check_g = CHECK_G (area, row, col);
                        register guint check_b = CHECK_B (area, row, col);

                        d[0] = ((check_r << 8) + (s2[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s2[0] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s2[0] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                default:
                  {
                    guchar inter[2] = { 0, };

                    if (s1[1] == s2[1])
                      {
                        inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;
                        inter[1] = s1[1];
                      }
                    else
                      {
                        inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;

                        if (inter[1])
                          {
                            gushort a = s1[0] * s1[1];
                            gushort b = s2[0] * s2[1];

                            inter[0] =
                              (((a << 8) + (b  - a) * m[0]) >> 8) / inter[1];
                          }
                      }

                    switch (inter[1])
                      {
                      case 0:
                        d[0] = CHECK_R (area, row, col);
                        d[1] = CHECK_G (area, row, col);
                        d[2] = CHECK_B (area, row, col);
                        break;

                      case 255:
                        d[0] = d[1] = d[2] = inter[0];
                        break;

                      default:
                        {
                          register guint alpha   = inter[1] + 1;
                          register guint check_r = CHECK_R (area, row, col);
                          register guint check_g = CHECK_G (area, row, col);
                          register guint check_b = CHECK_B (area, row, col);

                          d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                          d[1] = ((check_g << 8) + (inter[0] - check_g) * alpha) >> 8;
                          d[2] = ((check_b << 8) + (inter[0] - check_b) * alpha) >> 8;
                        }
                        break;
                      }
                  }
                  break;
                }
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += area->rowstride;
        }
      break;

    case CLAY_INDEXED_IMAGE:
      g_return_if_fail (area->colormap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, m++, d += 3)
            {
              const guchar *cmap1 = area->colormap + 3 * s1[0];
              const guchar *cmap2 = area->colormap + 3 * s2[0];

              d[0] = ((cmap1[0] << 8) + (cmap2[0] - cmap1[0]) * m[0]) >> 8;
              d[1] = ((cmap1[1] << 8) + (cmap2[1] - cmap1[1]) * m[0]) >> 8;
              d[2] = ((cmap1[2] << 8) + (cmap2[2] - cmap1[2]) * m[0]) >> 8;
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += area->rowstride;
        }
      break;

    case CLAY_INDEXEDA_IMAGE:
      g_return_if_fail (area->colormap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, m++, d += 3)
            {
              const guchar *cmap1  = area->colormap + 3 * s1[0];
              const guchar *cmap2  = area->colormap + 3 * s2[0];

              switch (m[0])
                {
                case 0:
                  switch (s1[1])
                    {
                    case 0:
                      d[0] = CHECK_R (area, row, col);
                      d[1] = CHECK_G (area, row, col);
                      d[2] = CHECK_B (area, row, col);
                      break;

                    case 255:
                      d[0] = cmap1[0];
                      d[1] = cmap1[1];
                      d[2] = cmap1[2];
                      break;

                    default:
                      {
                        register guint alpha   = s1[1] + 1;
                        register guint check_r = CHECK_R (area, row, col);
                        register guint check_g = CHECK_G (area, row, col);
                        register guint check_b = CHECK_B (area, row, col);

                        d[0] = ((check_r << 8) + (cmap1[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (cmap1[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (cmap1[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                case 255:
                  switch (s2[1])
                    {
                    case 0:
                      d[0] = CHECK_R (area, row, col);
                      d[1] = CHECK_G (area, row, col);
                      d[2] = CHECK_B (area, row, col);
                      break;

                    case 255:
                      d[0] = cmap2[0];
                      d[1] = cmap2[1];
                      d[2] = cmap2[2];
                      break;

                    default:
                      {
                        register guint alpha   = s2[1] + 1;
                        register guint check_r = CHECK_R (area, row, col);
                        register guint check_g = CHECK_G (area, row, col);
                        register guint check_b = CHECK_B (area, row, col);

                        d[0] = ((check_r << 8) + (cmap2[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (cmap2[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (cmap2[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                default:
                  {
                    guchar inter[4];

                    if (s1[1] == s2[1])
                      {
                        inter[0] = (((cmap1[0] << 8) +
                                     (cmap2[0] - cmap1[0]) * m[0]) >> 8);
                        inter[1] = (((cmap1[1] << 8) +
                                     (cmap2[1] - cmap1[1]) * m[0]) >> 8);
                        inter[2] = (((cmap1[2] << 8) +
                                     (cmap2[2] - cmap1[2]) * m[0]) >> 8);
                        inter[3] = s1[1];
                      }
                    else
                      {
                        inter[3] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;

                        if (inter[3])
                          {
                            for (i = 0 ; i < 3 ; i++)
                              {
                                gushort a = cmap1[i] * s1[1];
                                gushort b = cmap2[i] * s2[1];

                                inter[i] = ((((a << 8) + (b  - a) * m[0]) >> 8)
                                            / inter[3]);
                              }
                          }
                      }

                    switch (inter[3])
                      {
                      case 0:
                        d[0] = CHECK_R (area, row, col);
                        d[1] = CHECK_G (area, row, col);
                        d[2] = CHECK_B (area, row, col);
                        break;

                      case 255:
                        d[0] = inter[0];
                        d[1] = inter[1];
                        d[2] = inter[2];
                        break;

                      default:
                        {
                          register guint alpha   = inter[3] + 1;
                          register guint check_r = CHECK_R (area, row, col);
                          register guint check_g = CHECK_G (area, row, col);
                          register guint check_b = CHECK_B (area, row, col);

                          d[0] =
                            ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                          d[1] =
                            ((check_g << 8) + (inter[1] - check_g) * alpha) >> 8;
                          d[2] =
                            ((check_b << 8) + (inter[2] - check_b) * alpha) >> 8;
                        }
                        break;
                      }
                  }
                  break;
                }
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += area->rowstride;
        }
      break;
    }

  clay_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * clay_preview_area_fill:
 * @area:   a #ClayPreviewArea widget.
 * @x:      x offset in preview
 * @y:      y offset in preview
 * @width:  width of the rectangle to fill
 * @height: height of the rectangle to fill
 * @red:    red component of the fill color (0-255)
 * @green:  green component of the fill color (0-255)
 * @blue:   red component of the fill color (0-255)
 *
 * Fills the given rectangle of @area in the given color and queues a
 * redraw.
 *
 * Since Clay 2.2
 **/
void
clay_preview_area_fill (ClayPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        guchar           red,
                        guchar           green,
                        guchar           blue)
{
  guchar *dest;
  guchar *d;
  gint    row;
  gint    col;

  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  if (width == 0 || height == 0)
    return;

  if (x + width < 0 || x >= area->width)
    return;

  if (y + height < 0 || y >= area->height)
    return;

  if (x < 0)
    {
      width += x;
      x = 0;
    }

  if (x + width > area->width)
    width = area->width - x;

  if (y < 0)
    {
      height += y;
      y = 0;
    }

  if (y + height > area->height)
    height = area->height - y;

  if (! area->buf)
    {
      area->rowstride = ((area->width * 3) + 3) & ~3;
      area->buf = g_new0 (guchar, area->rowstride * area->height);
    }

  dest = area->buf + x * 3 + y * area->rowstride;

  /*  colorize first row  */
  for (col = 0, d = dest; col < width; col++, d+= 3)
    {
      d[0] = red;
      d[1] = green;
      d[2] = blue;
    }

  /*  copy first row to remaining rows  */
  for (row = 1, d = dest; row < height; row++)
    {
      d += area->rowstride;
      memcpy (d, dest, width * 3);
    }

  clay_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * clay_preview_area_reset:
 * @area:   a #ClayPreviewArea widget.
 *
 * Reset any previous drawing done through [class@ClayUi.PreviewArea] functions.
 *
 * Since: 3.0
 **/
void
clay_preview_area_reset (ClayPreviewArea *area)
{
  GtkAllocation allocation;

  if (area->buf)
    {
      g_free (area->buf);

      area->buf       = NULL;
      area->rowstride = 0;
    }

  gtk_widget_get_allocation (GTK_WIDGET (area), &allocation);
  clay_preview_area_queue_draw (area, 0, 0, allocation.width, allocation.height);
}

/**
 * clay_preview_area_set_offsets:
 * @area: a #ClayPreviewArea
 * @x:    horizontal offset
 * @y:    vertical offset
 *
 * Sets the offsets of the previewed area. This information is used
 * when drawing the checkerboard and to determine the dither offsets.
 *
 * Since: 2.2
 **/
void
clay_preview_area_set_offsets (ClayPreviewArea *area,
                               gint             x,
                               gint             y)
{
  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));

  area->offset_x = x;
  area->offset_y = y;
}

/**
 * clay_preview_area_set_colormap:
 * @area:       a #ClayPreviewArea
 * @colormap: (array): a #guchar buffer that contains the colormap
 * @num_colors: the number of colors in the colormap
 *
 * Sets the colormap for the #ClayPreviewArea widget. You need to
 * call this function before you use clay_preview_area_draw() with
 * an image type of %CLAY_INDEXED_IMAGE or %CLAY_INDEXEDA_IMAGE.
 *
 * Since Clay 2.2
 **/
void
clay_preview_area_set_colormap (ClayPreviewArea *area,
                                const guchar    *colormap,
                                gint             num_colors)
{
  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (colormap != NULL || num_colors == 0);
  g_return_if_fail (num_colors >= 0 && num_colors <= 256);

  if (num_colors > 0)
    {
      if (area->colormap)
        memset (area->colormap, 0, 3 * 256);
      else
        area->colormap = g_new0 (guchar, 3 * 256);

      memcpy (area->colormap, colormap, 3 * num_colors);
    }
  else
    {
      g_free (area->colormap);
      area->colormap = NULL;
    }
}

/**
 * clay_preview_area_set_color_config:
 * @area:   a #ClayPreviewArea widget.
 * @config: a #ClayColorConfig object.
 *
 * Sets the color management configuration to use with this preview area.
 *
 * Since: 2.10
 */
void
clay_preview_area_set_color_config (ClayPreviewArea *area,
                                    ClayColorConfig *config)
{
  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (config == NULL || CLAY_IS_COLOR_CONFIG (config));

  if (config != area->config)
    {
      if (area->config)
        {
          g_signal_handlers_disconnect_by_func (area->config,
                                                clay_preview_area_destroy_transform,
                                                area);

          clay_preview_area_destroy_transform (area);
        }

      g_set_object (&area->config, config);

      if (area->config)
        {
          g_signal_connect_swapped (area->config, "notify",
                                    G_CALLBACK (clay_preview_area_destroy_transform),
                                    area);
        }
    }
}

/**
 * clay_preview_area_get_size:
 * @area:   a #ClayPreviewArea widget.
 * @width: (out): The preview areay width
 * @height: (out): The preview areay height
 *
 * Gets the preview area size
 */
void
clay_preview_area_get_size (ClayPreviewArea *area,
                            gint            *width,
                            gint            *height)
{
  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));

  if (width)  *width  = area->width;
  if (height) *height = area->height;
}

/**
 * clay_preview_area_set_max_size:
 * @area:   a #ClayPreviewArea widget
 * @width:  the maximum width in pixels or -1 to unset the limit
 * @height: the maximum height in pixels or -1 to unset the limit
 *
 * Usually a #ClayPreviewArea fills the size that it is
 * allocated. This function allows you to limit the preview area to a
 * maximum size. If a larger size is allocated for the widget, the
 * preview will draw itself centered into the allocated area.
 *
 * Since: 2.2
 **/
void
clay_preview_area_set_max_size (ClayPreviewArea *area,
                                gint             width,
                                gint             height)
{
  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));

  area->max_width  = width;
  area->max_height = height;
}



/*  popup menu  */

static void
clay_preview_area_menu_toggled (GtkWidget       *item,
                                ClayPreviewArea *area)
{
  gboolean active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));

  if (active)
    {
      const gchar *name = g_object_get_data (G_OBJECT (item),
                                             "clay-preview-area-prop-name");
      if (name)
        {
          gint  value =
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                "clay-preview-area-prop-value"));
          g_object_set (area,
                        name, value,
                        NULL);
        }
    }
}

static GtkWidget *
clay_preview_area_menu_new (ClayPreviewArea *area,
                            const gchar     *property)
{
  GParamSpec *pspec;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  GtkWidget  *menu;
  GtkWidget  *item;
  GSList     *group = NULL;
  gint        value;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (area), property);

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (pspec), NULL);

  g_object_get (area,
                property, &value,
                NULL);

  enum_class = G_PARAM_SPEC_ENUM (pspec)->enum_class;

  menu = gtk_menu_new ();

  for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
    {
      const gchar *name = clay_enum_value_get_desc (enum_class, enum_value);

      item = gtk_radio_menu_item_new_with_label (group, name);

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_set_visible (item, TRUE);

      g_object_set_data (G_OBJECT (item),
                         "clay-preview-area-prop-name",
                         (gpointer) property);

      g_object_set_data (G_OBJECT (item),
                         "clay-preview-area-prop-value",
                         GINT_TO_POINTER (enum_value->value));

      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      (enum_value->value == value));

      g_signal_connect (item, "toggled",
                        G_CALLBACK (clay_preview_area_menu_toggled),
                        area);

      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    }

  item = gtk_menu_item_new_with_label (g_param_spec_get_nick (pspec));

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

  gtk_widget_set_visible (item, TRUE);

  return item;
}

/**
 * clay_preview_area_menu_popup:
 * @area:  a #ClayPreviewArea
 * @event: (nullable): the button event that causes the menu to popup or %NULL
 *
 * Creates a popup menu that allows one to configure the size and type of
 * the checkerboard pattern that the @area uses to visualize transparency.
 *
 * Since: 2.2
 **/
void
clay_preview_area_menu_popup (ClayPreviewArea *area,
                              GdkEventButton  *event)
{
  GtkWidget *menu;

  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));

  menu = gtk_menu_new ();
  gtk_menu_set_screen (GTK_MENU (menu),
                       gtk_widget_get_screen (GTK_WIDGET (area)));

  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         clay_preview_area_menu_new (area, "check-type"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         clay_preview_area_menu_new (area, "check-size"));
#if 0
  /* clay_preview_area_menu_new() currently only handles enum types, and
   * in particular not color properties.
   */
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         clay_preview_area_menu_new (area, "check-custom-color1"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         clay_preview_area_menu_new (area, "check-custom-color2"));
#endif

  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);
}
