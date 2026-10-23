/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claybrushchooser.c
 * Copyright (C) 1998 Andy Thomas
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clay.h"

#include "clayuitypes.h"
#include "claybrushchooser.h"
#include "clayuimarshal.h"

#include "libclay-intl.h"


/**
 * SECTION: claybrushchooser
 * @title: ClayBrushChooser
 * @short_description: A button which pops up a brush selection dialog.
 *
 * A button which pops up a brush selection dialog.
 *
 * Note that this widget draws itself using `GEGL` code. You **must** call
 * [func@Gegl.init] first to be able to use this chooser.
 **/

#define CELL_SIZE 40

struct _ClayBrushChooser
{
  ClayResourceChooser  parent_instance;

  GtkWidget           *preview;
  GtkWidget           *popup;

  ClayBrush           *brush;
  gint                 width;
  gint                 height;
  GeglBuffer          *buffer;
  GeglBuffer          *mask;
};


static void           clay_brush_chooser_finalize         (GObject          *object);

static gboolean       clay_brush_select_on_preview_events (GtkWidget        *widget,
                                                           GdkEvent         *event,
                                                           ClayBrushChooser *button);

static void           clay_brush_select_preview_draw      (ClayBrushChooser *chooser,
                                                           ClayPreviewArea  *area);

static void           clay_brush_chooser_draw             (ClayBrushChooser *self);
static void           clay_brush_chooser_get_brush_bitmap (ClayBrushChooser *self,
                                                           gint              width,
                                                           gint              height);

/* Popup methods. */
static void           clay_brush_chooser_open_popup       (ClayBrushChooser *button,
                                                           gint              x,
                                                           gint              y);
static void           clay_brush_chooser_close_popup      (ClayBrushChooser *button);


static const GtkTargetEntry drag_target = { "application/x-clay-brush-name", 0, 0 };

G_DEFINE_FINAL_TYPE (ClayBrushChooser, clay_brush_chooser, CLAY_TYPE_RESOURCE_CHOOSER)


static void
clay_brush_chooser_class_init (ClayBrushChooserClass *klass)
{
  GObjectClass                  *object_class = G_OBJECT_CLASS (klass);
  ClayResourceChooserClass *res_class    = CLAY_RESOURCE_CHOOSER_CLASS (klass);

  res_class->draw_interior = (void (*)(ClayResourceChooser *)) clay_brush_chooser_draw;
  res_class->resource_type = CLAY_TYPE_BRUSH;

  object_class->finalize   = clay_brush_chooser_finalize;
}

static void
clay_brush_chooser_init (ClayBrushChooser *chooser)
{
  GtkWidget *widget;
  gint       scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (chooser));

  chooser->brush  = NULL;
  chooser->buffer = NULL;
  chooser->mask   = NULL;

  widget = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (widget), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (chooser), widget, FALSE, FALSE, 0);
  gtk_widget_set_visible (widget, TRUE);

  chooser->preview = clay_preview_area_new ();
  gtk_widget_add_events (chooser->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (chooser->preview, scale_factor * CELL_SIZE, scale_factor * CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (widget), chooser->preview);
  gtk_widget_set_visible (chooser->preview, TRUE);

  g_signal_connect_swapped (chooser->preview, "size-allocate",
                            G_CALLBACK (clay_brush_chooser_draw),
                            chooser);

  g_signal_connect (chooser->preview, "event",
                    G_CALLBACK (clay_brush_select_on_preview_events),
                    chooser);

  widget = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (chooser), widget, FALSE, FALSE, 0);

  _clay_resource_chooser_set_drag_target (CLAY_RESOURCE_CHOOSER (chooser),
                                               chooser->preview,
                                               &drag_target);

  _clay_resource_chooser_set_clickable (CLAY_RESOURCE_CHOOSER (chooser),
                                             widget);
  gtk_widget_set_visible (widget, TRUE);
}

static void
clay_brush_chooser_finalize (GObject *object)
{
  ClayBrushChooser *chooser = CLAY_BRUSH_CHOOSER (object);

  g_clear_object (&chooser->buffer);
  g_clear_object (&chooser->mask);

  G_OBJECT_CLASS (clay_brush_chooser_parent_class)->finalize (object);
}

