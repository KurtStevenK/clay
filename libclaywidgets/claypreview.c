/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypreview.c
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

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "claywidgets.h"

#include "claypreview.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claypreview
 * @title: ClayPreview
 * @short_description: A widget providing a #ClayPreviewArea plus
 *                     framework to update the preview.
 *
 * A widget providing a #ClayPreviewArea plus framework to update the
 * preview.
 **/


#define DEFAULT_SIZE     200
#define PREVIEW_TIMEOUT  200


enum
{
  INVALIDATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_UPDATE
};


typedef struct _ClayPreviewPrivate
{
  GtkWidget *area;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *controls;
  GdkCursor *cursor_busy;
  GdkCursor *default_cursor;

  gint       xoff, yoff;
  gint       xmin, xmax, ymin, ymax;
  gint       width, height;

  gboolean   update_preview;
  guint      timeout_id;
} ClayPreviewPrivate;


static void      clay_preview_dispose             (GObject          *object);
static void      clay_preview_get_property        (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);
static void      clay_preview_set_property        (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);

static void      clay_preview_direction_changed   (GtkWidget        *widget,
                                                   GtkTextDirection  prev_dir);
static gboolean  clay_preview_popup_menu          (GtkWidget        *widget);

static void      clay_preview_area_realize        (GtkWidget        *widget,
                                                   ClayPreview      *preview);
static void      clay_preview_area_unrealize      (GtkWidget        *widget,
                                                   ClayPreview      *preview);
static void      clay_preview_area_size_allocate  (GtkWidget        *widget,
                                                   GtkAllocation    *allocation,
                                                   ClayPreview      *preview);
static void      clay_preview_area_set_cursor     (ClayPreview      *preview);
static gboolean  clay_preview_area_event          (GtkWidget        *area,
                                                   GdkEvent         *event,
                                                   ClayPreview      *preview);

static void      clay_preview_toggle_callback     (GtkWidget        *toggle,
                                                   ClayPreview      *preview);

static void      clay_preview_notify_checks       (ClayPreview      *preview);

static gboolean  clay_preview_invalidate_now      (ClayPreview      *preview);
static void      clay_preview_real_set_cursor     (ClayPreview      *preview);
static void      clay_preview_real_transform      (ClayPreview      *preview,
                                                   gint              src_x,
                                                   gint              src_y,
                                                   gint             *dest_x,
                                                   gint             *dest_y);
static void      clay_preview_real_untransform    (ClayPreview      *preview,
                                                   gint              src_x,
                                                   gint              src_y,
                                                   gint             *dest_x,
                                                   gint             *dest_y);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayPreview, clay_preview, GTK_TYPE_BOX)

#define parent_class clay_preview_parent_class

static guint preview_signals[LAST_SIGNAL] = { 0 };


