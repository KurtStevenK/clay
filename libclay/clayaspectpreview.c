/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayaspectpreview.c
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

#include "libclay-intl.h"

#include "clayaspectpreview.h"


/**
 * SECTION: clayaspectpreview
 * @title: ClayAspectPreview
 * @short_description: A widget providing a preview with fixed aspect ratio.
 *
 * A widget providing a preview with fixed aspect ratio.
 **/


enum
{
  PROP_0,
  PROP_DRAWABLE
};

struct _ClayAspectPreview
{
  ClayPreview   parent_instance;

  ClayDrawable *drawable;
};

typedef struct
{
  gboolean  update;
} PreviewSettings;


static void  clay_aspect_preview_constructed   (GObject         *object);
static void  clay_aspect_preview_dispose       (GObject         *object);
static void  clay_aspect_preview_get_property  (GObject         *object,
                                                guint            property_id,
                                                GValue          *value,
                                                GParamSpec      *pspec);
static void  clay_aspect_preview_set_property  (GObject         *object,
                                                guint            property_id,
                                                const GValue    *value,
                                                GParamSpec      *pspec);

static void  clay_aspect_preview_style_updated (GtkWidget       *widget);
static void  clay_aspect_preview_draw          (ClayPreview     *preview);
static void  clay_aspect_preview_draw_buffer   (ClayPreview     *preview,
                                                const guchar    *buffer,
                                                gint             rowstride);
static void  clay_aspect_preview_transform     (ClayPreview     *preview,
                                                gint             src_x,
                                                gint             src_y,
                                                gint            *dest_x,
                                                gint            *dest_y);
static void  clay_aspect_preview_untransform   (ClayPreview     *preview,
                                                gint             src_x,
                                                gint             src_y,
                                                gint            *dest_x,
                                                gint            *dest_y);

static void  clay_aspect_preview_set_drawable  (ClayAspectPreview *preview,
                                                ClayDrawable      *drawable);


G_DEFINE_TYPE (ClayAspectPreview, clay_aspect_preview, CLAY_TYPE_PREVIEW)

#define parent_class clay_aspect_preview_parent_class

static gint clay_aspect_preview_counter = 0;