/**
 * clay_brush_chooser_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label: (nullable): Button label or %NULL for no label.
 * @brush: (nullable): Initial brush.
 *
 * Creates a new #GtkWidget that lets a user choose a brush.
 * You can put this widget in a plug-in dialog.
 *
 * When brush is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
clay_brush_chooser_new (const gchar *title,
                        const gchar *label,
                        ClayBrush   *brush)
{
  GtkWidget *self;

  g_return_val_if_fail (brush == NULL || CLAY_IS_BRUSH (brush), NULL);

  if (brush == NULL)
    brush = clay_context_get_brush ();

  if (title)
    self = g_object_new (CLAY_TYPE_BRUSH_CHOOSER,
                         "title",     title,
                         "label",     label,
                         "resource",  brush,
                         NULL);
  else
    self = g_object_new (CLAY_TYPE_BRUSH_CHOOSER,
                         "label",     label,
                         "resource",  brush,
                         NULL);

  return self;
}

/*  private functions  */

static void
clay_brush_chooser_draw (ClayBrushChooser *chooser)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (chooser->preview, &allocation);

  clay_brush_chooser_get_brush_bitmap (chooser, allocation.width, allocation.height);
  clay_brush_select_preview_draw (chooser, CLAY_PREVIEW_AREA (chooser->preview));
}

static void
clay_brush_chooser_get_brush_bitmap (ClayBrushChooser *chooser,
                                     gint              width,
                                     gint              height)
{
  ClayBrush *brush;

  g_object_get (chooser, "resource", &brush, NULL);
  if (! brush)
    return;

  if (chooser->brush  == brush &&
      chooser->width  == width &&
      chooser->height == height)
    {
      /* Let's assume brush contents is not changing in a single run. */
      g_object_unref (brush);
      return;
    }

  g_clear_object (&chooser->buffer);
  g_clear_object (&chooser->mask);
  chooser->width = chooser->height = 0;

  chooser->brush  = brush;
  chooser->buffer = clay_brush_get_buffer (brush, width, height, NULL);

  if (chooser->buffer)
    {
      GeglColor *white_color = gegl_color_new ("rgb(1.0,1.0,1.0)");
      GeglNode  *graph;
      GeglNode  *white;
      GeglNode  *source;
      GeglNode  *op;

      chooser->width  = gegl_buffer_get_width (chooser->buffer);
      chooser->height = gegl_buffer_get_height (chooser->buffer);

      graph = gegl_node_new ();
      white = gegl_node_new_child (graph,
                                   "operation", "gegl:rectangle",
                                   "x",         0.0,
                                   "y",         0.0,
                                   "width",     (gdouble) gegl_buffer_get_width (chooser->buffer),
                                   "height",    (gdouble) gegl_buffer_get_height (chooser->buffer),
                                   "color",     white_color,
                                    NULL);
      source = gegl_node_new_child (graph,
                                    "operation", "gegl:buffer-source",
                                    "buffer",    chooser->buffer,
                                    NULL);
      op = gegl_node_new_child (graph,
                                "operation",   "svg:src-over",
                                NULL);
      gegl_node_link_many (white, op, NULL);
      gegl_node_connect (source, "output", op, "aux");
      gegl_node_blit_buffer (op, chooser->buffer, NULL, 0, GEGL_ABYSS_NONE);

      g_object_unref (graph);
      g_object_unref (white_color);
    }
  else
    {
      GeglBufferIterator *iter;

      chooser->mask   = clay_brush_get_mask (brush, width, height, NULL);
      chooser->width  = gegl_buffer_get_width (chooser->mask);
      chooser->height = gegl_buffer_get_height (chooser->mask);

      /* More common to display mask brushes as black on white. */
      iter = gegl_buffer_iterator_new (chooser->mask, NULL, 0,
                                       babl_format ("Y' u8"),
                                       GEGL_ACCESS_READWRITE,
                                       GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          guint8 *data = iter->items[0].data;

          for (gint i = 0; i < iter->length; i++)
            {
              *data = 255 - *data;
              data++;
            }
        }
    }

  g_object_unref (brush);
}