static void
clay_preview_class_init (ClayPreviewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[INVALIDATED] =
    g_signal_new ("invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPreviewClass, invalidated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose           = clay_preview_dispose;
  object_class->get_property      = clay_preview_get_property;
  object_class->set_property      = clay_preview_set_property;

  widget_class->direction_changed = clay_preview_direction_changed;
  widget_class->popup_menu        = clay_preview_popup_menu;

  klass->draw                     = NULL;
  klass->draw_thumb               = NULL;
  klass->draw_buffer              = NULL;
  klass->set_cursor               = clay_preview_real_set_cursor;
  klass->transform                = clay_preview_real_transform;
  klass->untransform              = clay_preview_real_untransform;

  g_object_class_install_property (object_class,
                                   PROP_UPDATE,
                                   g_param_spec_boolean ("update",
                                                         "Update",
                                                         "Whether the preview should update automatically",
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("size",
                                                             "Size",
                                                             "The preview's size",
                                                             1, 1024,
                                                             DEFAULT_SIZE,
                                                             CLAY_PARAM_READABLE));
}

static void
clay_preview_init (ClayPreview *preview)
{
  ClayPreviewPrivate *priv;
  GtkWidget          *frame;
  gdouble             xalign = 0.0;

  priv = clay_preview_get_instance_private (preview);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (preview),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_homogeneous (GTK_BOX (preview), FALSE);
  gtk_box_set_spacing (GTK_BOX (preview), 6);

  if (gtk_widget_get_direction (GTK_WIDGET (preview)) == GTK_TEXT_DIR_RTL)
    xalign = 1.0;

  priv->frame = gtk_aspect_frame_new (NULL, xalign, 0.0, 1.0, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (priv->frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (preview), priv->frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (priv->frame, TRUE);

  priv->grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (priv->frame), priv->grid);
  gtk_widget_set_visible (priv->grid, TRUE);

  priv->timeout_id = 0;

  priv->xmin   = priv->ymin = 0;
  priv->xmax   = priv->ymax = 1;
  priv->width  = priv->xmax - priv->xmin;
  priv->height = priv->ymax - priv->ymin;

  priv->xoff   = 0;
  priv->yoff   = 0;

  priv->default_cursor = NULL;

  /*  preview area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_widget_set_vexpand (frame, TRUE);
  gtk_grid_attach (GTK_GRID (priv->grid), frame, 0, 0, 1, 1);
  gtk_widget_set_visible (frame, TRUE);

  priv->area = clay_preview_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), priv->area);
  gtk_widget_set_visible (priv->area, TRUE);

  g_signal_connect_swapped (priv->area, "notify::check-size",
                            G_CALLBACK (clay_preview_notify_checks),
                            preview);
  g_signal_connect_swapped (priv->area, "notify::check-type",
                            G_CALLBACK (clay_preview_notify_checks),
                            preview);
  g_signal_connect_swapped (priv->area, "notify::check-custom-color1",
                            G_CALLBACK (clay_preview_notify_checks),
                            preview);
  g_signal_connect_swapped (priv->area, "notify::check-custom-color2",
                            G_CALLBACK (clay_preview_notify_checks),
                            preview);

  gtk_widget_add_events (priv->area,
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_BUTTON_RELEASE_MASK      |
                         GDK_SCROLL_MASK              |
                         GDK_SMOOTH_SCROLL_MASK       |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_MOTION_MASK);

  g_signal_connect (priv->area, "event",
                    G_CALLBACK (clay_preview_area_event),
                    preview);

  g_signal_connect (priv->area, "realize",
                    G_CALLBACK (clay_preview_area_realize),
                    preview);
  g_signal_connect (priv->area, "unrealize",
                    G_CALLBACK (clay_preview_area_unrealize),
                    preview);

  g_signal_connect_data (priv->area, "realize",
                         G_CALLBACK (clay_preview_area_set_cursor),
                         preview, NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  g_signal_connect (priv->area, "size-allocate",
                    G_CALLBACK (clay_preview_area_size_allocate),
                    preview);

  g_signal_connect_data (priv->area, "size-allocate",
                         G_CALLBACK (clay_preview_area_set_cursor),
                         preview, NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  priv->controls = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_top (priv->controls, 3);
  gtk_grid_attach (GTK_GRID (priv->grid), priv->controls, 0, 2, 2, 1);
  gtk_widget_set_visible (priv->controls, TRUE);

  /*  toggle button to (de)activate the instant preview  */
  priv->toggle = gtk_check_button_new_with_mnemonic (_("_Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle),
                                priv->update_preview);
  gtk_box_pack_start (GTK_BOX (priv->controls), priv->toggle, TRUE, TRUE, 0);
  gtk_widget_set_visible (priv->toggle, TRUE);

  g_signal_connect (priv->toggle, "toggled",
                    G_CALLBACK (clay_preview_toggle_callback),
                    preview);
}

static void
clay_preview_dispose (GObject *object)
{
  ClayPreview        *preview = CLAY_PREVIEW (object);
  ClayPreviewPrivate *priv    = clay_preview_get_instance_private (preview);

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_preview_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClayPreview        *preview = CLAY_PREVIEW (object);
  ClayPreviewPrivate *priv    = clay_preview_get_instance_private (preview);

  switch (property_id)
    {
    case PROP_UPDATE:
      g_value_set_boolean (value, priv->update_preview);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_preview_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClayPreview        *preview = CLAY_PREVIEW (object);
  ClayPreviewPrivate *priv    = clay_preview_get_instance_private (preview);

  switch (property_id)
    {
    case PROP_UPDATE:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle),
                                    g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_preview_direction_changed (GtkWidget        *widget,
                                GtkTextDirection  prev_dir)
{
  ClayPreview        *preview = CLAY_PREVIEW (widget);
  ClayPreviewPrivate *priv    = clay_preview_get_instance_private (preview);
  gdouble             xalign  = 0.0;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    xalign = 1.0;

  gtk_aspect_frame_set (GTK_ASPECT_FRAME (priv->frame),
                        xalign, 0.0, 1.0, TRUE);
}

static gboolean
clay_preview_popup_menu (GtkWidget *widget)
{
  ClayPreview        *preview = CLAY_PREVIEW (widget);
  ClayPreviewPrivate *priv    = clay_preview_get_instance_private (preview);

  clay_preview_area_menu_popup (CLAY_PREVIEW_AREA (priv->area), NULL);

  return TRUE;
}

static void
clay_preview_area_realize (GtkWidget   *widget,
                           ClayPreview *preview)
{
  ClayPreviewPrivate *priv    = clay_preview_get_instance_private (preview);
  GdkDisplay         *display = gtk_widget_get_display (widget);

  g_return_if_fail (priv->cursor_busy == NULL);

  priv->cursor_busy = gdk_cursor_new_for_display (display, GDK_WATCH);

}

static void
clay_preview_area_unrealize (GtkWidget   *widget,
                             ClayPreview *preview)
{
  ClayPreviewPrivate *priv = clay_preview_get_instance_private (preview);

  g_clear_object (&priv->cursor_busy);
}

static void
clay_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation,
                                 ClayPreview   *preview)
{
  ClayPreviewPrivate *priv   = clay_preview_get_instance_private (preview);
  gint                width  = priv->xmax - priv->xmin;
  gint                height = priv->ymax - priv->ymin;

  priv->width  = MIN (width,  allocation->width);
  priv->height = MIN (height, allocation->height);

  clay_preview_draw (preview);
  clay_preview_invalidate (preview);
}

static void
clay_preview_area_set_cursor (ClayPreview *preview)
{
  CLAY_PREVIEW_GET_CLASS (preview)->set_cursor (preview);
}

static gboolean
clay_preview_area_event (GtkWidget   *area,
                         GdkEvent    *event,
                         ClayPreview *preview)
{
  GdkEventButton *button_event = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (button_event->button)
        {
        case 3:
          clay_preview_area_menu_popup (CLAY_PREVIEW_AREA (area), button_event);
          return TRUE;
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
clay_preview_toggle_callback (GtkWidget   *toggle,
                              ClayPreview *preview)
{
  ClayPreviewPrivate *priv = clay_preview_get_instance_private (preview);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      priv->update_preview = TRUE;

      g_object_notify (G_OBJECT (preview), "update");

      if (priv->timeout_id)
        g_source_remove (priv->timeout_id);

      clay_preview_invalidate_now (preview);
    }
  else
    {
      priv->update_preview = FALSE;

      g_object_notify (G_OBJECT (preview), "update");

      clay_preview_draw (preview);
    }
}

static void
clay_preview_notify_checks (ClayPreview *preview)
{
  clay_preview_draw (preview);
  clay_preview_invalidate (preview);
}

static gboolean
clay_preview_invalidate_now (ClayPreview *preview)
{
  ClayPreviewPrivate *priv     = clay_preview_get_instance_private (preview);
  GtkWidget          *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (preview));
  ClayPreviewClass   *class    = CLAY_PREVIEW_GET_CLASS (preview);

  clay_preview_draw (preview);

  priv->timeout_id = 0;

  if (toplevel && gtk_widget_get_realized (toplevel))
    {
      gdk_window_set_cursor (gtk_widget_get_window (toplevel),
                             priv->cursor_busy);
      gdk_window_set_cursor (gtk_widget_get_window (priv->area),
                             priv->cursor_busy);

      gdk_display_flush (gtk_widget_get_display (toplevel));

      g_signal_emit (preview, preview_signals[INVALIDATED], 0);

      class->set_cursor (preview);
      gdk_window_set_cursor (gtk_widget_get_window (toplevel), NULL);
    }
  else
    {
      g_signal_emit (preview, preview_signals[INVALIDATED], 0);
    }

  return FALSE;
}

static void
clay_preview_real_set_cursor (ClayPreview *preview)
{
  ClayPreviewPrivate *priv = clay_preview_get_instance_private (preview);

  if (gtk_widget_get_realized (priv->area))
    gdk_window_set_cursor (gtk_widget_get_window (priv->area),
                           priv->default_cursor);
}

static void
clay_preview_real_transform (ClayPreview *preview,
                             gint         src_x,
                             gint         src_y,
                             gint        *dest_x,
                             gint        *dest_y)
{
  ClayPreviewPrivate *priv = clay_preview_get_instance_private (preview);

  *dest_x = src_x - priv->xoff - priv->xmin;
  *dest_y = src_y - priv->yoff - priv->ymin;
}

static void
clay_preview_real_untransform (ClayPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  ClayPreviewPrivate *priv = clay_preview_get_instance_private (preview);

  *dest_x = src_x + priv->xoff + priv->xmin;
  *dest_y = src_y + priv->yoff + priv->ymin;
}

/**
 * clay_preview_set_update:
 * @preview: a #ClayPreview widget
 * @update: %TRUE if the preview should invalidate itself when being
 *          scrolled or when clay_preview_invalidate() is being called
 *
 * Sets the state of the "Preview" check button.
 *
 * Since: 2.2
 **/
void
clay_preview_set_update (ClayPreview *preview,
                         gboolean     update)
{
  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  g_object_set (preview,
                "update", update,
                NULL);
}

/**
 * clay_preview_get_update:
 * @preview: a #ClayPreview widget
 *
 * Returns: the state of the "Preview" check button.
 *
 * Since: 2.2
 **/
gboolean
clay_preview_get_update (ClayPreview *preview)
{
  ClayPreviewPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PREVIEW (preview), FALSE);

  priv = clay_preview_get_instance_private (preview);

  return priv->update_preview;
}

/**
 * clay_preview_set_bounds:
 * @preview: a #ClayPreview widget
 * @xmin:    the minimum X value
 * @ymin:    the minimum Y value
 * @xmax:    the maximum X value
 * @ymax:    the maximum Y value
 *
 * Sets the lower and upper limits for the previewed area. The
 * difference between the upper and lower value is used to set the
 * maximum size of the #ClayPreviewArea used in the @preview.
 *
 * Since: 2.2
 **/
void
clay_preview_set_bounds (ClayPreview *preview,
                         gint         xmin,
                         gint         ymin,
                         gint         xmax,
                         gint         ymax)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));
  g_return_if_fail (xmax > xmin);
  g_return_if_fail (ymax > ymin);

  priv = clay_preview_get_instance_private (preview);

  priv->xmin = xmin;
  priv->ymin = ymin;
  priv->xmax = xmax;
  priv->ymax = ymax;

  clay_preview_area_set_max_size (CLAY_PREVIEW_AREA (priv->area),
                                  xmax - xmin,
                                  ymax - ymin);
}

/**
 * clay_preview_get_bounds:
 * @preview: A #ClayPreview:
 * @xmin: (out) (optional): Lower bound for x
 * @ymin: (out) (optional): Lower bound for y
 * @xmax: (out) (optional): Higher bound for x
 * @ymax: (out) (optional): Higher bound for y
 */
void
clay_preview_get_bounds (ClayPreview *preview,
                         gint        *xmin,
                         gint        *ymin,
                         gint        *xmax,
                         gint        *ymax)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  if (xmin) *xmin = priv->xmin;
  if (ymin) *ymin = priv->ymin;
  if (xmax) *xmax = priv->xmax;
  if (ymax) *ymax = priv->ymax;
}

