/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypatternchooser.c
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
#include "claypatternchooser.h"
#include "clayuimarshal.h"

#include "libclay-intl.h"


/**
 * SECTION: claypatternchooser
 * @title: ClayPatternChooser
 * @short_description: A button which pops up a pattern selection dialog.
 *
 * A button which pops up a pattern selection dialog.
 *
 * Note that this widget draws itself using `GEGL` code. You **must** call
 * [func@Gegl.init] first to be able to use this chooser.
 **/

#define CELL_SIZE 40

struct _ClayPatternChooser
{
  ClayResourceChooser  parent_instance;

  GtkWidget           *preview;
  GtkWidget           *popup;

  ClayPattern         *pattern;
  GeglBuffer          *buffer;
  gint                 width;
  gint                 height;
};

/*  local  */

static gboolean clay_pattern_select_on_preview_events  (GtkWidget           *widget,
                                                        GdkEvent            *event,
                                                        ClayPatternChooser  *button);

/* local drawing methods. */
static void     clay_pattern_select_preview_fill_draw  (ClayPatternChooser  *chooser,
                                                        ClayPreviewArea     *area);

static void     clay_pattern_chooser_draw              (ClayResourceChooser *self);
static void     clay_pattern_chooser_get_pattern_image (ClayPatternChooser  *self,
                                                        gint                 width,
                                                        gint                 height);

/* Popup methods. */
static void     clay_pattern_chooser_open_popup        (ClayPatternChooser  *button,
                                                        gint                 x,
                                                        gint                 y);
static void     clay_pattern_chooser_close_popup       (ClayPatternChooser  *button);


/* A GtkTargetEntry has a string and two ints.
 * This is one, but we treat it as an array.
 */
static const GtkTargetEntry drag_target = { "application/x-clay-pattern-name", 0, 0 };

G_DEFINE_FINAL_TYPE (ClayPatternChooser, clay_pattern_chooser, CLAY_TYPE_RESOURCE_CHOOSER)


static void
clay_pattern_chooser_class_init (ClayPatternChooserClass *klass)
{
  ClayResourceChooserClass *superclass = CLAY_RESOURCE_CHOOSER_CLASS (klass);

  superclass->draw_interior = clay_pattern_chooser_draw;
  superclass->resource_type = CLAY_TYPE_PATTERN;
}

static void
clay_pattern_chooser_init (ClayPatternChooser *self)
{
  GtkWidget *frame;
  GtkWidget *button;

  frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (self), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  self->preview = clay_preview_area_new ();
  gtk_widget_add_events (self->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (self->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), self->preview);
  gtk_widget_set_visible (self->preview, TRUE);

  g_signal_connect_swapped (self->preview, "size-allocate",
                            G_CALLBACK (clay_pattern_chooser_draw),
                            self);

  g_signal_connect (self->preview, "event",
                    G_CALLBACK (clay_pattern_select_on_preview_events),
                    self);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (self), button, FALSE, FALSE, 0);

  _clay_resource_chooser_set_drag_target (CLAY_RESOURCE_CHOOSER (self),
                                         self->preview, &drag_target);

  _clay_resource_chooser_set_clickable (CLAY_RESOURCE_CHOOSER (self), button);
  gtk_widget_set_visible (button, TRUE);
}

