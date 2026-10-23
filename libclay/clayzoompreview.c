/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayzoompreview.c
 * Copyright (C) 2005  David Odin <dindinx@gimp.org>
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
#include "clayzoompreview.h"


/**
 * SECTION: clayzoompreview
 * @title: ClayZoomPreview
 * @short_description: A drawable preview with zooming capabilities.
 *
 * A drawable preview with zooming capabilities.
 **/


enum
{
  PROP_0,
  PROP_DRAWABLE,
  PROP_MODEL
};

typedef struct
{
  gboolean  update;
} PreviewSettings;


struct _ClayZoomPreview
{
  ClayScrolledPreview  parent_instance;

  ClayDrawable        *drawable;
  ClayZoomModel       *model;
  GdkRectangle         extents;
};


static void     clay_zoom_preview_constructed     (GObject         *object);
static void     clay_zoom_preview_finalize        (GObject         *object);
static void     clay_zoom_preview_dispose         (GObject         *object);
static void     clay_zoom_preview_get_property    (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
static void     clay_zoom_preview_set_property    (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);

static void     clay_zoom_preview_set_adjustments (ClayZoomPreview *preview,
                                                   gdouble          old_factor,
                                                   gdouble          new_factor);
static void     clay_zoom_preview_size_allocate   (GtkWidget       *widget,
                                                   GtkAllocation   *allocation,
                                                   ClayZoomPreview *preview);
static void     clay_zoom_preview_style_updated   (GtkWidget       *widget);
static gboolean clay_zoom_preview_scroll_event    (GtkWidget       *widget,
                                                   GdkEventScroll  *event,
                                                   ClayZoomPreview *preview);
static void     clay_zoom_preview_draw            (ClayPreview     *preview);
static void     clay_zoom_preview_draw_buffer     (ClayPreview     *preview,
                                                   const guchar    *buffer,
                                                   gint             rowstride);
static void     clay_zoom_preview_draw_thumb      (ClayPreview     *preview,
                                                   ClayPreviewArea *area,
                                                   gint             width,
                                                   gint             height);
static void     clay_zoom_preview_set_cursor      (ClayPreview     *preview);
static void     clay_zoom_preview_transform       (ClayPreview     *preview,
                                                   gint             src_x,
                                                   gint             src_y,
                                                   gint            *dest_x,
                                                   gint            *dest_y);
static void     clay_zoom_preview_untransform     (ClayPreview     *preview,
                                                   gint             src_x,
                                                   gint             src_y,
                                                   gint            *dest_x,
                                                   gint            *dest_y);

static void     clay_zoom_preview_set_drawable    (ClayZoomPreview *preview,
                                                   ClayDrawable    *drawable);
static void     clay_zoom_preview_set_model       (ClayZoomPreview *preview,
                                                   ClayZoomModel   *model);

static void     clay_zoom_preview_get_source_area (ClayPreview     *preview,
                                                   gint            *x,
                                                   gint            *y,
                                                   gint            *w,
                                                   gint            *h);


G_DEFINE_TYPE (ClayZoomPreview, clay_zoom_preview, CLAY_TYPE_SCROLLED_PREVIEW)

#define parent_class clay_zoom_preview_parent_class

static gint clay_zoom_preview_counter = 0;


static void
clay_zoom_preview_class_init (ClayZoomPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  ClayPreviewClass *preview_class = CLAY_PREVIEW_CLASS (klass);

  object_class->constructed   = clay_zoom_preview_constructed;
  object_class->finalize      = clay_zoom_preview_finalize;
  object_class->dispose       = clay_zoom_preview_dispose;
  object_class->get_property  = clay_zoom_preview_get_property;
  object_class->set_property  = clay_zoom_preview_set_property;

  widget_class->style_updated = clay_zoom_preview_style_updated;

  preview_class->draw         = clay_zoom_preview_draw;
  preview_class->draw_buffer  = clay_zoom_preview_draw_buffer;
  preview_class->draw_thumb   = clay_zoom_preview_draw_thumb;
  preview_class->set_cursor   = clay_zoom_preview_set_cursor;
  preview_class->transform    = clay_zoom_preview_transform;
  preview_class->untransform  = clay_zoom_preview_untransform;

  /**
   * ClayZoomPreview:drawable-id:
   *
   * The drawable the #ClayZoomPreview is attached to.
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

  /**
   * ClayZoomPreview:model:
   *
   * The #ClayZoomModel used by this #ClayZoomPreview.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Model",
                                                        "The zoom preview's ClayZoomModel",
                                                        CLAY_TYPE_ZOOM_MODEL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_zoom_preview_init (ClayZoomPreview *preview)
{
  GtkWidget *area = clay_preview_get_area (CLAY_PREVIEW (preview));

  g_signal_connect (area, "size-allocate",
                    G_CALLBACK (clay_zoom_preview_size_allocate),
                    preview);
  g_signal_connect (area, "scroll-event",
                    G_CALLBACK (clay_zoom_preview_scroll_event),
                    preview);

  g_object_set (area,
                "check-size", clay_check_size (),
                "check-type", clay_check_type (),
                "check-custom-color1", clay_check_custom_color1 (),
                "check-custom-color2", clay_check_custom_color2 (),
                NULL);

  clay_scrolled_preview_set_policy (CLAY_SCROLLED_PREVIEW (preview),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
}

static void
clay_zoom_preview_constructed (GObject *object)
{
  ClayZoomPreview *preview = CLAY_ZOOM_PREVIEW (object);
  gchar           *data_name;
  PreviewSettings  settings;
  GBytes          *settings_bytes = NULL;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-zoom-preview-%d",
                               g_get_prgname (),
                               clay_zoom_preview_counter++);

  if (clay_pdb_get_data (data_name, &settings_bytes) &&
      g_bytes_get_size (settings_bytes) == sizeof (PreviewSettings))
    {
      settings = *((PreviewSettings *) g_bytes_get_data (settings_bytes, NULL));
      clay_preview_set_update (CLAY_PREVIEW (object), settings.update);
    }
  g_bytes_unref (settings_bytes);

  g_object_set_data_full (object, "clay-zoom-preview-data-name",
                          data_name, (GDestroyNotify) g_free);

  if (! preview->model)
    {
      ClayZoomModel *model = clay_zoom_model_new ();

      clay_zoom_model_set_range (model, 1.0, 256.0);
      clay_zoom_preview_set_model (CLAY_ZOOM_PREVIEW (object), model);

      g_object_unref (model);
    }

  clay_zoom_preview_set_adjustments (CLAY_ZOOM_PREVIEW (object), 1.0, 1.0);
}

static void
clay_zoom_preview_finalize (GObject *object)
{
  ClayZoomPreview *preview = CLAY_ZOOM_PREVIEW (object);

  g_clear_object (&preview->model);
  g_clear_object (&preview->drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_zoom_preview_dispose (GObject *object)
{
  const gchar *data_name = g_object_get_data (G_OBJECT (object),
                                               "clay-zoom-preview-data-name");

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

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_zoom_preview_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayZoomPreview *preview = CLAY_ZOOM_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value, clay_zoom_preview_get_drawable (preview));
      break;

    case PROP_MODEL:
      g_value_set_object (value, clay_zoom_preview_get_model (preview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_zoom_preview_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayZoomPreview *preview = CLAY_ZOOM_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      clay_zoom_preview_set_drawable (preview, g_value_dup_object (value));
      break;

    case PROP_MODEL:
      clay_zoom_preview_set_model (preview, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_zoom_preview_set_adjustments (ClayZoomPreview *preview,
                                   gdouble          old_factor,
                                   gdouble          new_factor)
{
  ClayScrolledPreview *scrolled_preview = CLAY_SCROLLED_PREVIEW (preview);
  GtkAdjustment       *hadj;
  GtkAdjustment       *vadj;
  gint                 width;
  gint                 height;
  gdouble              ratio;

  clay_scrolled_preview_freeze (scrolled_preview);

  clay_preview_get_size (CLAY_PREVIEW (preview), &width, &height);

  ratio = new_factor / old_factor;

  clay_scrolled_preview_get_adjustments (scrolled_preview, &hadj, &vadj);

  gtk_adjustment_configure (hadj,
                            (gtk_adjustment_get_value (vadj) + width / 2.0) * ratio
                            - width / 2.0,
                            0,
                            width * new_factor,
                            new_factor,
                            MAX (width / 2.0, new_factor),
                            width);

  gtk_adjustment_configure (vadj,
                            (gtk_adjustment_get_value (vadj) + height / 2.0) * ratio
                            - height / 2.0,
                            0,
                            height * new_factor,
                            new_factor,
                            MAX (height / 2.0, new_factor),
                            height);

  clay_scrolled_preview_thaw (scrolled_preview);
}

static void
clay_zoom_preview_size_allocate (GtkWidget       *widget,
                                 GtkAllocation   *allocation,
                                 ClayZoomPreview *preview)
{
  gdouble zoom;

#if 0
  gint width  = CLAY_PREVIEW (preview)->xmax - CLAY_PREVIEW (preview)->xmin;
  gint height = CLAY_PREVIEW (preview)->ymax - CLAY_PREVIEW (preview)->ymin;

  CLAY_PREVIEW (preview)->width  = MIN (width,  allocation->width);
  CLAY_PREVIEW (preview)->height = MIN (height, allocation->height);
#endif

  zoom = clay_zoom_model_get_factor (preview->model);

  clay_zoom_preview_set_adjustments (preview, zoom, zoom);
}

static void
clay_zoom_preview_style_updated (GtkWidget *widget)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (widget);
  ClayPreview     *preview      = CLAY_PREVIEW (widget);
  GtkWidget       *area         = clay_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      gint size;
      gint width;
      gint height;
      gint preview_width;
      gint preview_height;
      gint x1, y1;
      gint x2, y2;

      gtk_widget_style_get (widget, "size", &size, NULL);

      if (_clay_drawable_preview_get_bounds (zoom_preview->drawable,
                                             &x1, &y1, &x2, &y2))
        {
          width  = x2 - x1;
          height = y2 - y1;
        }
      else
        {
          width  = clay_drawable_get_width  (zoom_preview->drawable);
          height = clay_drawable_get_height (zoom_preview->drawable);
        }

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

static gboolean
clay_zoom_preview_scroll_event (GtkWidget       *widget,
                                GdkEventScroll  *event,
                                ClayZoomPreview *preview)
{
  if (event->state & GDK_CONTROL_MASK)
    {
      gdouble delta;

      clay_scrolled_preview_freeze (CLAY_SCROLLED_PREVIEW (preview));

      switch (event->direction)
        {
        case GDK_SCROLL_UP:
          clay_zoom_model_zoom (preview->model, CLAY_ZOOM_IN, 0.0);
          break;

        case GDK_SCROLL_DOWN:
          clay_zoom_model_zoom (preview->model, CLAY_ZOOM_OUT, 0.0);
          break;

        case GDK_SCROLL_SMOOTH:
          gdk_event_get_scroll_deltas ((GdkEvent *) event, NULL, &delta);
          clay_zoom_model_zoom (preview->model, CLAY_ZOOM_SMOOTH, delta);
          break;

        default:
          break;
        }

      clay_scrolled_preview_thaw (CLAY_SCROLLED_PREVIEW (preview));
    }

  return FALSE;
}

static void
clay_zoom_preview_draw (ClayPreview *preview)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (preview);
  guchar          *data;
  gint             width;
  gint             height;
  gint             bpp;

  if (! zoom_preview->model)
    return;

  if (zoom_preview->drawable == NULL)
    return;

  data = clay_zoom_preview_get_source (CLAY_ZOOM_PREVIEW (preview),
                                       &width, &height, &bpp);

  if (data)
    {
      GtkWidget *area = clay_preview_get_area (preview);

      clay_preview_area_draw (CLAY_PREVIEW_AREA (area),
                              0, 0, width, height,
                              clay_drawable_type (zoom_preview->drawable),
                              data, width * bpp);
      g_free (data);
    }
}

static void
clay_zoom_preview_draw_buffer (ClayPreview  *preview,
                               const guchar *buffer,
                               gint          rowstride)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (preview);
  GtkWidget       *area         = clay_preview_get_area (preview);
  ClayImage       *image;
  gint             width;
  gint             height;

  clay_preview_get_size (preview, &width, &height);
  image = clay_item_get_image (CLAY_ITEM (zoom_preview->drawable));

  if (clay_selection_is_empty (image))
    {
      clay_preview_area_draw (CLAY_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              clay_drawable_type (zoom_preview->drawable),
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
      gint           src_x;
      gint           src_y;
      gint           src_width;
      gint           src_height;
      gint           offsx = 0;
      gint           offsy = 0;

      selection = clay_image_get_selection (image);

      clay_zoom_preview_get_source_area (preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      src = clay_drawable_get_sub_thumbnail_data (zoom_preview->drawable,
                                                  src_x, src_y,
                                                  src_width, src_height,
                                                  width, height,
                                                  &w, &h, &bpp);
      clay_drawable_get_offsets (zoom_preview->drawable, &offsx, &offsy);
      sel = clay_drawable_get_sub_thumbnail_data (CLAY_DRAWABLE (selection),
                                                  src_x + offsx, src_y + offsy,
                                                  src_width, src_height,
                                                  width, height,
                                                  &w, &h, &bpp);

      clay_preview_area_mask (CLAY_PREVIEW_AREA (area),
                              0, 0, width, height,
                              clay_drawable_type (zoom_preview->drawable),
                              g_bytes_get_data (src, NULL), width * clay_drawable_get_bpp (zoom_preview->drawable),
                              buffer, rowstride,
                              g_bytes_get_data (sel, NULL), width);

      g_bytes_unref (sel);
      g_bytes_unref (src);
    }
}

static void
clay_zoom_preview_draw_thumb (ClayPreview     *preview,
                              ClayPreviewArea *area,
                              gint             width,
                              gint             height)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (preview);

  if (zoom_preview->drawable != NULL)
    _clay_drawable_preview_area_draw_thumb (area, zoom_preview->drawable,
                                            width, height);
}

static void
clay_zoom_preview_set_cursor (ClayPreview *preview)
{
  GtkWidget *area = clay_preview_get_area (preview);

  if (! gtk_widget_get_realized (area))
    return;

  if (clay_zoom_preview_get_factor (CLAY_ZOOM_PREVIEW (preview)) > 1.0)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (preview));
      GdkCursor  *cursor;

      cursor = gdk_cursor_new_for_display (display, GDK_HAND1);
      gdk_window_set_cursor (gtk_widget_get_window (area),
                             cursor);
      g_object_unref (cursor);
    }
  else
    {
      gdk_window_set_cursor (gtk_widget_get_window (area),
                             clay_preview_get_default_cursor (preview));
    }
}

static void
clay_zoom_preview_transform (ClayPreview *preview,
                             gint         src_x,
                             gint         src_y,
                             gint        *dest_x,
                             gint        *dest_y)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (preview);
  gint             width;
  gint             height;
  gint             xoff;
  gint             yoff;
  gdouble          zoom;

  clay_preview_get_size (preview, &width, &height);
  clay_preview_get_offsets (preview, &xoff, &yoff);

  zoom = clay_zoom_preview_get_factor (CLAY_ZOOM_PREVIEW (preview));

  *dest_x = ((gdouble) (src_x - zoom_preview->extents.x) *
             width / zoom_preview->extents.width * zoom) - xoff;

  *dest_y = ((gdouble) (src_y - zoom_preview->extents.y) *
             height / zoom_preview->extents.height * zoom) - yoff;
}

static void
clay_zoom_preview_untransform (ClayPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (preview);
  gint             width;
  gint             height;
  gint             xoff;
  gint             yoff;
  gdouble          zoom;

  clay_preview_get_size (preview, &width, &height);
  clay_preview_get_offsets (preview, &xoff, &yoff);

  zoom = clay_zoom_preview_get_factor (CLAY_ZOOM_PREVIEW (preview));

  *dest_x = (zoom_preview->extents.x +
             ((gdouble) (src_x + xoff) *
              zoom_preview->extents.width / width / zoom));

  *dest_y = (zoom_preview->extents.y +
             ((gdouble) (src_y + yoff) *
              zoom_preview->extents.height / height / zoom));
}

static void
clay_zoom_preview_set_drawable (ClayZoomPreview *preview,
                                ClayDrawable    *drawable)
{
  gint x, y;
  gint width, height;
  gint max_width, max_height;

  g_return_if_fail (preview->drawable == NULL);

  preview->drawable = drawable;

  if (clay_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      preview->extents.x = x;
      preview->extents.y = y;
    }
  else
    {
      width  = clay_drawable_get_width  (drawable);
      height = clay_drawable_get_height (drawable);

      preview->extents.x = 0;
      preview->extents.y = 0;
    }

  preview->extents.width  = width;
  preview->extents.height = height;

  if (width > height)
    {
      max_width  = MIN (width, 512);
      max_height = (height * max_width) / width;
    }
  else
    {
      max_height = MIN (height, 512);
      max_width  = (width * max_height) / height;
    }

  clay_preview_set_bounds (CLAY_PREVIEW (preview),
                           0, 0, max_width, max_height);

  g_object_set (clay_preview_get_frame (CLAY_PREVIEW (preview)),
                "ratio", (gdouble) width / (gdouble) height,
                NULL);
}

static void
clay_zoom_preview_set_model (ClayZoomPreview *preview,
                             ClayZoomModel   *model)
{
  GtkWidget *button_bar;
  GtkWidget *button;
  GtkWidget *box;

  g_return_if_fail (preview->model == NULL);

  if (! model)
    return;

  preview->model = g_object_ref (model);

  g_signal_connect_swapped (preview->model, "zoomed",
                            G_CALLBACK (clay_zoom_preview_set_adjustments),
                            preview);

  box = clay_preview_get_controls (CLAY_PREVIEW (preview));
  g_return_if_fail (GTK_IS_BOX (box));

  button_bar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_end (GTK_BOX (box), button_bar, FALSE, FALSE, 0);
  gtk_widget_set_visible (button_bar, TRUE);

  /* zoom out */
  button = clay_zoom_button_new (preview->model,
                                 CLAY_ZOOM_OUT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  /* zoom in */
  button = clay_zoom_button_new (preview->model,
                                 CLAY_ZOOM_IN, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (button_bar), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
}

static void
clay_zoom_preview_get_source_area (ClayPreview *preview,
                                   gint        *x,
                                   gint        *y,
                                   gint        *w,
                                   gint        *h)
{
  ClayZoomPreview *zoom_preview = CLAY_ZOOM_PREVIEW (preview);
  gdouble          zoom         = clay_zoom_model_get_factor (zoom_preview->model);

  clay_zoom_preview_untransform (preview, 0, 0, x, y);

  *w = zoom_preview->extents.width / zoom;
  *h = zoom_preview->extents.height / zoom;
}


/**
 * clay_zoom_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #ClayZoomPreview widget for @drawable.
 *
 * Since: 3.0
 *
 * Returns: a new #ClayZoomPreview.
 **/
GtkWidget *
clay_zoom_preview_new_from_drawable (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_valid (CLAY_ITEM (drawable)), NULL);

  return g_object_new (CLAY_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * clay_zoom_preview_new_with_model_from_drawable:
 * @drawable: a #ClayDrawable
 * @model:    a #ClayZoomModel
 *
 * Creates a new #ClayZoomPreview widget for @drawable using the
 * given @model.
 *
 * This variant of clay_zoom_preview_new_from_drawable() allows you
 * to create a preview using an existing zoom model. This may be
 * useful if for example you want to have two zoom previews that keep
 * their zoom factor in sync.
 *
 * Since: 2.10
 *
 * Returns: a new #ClayZoomPreview.
 **/
GtkWidget *
clay_zoom_preview_new_with_model_from_drawable (ClayDrawable  *drawable,
                                                ClayZoomModel *model)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_valid (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (CLAY_IS_ZOOM_MODEL (model), NULL);

  return g_object_new (CLAY_TYPE_ZOOM_PREVIEW,
                       "drawable", drawable,
                       "model",    model,
                       NULL);
}

/**
 * clay_zoom_preview_get_drawable:
 * @preview: a #ClayZoomPreview widget
 *
 * Returns the drawable the #ClayZoomPreview is attached to.
 *
 * Returns: (transfer none): the drawable that was passed to
 *          clay_zoom_preview_new_from_drawable().
 *
 * Since: 3.0
 **/
ClayDrawable *
clay_zoom_preview_get_drawable (ClayZoomPreview *preview)
{
  g_return_val_if_fail (CLAY_IS_ZOOM_PREVIEW (preview), NULL);

  return preview->drawable;
}

/**
 * clay_zoom_preview_get_model:
 * @preview: a #ClayZoomPreview widget
 *
 * Returns the #ClayZoomModel the preview is using.
 *
 * Returns: (transfer none): a pointer to the #ClayZoomModel owned
 *          by the @preview
 *
 * Since: 2.4
 **/
ClayZoomModel *
clay_zoom_preview_get_model (ClayZoomPreview *preview)
{
  g_return_val_if_fail (CLAY_IS_ZOOM_PREVIEW (preview), NULL);

  return preview->model;
}

/**
 * clay_zoom_preview_get_factor:
 * @preview: a #ClayZoomPreview widget
 *
 * Returns the zoom factor the preview is currently using.
 *
 * Returns: the current zoom factor
 *
 * Since: 2.4
 **/
gdouble
clay_zoom_preview_get_factor (ClayZoomPreview *preview)
{
  g_return_val_if_fail (CLAY_IS_ZOOM_PREVIEW (preview), 1.0);

  return preview->model ? clay_zoom_model_get_factor (preview->model) : 1.0;
}

/**
 * clay_zoom_preview_get_source:
 * @preview: a #ClayZoomPreview widget
 * @width: (out): a pointer to an int where the current width of the zoom widget
 *         will be put.
 * @height: (out): a pointer to an int where the current width of the zoom widget
 *          will be put.
 * @bpp: (out): return location for the number of bytes per pixel
 *
 * Returns the scaled image data of the part of the drawable the
 * #ClayZoomPreview is currently showing, as a newly allocated array of guchar.
 * This function also allow to get the current width, height and bpp of the
 * #ClayZoomPreview.
 *
 * Returns: (transfer full) (array): newly allocated data that should be
 *          released using g_free() when it is not any longer needed
 *
 * Since: 2.4
 */
guchar *
clay_zoom_preview_get_source (ClayZoomPreview *preview,
                              gint            *width,
                              gint            *height,
                              gint            *bpp)
{
  ClayDrawable *drawable;

  g_return_val_if_fail (CLAY_IS_ZOOM_PREVIEW (preview), NULL);
  g_return_val_if_fail (width != NULL && height != NULL && bpp != NULL, NULL);

  drawable = clay_zoom_preview_get_drawable (preview);

  if (drawable)
    {
      ClayPreview *clay_preview = CLAY_PREVIEW (preview);
      gint         src_x;
      gint         src_y;
      gint         src_width;
      gint         src_height;
      GBytes      *src_bytes;
      gsize        src_bytes_size;

      clay_preview_get_size (clay_preview, width, height);

      clay_zoom_preview_get_source_area (clay_preview,
                                         &src_x, &src_y,
                                         &src_width, &src_height);

      src_bytes =  clay_drawable_get_sub_thumbnail_data (drawable,
                                                         src_x, src_y,
                                                         src_width, src_height,
                                                         *width, *height,
                                                         width, height, bpp);
      return g_bytes_unref_to_data (src_bytes, &src_bytes_size);
    }
  else
    {
      *width  = 0;
      *height = 0;
      *bpp    = 0;

      return NULL;
    }
}