void
clay_preview_set_size (ClayPreview *preview,
                       gint         width,
                       gint         height)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  priv->width  = width;
  priv->height = height;

  gtk_widget_set_size_request (priv->area, width, height);
}

/**
 * clay_preview_get_size:
 * @preview: a #ClayPreview widget
 * @width: (out) (optional): return location for the preview area width
 * @height: (out) (optional): return location for the preview area height
 *
 * Since: 2.2
 **/
void
clay_preview_get_size (ClayPreview *preview,
                       gint        *width,
                       gint        *height)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  if (width)  *width  = priv->width;
  if (height) *height = priv->height;
}

void
clay_preview_set_offsets (ClayPreview *preview,
                          gint         xoff,
                          gint         yoff)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  priv->xoff = xoff;
  priv->yoff = yoff;
}

/**
 * clay_preview_get_offsets:
 * @preview: A #ClayPreview:
 * @xoff: (out) (optional): X offset
 * @yoff: (out) (optional): Y offset
 */
void
clay_preview_get_offsets (ClayPreview *preview,
                          gint        *xoff,
                          gint        *yoff)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  if (xoff) *xoff = priv->xoff;
  if (yoff) *yoff = priv->yoff;
}

/**
 * clay_preview_get_position:
 * @preview: a #ClayPreview widget
 * @x: (out) (optional): return location for the horizontal offset
 * @y: (out) (optional): return location for the vertical offset
 *
 * Since: 2.2
 **/
