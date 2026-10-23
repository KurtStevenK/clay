/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claydrawablepreview.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clayuitypes.h"

#include "clay.h"
#include "claypdb-private.h"

#include "claydrawablepreview.h"


/**
 * SECTION: claydrawablepreview
 * @title: ClayDrawablePreview
 * @short_description: A widget providing a preview of a #ClayDrawable.
 *
 * A widget providing a preview of a #ClayDrawable.
 **/


#define SELECTION_BORDER  8

enum
{
  PROP_0,
  PROP_DRAWABLE
};

typedef struct
{
  gint      x;
  gint      y;
  gboolean  update;
} PreviewSettings;


struct _ClayDrawablePreview
{
  ClayScrolledPreview  parent_instance;

  ClayDrawable        *drawable;
};


static void  clay_drawable_preview_constructed   (GObject         *object);
static void  clay_drawable_preview_dispose       (GObject         *object);
static void  clay_drawable_preview_get_property  (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void  clay_drawable_preview_set_property  (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);

static void  clay_drawable_preview_style_updated (GtkWidget       *widget);

static void  clay_drawable_preview_draw_original (ClayPreview     *preview);
static void  clay_drawable_preview_draw_thumb    (ClayPreview     *preview,
                                                  ClayPreviewArea *area,
                                                  gint             width,
                                                  gint             height);
static void  clay_drawable_preview_draw_buffer   (ClayPreview     *preview,
                                                  const guchar    *buffer,
                                                  gint             rowstride);

static void  clay_drawable_preview_set_drawable  (ClayDrawablePreview *preview,
                                                  ClayDrawable        *drawable);


G_DEFINE_TYPE (ClayDrawablePreview, clay_drawable_preview, CLAY_TYPE_SCROLLED_PREVIEW)

#define parent_class clay_drawable_preview_parent_class

static gint clay_drawable_preview_counter = 0;


static void
clay_drawable_preview_class_init (ClayDrawablePreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  ClayPreviewClass *preview_class = CLAY_PREVIEW_CLASS (klass);

  object_class->constructed   = clay_drawable_preview_constructed;
  object_class->dispose       = clay_drawable_preview_dispose;
  object_class->get_property  = clay_drawable_preview_get_property;
  object_class->set_property  = clay_drawable_preview_set_property;

  widget_class->style_updated = clay_drawable_preview_style_updated;

  preview_class->draw         = clay_drawable_preview_draw_original;
  preview_class->draw_thumb   = clay_drawable_preview_draw_thumb;
  preview_class->draw_buffer  = clay_drawable_preview_draw_buffer;

  /**
   * ClayDrawablePreview:drawable-id:
   *
   * The drawable the #ClayDrawablePreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        "Drawable",
                                                        "The drawable this preview is attached to",
                                                        CLAY_TYPE_DRAWABLE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

}

static void
clay_drawable_preview_init (ClayDrawablePreview *preview)
{
  g_object_set (clay_preview_get_area (CLAY_PREVIEW (preview)),
                "check-size", clay_check_size (),
                "check-type", clay_check_type (),
                "check-custom-color1", clay_check_custom_color1 (),
                "check-custom-color2", clay_check_custom_color2 (),
                NULL);
}

static void
clay_drawable_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;
  GBytes          *settings_bytes = NULL;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-drawable-preview-%d",
                               g_get_prgname (),
                               ++clay_drawable_preview_counter);

  if (clay_pdb_get_data (data_name, &settings_bytes) &&
      g_bytes_get_size (settings_bytes) == sizeof (PreviewSettings))
    {
      settings = *((PreviewSettings *) g_bytes_get_data (settings_bytes, NULL));
      clay_preview_set_update (CLAY_PREVIEW (object), settings.update);
      clay_scrolled_preview_set_position (CLAY_SCROLLED_PREVIEW (object),
                                          settings.x, settings.y);
    }
  g_bytes_unref (settings_bytes);

  g_object_set_data_full (object, "clay-drawable-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
clay_drawable_preview_dispose (GObject *object)
{
  ClayDrawablePreview *preview = CLAY_DRAWABLE_PREVIEW (object);
  const gchar         *data_name;

  data_name = g_object_get_data (G_OBJECT (object),
                                 "clay-drawable-preview-data-name");
  if (data_name)
    {
      ClayPreview     *preview = CLAY_PREVIEW (object);
      GBytes          *bytes;
      PreviewSettings  settings;

      clay_preview_get_position (preview, &settings.x, &settings.y);
      settings.update = clay_preview_get_update (preview);

      bytes = g_bytes_new_static (&settings, sizeof (PreviewSettings));
      clay_pdb_set_data (data_name, bytes);
      g_bytes_unref (bytes);
    }

  g_clear_object (&preview->drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_drawable_preview_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayDrawablePreview *preview = CLAY_DRAWABLE_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value,
                          clay_drawable_preview_get_drawable (preview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_preview_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayDrawablePreview *preview = CLAY_DRAWABLE_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      clay_drawable_preview_set_drawable (preview,
                                          g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_preview_style_updated (GtkWidget *widget)
{
  ClayPreview *preview = CLAY_PREVIEW (widget);
  GtkWidget   *area    = clay_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      gint xmin, ymin;
      gint xmax, ymax;
      gint size;

      clay_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

      gtk_widget_style_get (widget,
                            "size", &size,
                            NULL);

      gtk_widget_set_size_request (area,
                                   MIN (xmax - xmin, size),
                                   MIN (ymax - ymin, size));
    }
}

static void
clay_drawable_preview_draw_original (ClayPreview *preview)
{
  ClayDrawablePreview *drawable_preview = CLAY_DRAWABLE_PREVIEW (preview);
  GBytes              *buffer;
  gint                 width, height;
  gint                 tn_width, tn_height;
  gint                 xoff, yoff;
  gint                 xmin, ymin;
  gint                 xmax, ymax;
  gint                 bpp;
  ClayImageType        type;

  if (drawable_preview->drawable == NULL)
    return;

  clay_preview_get_size (preview, &width, &height);
  clay_preview_get_offsets (preview, &xoff, &yoff);
  clay_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

  xoff = CLAMP (xoff, 0, xmax - xmin - width);
  yoff = CLAMP (yoff, 0, ymax - ymin - height);

  clay_preview_set_offsets (preview, xoff, yoff);

  buffer = clay_drawable_get_sub_thumbnail_data (drawable_preview->drawable,
                                                 xoff + xmin,
                                                 yoff + ymin,
                                                 width, height,
                                                 width, height,
                                                 &tn_width, &tn_height, &bpp);

  switch (bpp)
    {
    case 1: type = CLAY_GRAY_IMAGE; break;
    case 2: type = CLAY_GRAYA_IMAGE; break;
    case 3: type = CLAY_RGB_IMAGE; break;
    case 4: type = CLAY_RGBA_IMAGE; break;
    default:
      g_bytes_unref (buffer);
      return;
    }

  clay_preview_area_draw (CLAY_PREVIEW_AREA (clay_preview_get_area (preview)),
                          0, 0, tn_width, tn_height, type,
                          g_bytes_get_data (buffer, NULL), width * bpp);
  g_bytes_unref (buffer);
}

static void
clay_drawable_preview_draw_thumb (ClayPreview     *preview,
                                  ClayPreviewArea *area,
                                  gint             width,
                                  gint             height)
{
  ClayDrawablePreview *drawable_preview = CLAY_DRAWABLE_PREVIEW (preview);

  if (drawable_preview->drawable)
    _clay_drawable_preview_area_draw_thumb (area, drawable_preview->drawable,
                                            width, height);
}

void
_clay_drawable_preview_area_draw_thumb (ClayPreviewArea *area,
                                        ClayDrawable    *drawable,
                                        gint             width,
                                        gint             height)
{
  GBytes *buffer;
  gint    x1, y1, x2, y2;
  gint    bpp;
  gint    size = 100;
  gint    nav_width, nav_height;
  gint    tn_width, tn_height;

  g_return_if_fail (CLAY_IS_PREVIEW_AREA (area));
  g_return_if_fail (clay_item_is_valid (CLAY_ITEM (drawable)));
  g_return_if_fail (clay_item_is_drawable (CLAY_ITEM (drawable)));

  if (_clay_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      width  = x2 - x1;
      height = y2 - y1;
    }
  else
    {
      width  = clay_drawable_get_width  (drawable);
      height = clay_drawable_get_height (drawable);
    }

  if (width > height)
    {
      nav_width  = MIN (width, size);
      nav_height = (height * nav_width) / width;
    }
  else
    {
      nav_height = MIN (height, size);
      nav_width  = (width * nav_height) / height;
    }

  if (_clay_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      buffer = clay_drawable_get_sub_thumbnail_data (drawable,
                                                     x1, y1, x2 - x1, y2 - y1,
                                                     nav_width, nav_height,
                                                     &tn_width, &tn_height,
                                                     &bpp);
    }
  else
    {
      buffer = clay_drawable_get_thumbnail_data (drawable,
                                                 nav_width, nav_height,
                                                 &tn_width, &tn_height,
                                                 &bpp);
    }

  if (buffer)
    {
      ClayImageType type;

      gtk_widget_set_size_request (GTK_WIDGET (area), tn_width, tn_height);
      gtk_widget_set_visible (GTK_WIDGET (area), TRUE);
      gtk_widget_realize (GTK_WIDGET (area));

      switch (bpp)
        {
        case 1:  type = CLAY_GRAY_IMAGE;   break;
        case 2:  type = CLAY_GRAYA_IMAGE;  break;
        case 3:  type = CLAY_RGB_IMAGE;    break;
        case 4:  type = CLAY_RGBA_IMAGE;   break;
        default:
          g_bytes_unref (buffer);
          return;
        }

      clay_preview_area_draw (area,
                              0, 0, tn_width, tn_height,
                              type,
                              g_bytes_get_data (buffer, NULL), bpp * tn_width);
      g_bytes_unref (buffer);
    }
}

static void
clay_drawable_preview_draw_area (ClayDrawablePreview *preview,
                                 gint                 x,
                                 gint                 y,
                                 gint                 width,
                                 gint                 height,
                                 const guchar        *buf,
                                 gint                 rowstride)
{
  ClayPreview *clay_preview = CLAY_PREVIEW (preview);
  GtkWidget   *area         = clay_preview_get_area (clay_preview);
  ClayImage   *image;
  gint         xmin, ymin;
  gint         xoff, yoff;

  clay_preview_get_bounds (clay_preview, &xmin, &ymin, NULL, NULL);
  clay_preview_get_offsets (clay_preview, &xoff, &yoff);

  image = clay_item_get_image (CLAY_ITEM (preview->drawable));

  if (clay_selection_is_empty (image))
    {
      clay_preview_area_draw (CLAY_PREVIEW_AREA (area),
                              x - xoff - xmin,
                              y - yoff - ymin,
                              width,
                              height,
                              clay_drawable_type (preview->drawable),
                              buf, rowstride);
    }
  else
    {
      gint offset_x, offset_y;
      gint mask_x, mask_y;
      gint mask_width, mask_height;
      gint draw_x, draw_y;
      gint draw_width, draw_height;

      clay_drawable_get_offsets (preview->drawable, &offset_x, &offset_y);

      if (clay_drawable_mask_intersect (preview->drawable,
                                        &mask_x, &mask_y,
                                        &mask_width, &mask_height) &&
          clay_rectangle_intersect (mask_x, mask_y,
                                    mask_width, mask_height,
                                    x, y, width, height,
                                    &draw_x, &draw_y,
                                    &draw_width, &draw_height))
        {
          ClayImageType   type;
          ClaySelection  *selection;
          GBytes         *src;
          GBytes         *sel;
          gint            d_w, d_h, d_bpp;
          gint            s_w, s_h, s_bpp;

          selection = clay_image_get_selection (image);

          src = clay_drawable_get_sub_thumbnail_data (preview->drawable,
                                                      draw_x, draw_y,
                                                      draw_width, draw_height,
                                                      draw_width, draw_height,
                                                      &d_w, &d_h,
                                                      &d_bpp);

          sel = clay_drawable_get_sub_thumbnail_data (CLAY_DRAWABLE (selection),
                                                      draw_x + offset_x,
                                                      draw_y + offset_y,
                                                      draw_width, draw_height,
                                                      draw_width, draw_height,
                                                      &s_w, &s_h,
                                                      &s_bpp);

          switch (d_bpp)
            {
            case 1:  type = CLAY_GRAY_IMAGE;   break;
            case 2:  type = CLAY_GRAYA_IMAGE;  break;
            case 3:  type = CLAY_RGB_IMAGE;    break;
            case 4:  type = CLAY_RGBA_IMAGE;   break;
            default:
              g_bytes_unref (sel);
              g_bytes_unref (src);
              return;
            }

          clay_preview_area_mask (CLAY_PREVIEW_AREA (area),
                                  draw_x - xoff - xmin,
                                  draw_y - yoff - ymin,
                                  draw_width,
                                  draw_height,
                                  type,
                                  g_bytes_get_data (src, NULL), draw_width * d_bpp,
                                  (buf +
                                   (draw_x - x) * d_bpp +
                                   (draw_y - y) * d_w * d_bpp),
                                  rowstride,
                                  g_bytes_get_data (sel, NULL), s_w);

          g_bytes_unref (sel);
          g_bytes_unref (src);
        }
    }
}

static void
clay_drawable_preview_draw_buffer (ClayPreview  *preview,
                                   const guchar *buffer,
                                   gint          rowstride)
{
  gint x, y;
  gint width, height;

  clay_preview_get_position (preview, &x, &y);
  clay_preview_get_size (preview, &width, &height);

  clay_drawable_preview_draw_area (CLAY_DRAWABLE_PREVIEW (preview),
                                   x, y,
                                   width, height,
                                   buffer, rowstride);
}

static void
clay_drawable_preview_set_drawable (ClayDrawablePreview *drawable_preview,
                                    ClayDrawable        *drawable)
{
  ClayPreview *preview = CLAY_PREVIEW (drawable_preview);
  gint         x1, y1, x2, y2;

  g_return_if_fail (drawable_preview->drawable == NULL);

  drawable_preview->drawable = drawable;

  _clay_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2);

  clay_preview_set_bounds (preview, x1, y1, x2, y2);

  if (clay_drawable_is_indexed (drawable))
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (drawable));
      GtkWidget *area  = clay_preview_get_area (preview);
      guchar    *cmap;
      gint       num_colors;

      cmap = clay_palette_get_colormap (clay_image_get_palette (image), babl_format ("R'G'B' u8"), &num_colors, NULL);
      clay_preview_area_set_colormap (CLAY_PREVIEW_AREA (area),
                                      cmap, num_colors);
      g_free (cmap);
    }
}


#define MAX3(a, b, c)  (MAX (MAX ((a), (b)), (c)))
#define MIN3(a, b, c)  (MIN (MIN ((a), (b)), (c)))

gboolean
_clay_drawable_preview_get_bounds (ClayDrawable *drawable,
                                   gint         *xmin,
                                   gint         *ymin,
                                   gint         *xmax,
                                   gint         *ymax)
{
  gint     width;
  gint     height;
  gint     offset_x;
  gint     offset_y;
  gint     x1, y1;
  gint     x2, y2;
  gboolean retval;

  g_return_val_if_fail (clay_item_is_valid (CLAY_ITEM (drawable)), FALSE);
  g_return_val_if_fail (clay_item_is_drawable (CLAY_ITEM (drawable)), FALSE);

  width  = clay_drawable_get_width (drawable);
  height = clay_drawable_get_height (drawable);

  retval = clay_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  clay_drawable_get_offsets (drawable, &offset_x, &offset_y);

  *xmin = MAX3 (x1 - SELECTION_BORDER, 0, - offset_x);
  *ymin = MAX3 (y1 - SELECTION_BORDER, 0, - offset_y);
  *xmax = MIN3 (x2 + SELECTION_BORDER, width,  width  + offset_x);
  *ymax = MIN3 (y2 + SELECTION_BORDER, height, height + offset_y);

  return retval;
}


/**
 * clay_drawable_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #ClayDrawablePreview widget for @drawable.
 *
 * Returns: A pointer to the new #ClayDrawablePreview widget.
 *
 * Since: 2.10
 **/
GtkWidget *
clay_drawable_preview_new_from_drawable (ClayDrawable *drawable)
{
  g_return_val_if_fail (clay_item_is_valid (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (clay_item_is_drawable (CLAY_ITEM (drawable)), NULL);

  return g_object_new (CLAY_TYPE_DRAWABLE_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * clay_drawable_preview_get_drawable:
 * @preview:   a #ClayDrawablePreview widget
 *
 * Returns: (transfer none): the drawable that has been passed to
 *          clay_drawable_preview_new_from_drawable().
 *
 * Since: 2.10
 **/
ClayDrawable *
clay_drawable_preview_get_drawable (ClayDrawablePreview *preview)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_PREVIEW (preview), NULL);

  return preview->drawable;
}