static void
clay_aspect_preview_class_init (ClayAspectPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  ClayPreviewClass *preview_class = CLAY_PREVIEW_CLASS (klass);

  object_class->constructed   = clay_aspect_preview_constructed;
  object_class->dispose       = clay_aspect_preview_dispose;
  object_class->get_property  = clay_aspect_preview_get_property;
  object_class->set_property  = clay_aspect_preview_set_property;

  widget_class->style_updated = clay_aspect_preview_style_updated;

  preview_class->draw         = clay_aspect_preview_draw;
  preview_class->draw_buffer  = clay_aspect_preview_draw_buffer;
  preview_class->transform    = clay_aspect_preview_transform;
  preview_class->untransform  = clay_aspect_preview_untransform;

  /**
   * ClayAspectPreview:drawable-id:
   *
   * The drawable the #ClayAspectPreview is attached to.
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
clay_aspect_preview_init (ClayAspectPreview *preview)
{
  g_object_set (clay_preview_get_area (CLAY_PREVIEW (preview)),
                "check-size", clay_check_size (),
                "check-type", clay_check_type (),
                "check-custom-color1", clay_check_custom_color1 (),
                "check-custom-color2", clay_check_custom_color2 (),
                NULL);
}

static void
clay_aspect_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;
  GBytes          *settings_bytes = NULL;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-aspect-preview-%d",
                               g_get_prgname (),
                               clay_aspect_preview_counter++);

  if (clay_pdb_get_data (data_name, &settings_bytes) &&
      g_bytes_get_size (settings_bytes) == sizeof (PreviewSettings))
    {
      settings = *((PreviewSettings *) g_bytes_get_data (settings_bytes, NULL));
      clay_preview_set_update (CLAY_PREVIEW (object), settings.update);
    }
  g_bytes_unref (settings_bytes);

  g_object_set_data_full (object, "clay-aspect-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
clay_aspect_preview_dispose (GObject *object)
{
  ClayAspectPreview *preview = CLAY_ASPECT_PREVIEW (object);
  const gchar       *data_name;

  data_name = g_object_get_data (G_OBJECT (object),
                                 "clay-aspect-preview-data-name");
  if (data_name)
    {
      ClayPreview     *preview = CLAY_PREVIEW (object);
      GBytes          *bytes;
      PreviewSettings  settings;

      settings.update = clay_preview_get_update (preview);

      bytes = g_bytes_new_static (&settings, sizeof (PreviewSettings));
      clay_pdb_set_data (data_name, bytes);
      g_bytes_unref (bytes);
    }

  g_clear_object (&preview->drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_aspect_preview_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayAspectPreview *preview = CLAY_ASPECT_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value, preview->drawable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_aspect_preview_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayAspectPreview *preview = CLAY_ASPECT_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      clay_aspect_preview_set_drawable (preview, g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_aspect_preview_style_updated (GtkWidget *widget)
{
  ClayPreview       *preview        = CLAY_PREVIEW (widget);
  ClayAspectPreview *aspect_preview = CLAY_ASPECT_PREVIEW (preview);
  GtkWidget         *area           = clay_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      gint width;
      gint height;
      gint preview_width;
      gint preview_height;
      gint size;

      width  = clay_drawable_get_width  (aspect_preview->drawable);
      height = clay_drawable_get_height (aspect_preview->drawable);

      gtk_widget_style_get (widget,
                            "size", &size,
                            NULL);

      if (width > height)
        {
          preview_width  = MIN (width, size);
          preview_height = (height * preview_width) / width;
        }
      else
        {
          preview_height = MIN (height, size);
          preview_width  = (width * preview_height) / height;
        }

      clay_preview_set_size (preview, preview_width, preview_height);
    }
}

static void
clay_aspect_preview_draw (ClayPreview *preview)
{
  GtkWidget *area = clay_preview_get_area (preview);
  gint       width;
  gint       height;

  clay_preview_get_size (preview, &width, &height);

  clay_preview_area_fill (CLAY_PREVIEW_AREA (area),
                          0, 0,
                          width, height,
                          0, 0, 0);
}

static void
clay_aspect_preview_draw_buffer (ClayPreview  *preview,
                                 const guchar *buffer,
                                 gint          rowstride)
{
  ClayAspectPreview *aspect_preview = CLAY_ASPECT_PREVIEW (preview);
  GtkWidget         *area           = clay_preview_get_area (preview);
  ClayImage         *image;
  gint               width;
  gint               height;

  clay_preview_get_size (preview, &width, &height);

  image = clay_item_get_image (CLAY_ITEM (aspect_preview->drawable));

  if (clay_selection_is_empty (image))
    {
      clay_preview_area_draw (CLAY_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              clay_drawable_type (aspect_preview->drawable),
                              buffer,
                              rowstride);
    }
  else
    {
      GBytes        *sel;
      GBytes        *src;
      ClaySelection *selection;
      gint           w, h;
      gint           bpp;

      selection = clay_image_get_selection (image);

      src = clay_drawable_get_thumbnail_data (aspect_preview->drawable,
                                              width, height,
                                              &w, &h, &bpp);
      sel = clay_drawable_get_thumbnail_data (CLAY_DRAWABLE (selection),
                                              width, height,
                                              &w, &h, &bpp);

      clay_preview_area_mask (CLAY_PREVIEW_AREA (area),
                              0, 0, width, height,
                              clay_drawable_type (aspect_preview->drawable),
                              g_bytes_get_data (src, NULL),
                              width * clay_drawable_get_bpp (aspect_preview->drawable),
                              buffer, rowstride,
                              g_bytes_get_data (sel, NULL), width);

      g_bytes_unref (sel);
      g_bytes_unref (src);
    }
}

static void
clay_aspect_preview_transform (ClayPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  ClayAspectPreview *aspect_preview = CLAY_ASPECT_PREVIEW (preview);
  gint               width;
  gint               height;

  clay_preview_get_size (preview, &width, &height);

  *dest_x = (gdouble) src_x * width  / clay_drawable_get_width  (aspect_preview->drawable);
  *dest_y = (gdouble) src_y * height / clay_drawable_get_height (aspect_preview->drawable);
}

static void
clay_aspect_preview_untransform (ClayPreview *preview,
                                 gint         src_x,
                                 gint         src_y,
                                 gint        *dest_x,
                                 gint        *dest_y)
{
  ClayAspectPreview *aspect_preview = CLAY_ASPECT_PREVIEW (preview);
  gint               width;
  gint               height;

  clay_preview_get_size (preview, &width, &height);

  *dest_x = (gdouble) src_x * clay_drawable_get_width  (aspect_preview->drawable) / width;
  *dest_y = (gdouble) src_y * clay_drawable_get_height (aspect_preview->drawable) / height;
}

static void
clay_aspect_preview_set_drawable (ClayAspectPreview *preview,
                                  ClayDrawable      *drawable)
{
  gint d_width;
  gint d_height;
  gint width;
  gint height;

  g_return_if_fail (preview->drawable == NULL);

  preview->drawable = drawable;

  d_width  = clay_drawable_get_width  (preview->drawable);
  d_height = clay_drawable_get_height (preview->drawable);

  if (d_width > d_height)
    {
      width  = MIN (d_width, 512);
      height = (d_height * width) / d_width;
    }
  else
    {
      height = MIN (d_height, 512);
      width  = (d_width * height) / d_height;
    }

  clay_preview_set_bounds (CLAY_PREVIEW (preview), 0, 0, width, height);

  if (height > 0)
    g_object_set (clay_preview_get_frame (CLAY_PREVIEW (preview)),
                  "ratio",
                  (gdouble) d_width / (gdouble) d_height,
                  NULL);
}

/**
 * clay_aspect_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #ClayAspectPreview widget for @drawable_. See also
 * clay_drawable_preview_new_from_drawable().
 *
 * Since: 2.10
 *
 * Returns: a new #ClayAspectPreview.
 **/
GtkWidget *
clay_aspect_preview_new_from_drawable (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_valid (CLAY_ITEM (drawable)), NULL);

  return g_object_new (CLAY_TYPE_ASPECT_PREVIEW,
                       "drawable", drawable,
                       NULL);
}