void
clay_preview_get_position (ClayPreview *preview,
                           gint        *x,
                           gint        *y)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  if (x) *x = priv->xoff + priv->xmin;
  if (y) *y = priv->yoff + priv->ymin;
}

/**
 * clay_preview_transform:
 * @preview: a #ClayPreview widget
 * @src_x:   horizontal position on the previewed image
 * @src_y:   vertical position on the previewed image
 * @dest_x: (out): returns the transformed horizontal position
 * @dest_y: (out): returns the transformed vertical position
 *
 * Transforms from image to widget coordinates.
 *
 * Since: 2.4
 **/
void
clay_preview_transform (ClayPreview *preview,
                        gint         src_x,
                        gint         src_y,
                        gint        *dest_x,
                        gint        *dest_y)
{
  g_return_if_fail (CLAY_IS_PREVIEW (preview));
  g_return_if_fail (dest_x != NULL && dest_y != NULL);

  CLAY_PREVIEW_GET_CLASS (preview)->transform (preview,
                                               src_x, src_y, dest_x, dest_y);
}

/**
 * clay_preview_untransform:
 * @preview: a #ClayPreview widget
 * @src_x:   horizontal position relative to the preview area's origin
 * @src_y:   vertical position relative to  preview area's origin
 * @dest_x: (out): returns the untransformed horizontal position
 * @dest_y: (out): returns the untransformed vertical position
 *
 * Transforms from widget to image coordinates.
 *
 * Since: 2.4
 **/