/**
 * clay_pattern_chooser_new:
 * @title:   (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label:   (nullable): Button label or %NULL for no label.
 * @pattern: (nullable): Initial pattern.
 *
 * Creates a new #GtkWidget that lets a user choose a pattern.
 * You can put this widget in a table in a plug-in dialog.
 *
 * When pattern is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
clay_pattern_chooser_new (const gchar *title,
                          const gchar *label,
                          ClayPattern *pattern)
{
  GtkWidget *self;

  g_return_val_if_fail (pattern == NULL || CLAY_IS_PATTERN (pattern), NULL);

  if (pattern == NULL)
    pattern = clay_context_get_pattern ();

   if (title)
     self = g_object_new (CLAY_TYPE_PATTERN_CHOOSER,
                          "title",     title,
                          "label",     label,
                          "resource",  pattern,
                          NULL);
   else
     self = g_object_new (CLAY_TYPE_PATTERN_CHOOSER,
                          "label",     label,
                          "resource",  pattern,
                          NULL);

  clay_pattern_chooser_draw (CLAY_RESOURCE_CHOOSER (self));

  return self;
}


static void
clay_pattern_chooser_draw (ClayResourceChooser *chooser)
{
  ClayPatternChooser *pchooser = CLAY_PATTERN_CHOOSER (chooser);
  GtkAllocation       allocation;

  gtk_widget_get_allocation (pchooser->preview, &allocation);

  clay_pattern_chooser_get_pattern_image (pchooser, allocation.width, allocation.height);
  clay_pattern_select_preview_fill_draw (pchooser, CLAY_PREVIEW_AREA (pchooser->preview));
}

static void
clay_pattern_chooser_get_pattern_image (ClayPatternChooser *chooser,
                                        gint                width,
                                        gint                height)
{
  ClayPattern *pattern;

  g_object_get (chooser, "resource", &pattern, NULL);

  if (chooser->pattern == pattern &&
      chooser->width   == width &&
      chooser->height  == height)
    {
      /* Let's assume pattern contents is not changing in a single run. */
      g_object_unref (pattern);
      return;
    }

  g_clear_object (&chooser->buffer);

  chooser->pattern = pattern;
  chooser->buffer  = clay_pattern_get_buffer (pattern, width, height, NULL);
  chooser->width   = gegl_buffer_get_width (chooser->buffer);
  chooser->height  = gegl_buffer_get_height (chooser->buffer);

  g_object_unref (pattern);
}

/* On mouse events in self's preview, popup a zoom view of entire pattern */
static gboolean
clay_pattern_select_on_preview_events (GtkWidget          *widget,
                                       GdkEvent           *event,
                                       ClayPatternChooser *self)
{
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_add (widget);
          clay_pattern_chooser_open_popup (self,
                                               bevent->x, bevent->y);
        }
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_remove (widget);
          clay_pattern_chooser_close_popup (self);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

/* Fill a ClayPreviewArea with a image then draw. */
static void
clay_pattern_select_preview_fill_draw (ClayPatternChooser *chooser,
                                       ClayPreviewArea    *area)
{
  GeglBuffer    *src_buffer;
  const Babl    *format;
  const Babl    *model;
  guchar        *src;
  ClayImageType  type;
  gint           rowstride;
  GtkAllocation  allocation;
  gint           x = 0;
  gint           y = 0;

  gtk_widget_get_allocation (GTK_WIDGET (area), &allocation);

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

  /* Draw the pattern. */
  src_buffer = chooser->buffer;
  format     = gegl_buffer_get_format (src_buffer);
  rowstride  = chooser->width * babl_format_get_bytes_per_pixel (format);
  model      = babl_format_get_model (format);

  if (model == babl_model ("R'G'B'"))
    type = CLAY_RGB_IMAGE;
  else if (model == babl_model ("R'G'B'A"))
    type = CLAY_RGBA_IMAGE;
  else if (model == babl_model ("Y'"))
    type = CLAY_GRAY_IMAGE;
  else if (model == babl_model ("Y'A"))
    type = CLAY_GRAYA_IMAGE;
  else
    /* I just know that we can't have other formats because I set it up this way
     * in clay_pattern_get_buffer(). If we make the latter more generic, able to
     * return more types of pixel data, this should be reviewed. XXX
     */
    g_return_if_reached ();

  src = g_try_malloc (sizeof (guchar) * rowstride * chooser->height);

  gegl_buffer_get (chooser->buffer, NULL, 1.0, format, src, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  clay_preview_area_draw (area, x, y, chooser->width, chooser->height, type, src, rowstride);

  g_free (src);
}

/* popup methods. */

static void
clay_pattern_chooser_open_popup (ClayPatternChooser *chooser,
                                 gint                x,
                                 gint                y)
{
  GtkWidget    *frame;
  GtkWidget    *preview;
  GdkMonitor   *monitor;
  GdkRectangle  workarea;
  gint          x_org;
  gint          y_org;

  if (chooser->popup)
    clay_pattern_chooser_close_popup (chooser);

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
  clay_pattern_select_preview_fill_draw (chooser, CLAY_PREVIEW_AREA (preview));
}

static void
clay_pattern_chooser_close_popup (ClayPatternChooser *self)
{
  g_clear_pointer (&self->popup, gtk_widget_destroy);
}