/* On mouse events in self's preview, popup a zoom view of entire brush */
static gboolean
clay_brush_select_on_preview_events (GtkWidget        *widget,
                                     GdkEvent         *event,
                                     ClayBrushChooser *self)
{
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_add (widget);
          clay_brush_chooser_open_popup (self,
                                               bevent->x, bevent->y);
        }
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_remove (widget);
          clay_brush_chooser_close_popup (self);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

/* Draw a ClayPreviewArea with a given bitmap. */
static void
clay_brush_select_preview_draw (ClayBrushChooser *chooser,
                                ClayPreviewArea  *preview)
{
  ClayPreviewArea *area = CLAY_PREVIEW_AREA (preview);
  GeglBuffer      *src_buffer;
  const Babl      *format;
  guchar          *src;
  ClayImageType    type;
  gint             rowstride;
  GtkAllocation    allocation;
  gint             x = 0;
  gint             y = 0;

  gtk_widget_get_allocation (GTK_WIDGET (preview), &allocation);

  /* Fill with white. */
  if (chooser->width < allocation.width ||
      chooser->height < allocation.height)
    {
      clay_preview_area_fill (area,
                              0, 0,
                              allocation.width,
                              allocation.height,
                              0xFF, 0xFF, 0xFF);

      x = ((allocation.width  - chooser->width)  / 2);
      y = ((allocation.height - chooser->height) / 2);
    }

  /* Draw the brush. */
  if (chooser->buffer)
    {
      src_buffer = chooser->buffer;
      format     = gegl_buffer_get_format (src_buffer);
      rowstride  = chooser->width * babl_format_get_bytes_per_pixel (format);
      if (babl_format_has_alpha (format))
        type = CLAY_RGBA_IMAGE;
      else
        type = CLAY_RGB_IMAGE;
    }
  else
    {
      src_buffer = chooser->mask;
      format     = babl_format ("Y' u8");
      rowstride  = chooser->width;
      type       = CLAY_GRAY_IMAGE;
    }

  src = g_try_malloc (sizeof (guchar) * rowstride * chooser->height);

  gegl_buffer_get (src_buffer, NULL, 1.0, format, src, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  clay_preview_area_draw (area, x, y, chooser->width, chooser->height, type, src, rowstride);

  g_free (src);
}

/* popup methods. */

static void
clay_brush_chooser_open_popup (ClayBrushChooser *chooser,
                               gint              x,
                               gint              y)
{
  GtkWidget     *frame;
  GtkWidget     *preview;
  GdkMonitor    *monitor;
  GdkRectangle   workarea;
  gint           x_org;
  gint           y_org;

  if (chooser->popup)
    clay_brush_chooser_close_popup (chooser);

  if (chooser->width <= CELL_SIZE && chooser->height <= CELL_SIZE)
    return;

  chooser->popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (chooser->popup), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (chooser->popup),
                         gtk_widget_get_screen (GTK_WIDGET (chooser)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (chooser->popup), frame);
  gtk_widget_set_visible (frame, TRUE);

  preview = clay_preview_area_new ();
  gtk_widget_set_size_request (preview, chooser->width, chooser->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_set_visible (preview, TRUE);

  /* decide where to put the popup: near the preview i.e. at mousedown coords */
  gdk_window_get_origin (gtk_widget_get_window (chooser->preview),
                         &x_org, &y_org);

  monitor = clay_widget_get_monitor (GTK_WIDGET (chooser));
  gdk_monitor_get_workarea (monitor, &workarea);

  x = x_org + x - (chooser->width  / 2);
  y = y_org + y - (chooser->height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - chooser->width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - chooser->height);

  gtk_window_move (GTK_WINDOW (chooser->popup), x, y);

  gtk_widget_set_visible (chooser->popup, TRUE);

  /*  Draw popup now. Usual events do not cause a draw. */
  clay_brush_select_preview_draw (chooser, CLAY_PREVIEW_AREA (preview));
  gdk_window_set_transient_for (gtk_widget_get_window (chooser->popup),
                                gtk_widget_get_window (gtk_widget_get_toplevel (GTK_WIDGET (chooser))));
}

static void
clay_brush_chooser_close_popup (ClayBrushChooser *self)
{
  g_clear_pointer (&self->popup, gtk_widget_destroy);
}