void
clay_preview_untransform (ClayPreview *preview,
                          gint         src_x,
                          gint         src_y,
                          gint        *dest_x,
                          gint        *dest_y)
{
  g_return_if_fail (CLAY_IS_PREVIEW (preview));
  g_return_if_fail (dest_x != NULL && dest_y != NULL);

  CLAY_PREVIEW_GET_CLASS (preview)->untransform (preview,
                                                 src_x, src_y, dest_x, dest_y);
}

/**
 * clay_preview_get_frame:
 * @preview: a #ClayPreview widget
 *
 * Returns: (transfer none) (type GtkAspectFrame): a pointer to the
 * #GtkAspectFrame used in the @preview.
 *
 * Since: 3.0
 **/
GtkWidget *
clay_preview_get_frame (ClayPreview  *preview)
{
  ClayPreviewPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PREVIEW (preview), NULL);

  priv = clay_preview_get_instance_private (preview);

  return priv->frame;
}

/**
 * clay_preview_get_grid:
 * @preview: a #ClayPreview widget
 *
 * Returns: (transfer none) (type GtkGrid): a pointer to
 * the #GtkGrid used in the @preview.
 *
 * Since: 3.0
 **/
GtkWidget *
clay_preview_get_grid (ClayPreview  *preview)
{
  ClayPreviewPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PREVIEW (preview), NULL);

  priv = clay_preview_get_instance_private (preview);

  return priv->grid;
}

/**
 * clay_preview_get_area:
 * @preview: a #ClayPreview widget
 *
 * In most cases, you shouldn't need to access the #ClayPreviewArea
 * that is being used in the @preview. Sometimes however, you need to.
 * For example if you want to receive mouse events from the area. In
 * such cases, use clay_preview_get_area().
 *
 * Returns: (transfer none) (type ClayPreviewArea): a pointer to the
 * #ClayPreviewArea used in the @preview.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_preview_get_area (ClayPreview  *preview)
{
  ClayPreviewPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PREVIEW (preview), NULL);

  priv = clay_preview_get_instance_private (preview);

  return priv->area;
}

/**
 * clay_preview_draw:
 * @preview: a #ClayPreview widget
 *
 * Calls the ClayPreview::draw method. ClayPreview itself doesn't
 * implement a default draw method so the behavior is determined by
 * the derived class implementing this method.
 *
 * #ClayDrawablePreview implements clay_preview_draw() by drawing the
 * original, unmodified drawable to the @preview.
 *
 * Since: 2.2
 **/
void
clay_preview_draw (ClayPreview *preview)
{
  ClayPreviewClass *class = CLAY_PREVIEW_GET_CLASS (preview);

  if (class->draw)
    class->draw (preview);
}

/**
 * clay_preview_draw_buffer:
 * @preview:   a #ClayPreview widget
 * @buffer: (array): a pixel buffer the size of the preview
 * @rowstride: the @buffer's rowstride
 *
 * Calls the ClayPreview::draw_buffer method. ClayPreview itself
 * doesn't implement this method so the behavior is determined by the
 * derived class implementing this method.
 *
 * Since: 2.2
 **/
void
clay_preview_draw_buffer (ClayPreview  *preview,
                          const guchar *buffer,
                          gint          rowstride)
{
  ClayPreviewClass *class = CLAY_PREVIEW_GET_CLASS (preview);

  if (class->draw_buffer)
    class->draw_buffer (preview, buffer, rowstride);
}

/**
 * clay_preview_invalidate:
 * @preview: a #ClayPreview widget
 *
 * This function starts or renews a short low-priority timeout. When
 * the timeout expires, the ClayPreview::invalidated signal is emitted
 * which will usually cause the @preview to be updated.
 *
 * This function does nothing unless the "Preview" button is checked.
 *
 * During the emission of the signal a busy cursor is set on the
 * toplevel window containing the @preview and on the preview area
 * itself.
 *
 * Since: 2.2
 **/
void
clay_preview_invalidate (ClayPreview *preview)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  if (priv->update_preview)
    {
      if (priv->timeout_id)
        g_source_remove (priv->timeout_id);

      priv->timeout_id =
        g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, PREVIEW_TIMEOUT,
                            (GSourceFunc) clay_preview_invalidate_now,
                            preview, NULL);
    }
}

/**
 * clay_preview_set_default_cursor:
 * @preview: a #ClayPreview widget
 * @cursor:  a #GdkCursor or %NULL
 *
 * Sets the default mouse cursor for the preview.  Note that this will
 * be overridden by a %GDK_FLEUR if the preview has scrollbars, or by a
 * %GDK_WATCH when the preview is invalidated.
 *
 * Since: 2.2
 **/
void
clay_preview_set_default_cursor (ClayPreview *preview,
                                 GdkCursor   *cursor)
{
  ClayPreviewPrivate *priv;

  g_return_if_fail (CLAY_IS_PREVIEW (preview));

  priv = clay_preview_get_instance_private (preview);

  g_set_object (&priv->default_cursor, cursor);
}

/**
 * clay_preview_get_default_cursor:
 * @preview: a #ClayPreview widget
 *
 * See clay_preview_set_default_cursor():
 *
 * Returns: (transfer none): The @preview's default #GdkCursor.
 *
 * Since: 3.0
 **/
GdkCursor *
clay_preview_get_default_cursor (ClayPreview *preview)
{
  ClayPreviewPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PREVIEW (preview), NULL);

  priv = clay_preview_get_instance_private (preview);

  return priv->default_cursor;
}

/**
 * clay_preview_get_controls:
 * @preview: a #ClayPreview widget
 *
 * Gives access to the horizontal #GtkBox at the bottom of the preview
 * that contains the update toggle. Derived widgets can use this function
 * if they need to add controls to this area.
 *
 * Returns: (transfer none) (type GtkBox): the horizontal #GtkBox at the
 * bottom of the preview.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_preview_get_controls (ClayPreview *preview)
{
  ClayPreviewPrivate *priv;

  g_return_val_if_fail (CLAY_IS_PREVIEW (preview), NULL);

  priv = clay_preview_get_instance_private (preview);

  return priv->controls;
}
