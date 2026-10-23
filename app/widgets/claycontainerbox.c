/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerbox.c
 * Copyright (C) 2004-2025 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clay-utils.h"

#include "claycontainerbox.h"
#include "claycontainerview.h"
#include "claydnd.h"
#include "claydocked.h"
#include "claypropwidgets.h"
#include "clayview.h"
#include "clayviewrenderer.h"


typedef struct _ClayContainerBoxPrivate ClayContainerBoxPrivate;

struct _ClayContainerBoxPrivate
{
  GtkGesture *zoom_gesture;
  gdouble     zoom_last_value;
  gdouble     zoom_accum_scroll_delta;
};

#define GET_PRIVATE(obj) \
  ((ClayContainerBoxPrivate *) clay_container_box_get_instance_private ((ClayContainerBox *) (obj)))


static void   clay_container_box_view_iface_init   (ClayContainerViewInterface *iface);
static void   clay_container_box_docked_iface_init (ClayDockedInterface        *iface);

static void   clay_container_box_constructed         (GObject          *object);
static void   clay_container_box_finalize            (GObject          *object);

static void   clay_container_box_set_context         (ClayDocked       *docked,
                                                      ClayContext      *context);
static GtkWidget *
              clay_container_box_get_preview         (ClayDocked       *docked,
                                                      ClayContext      *context,
                                                      GtkIconSize       size);

static gboolean
              clay_container_box_scroll_to_zoom      (GtkWidget        *widget,
                                                      GdkEventScroll   *event,
                                                      ClayContainerBox *box);
static void   clay_container_box_zoom_gesture_begin  (GtkGestureZoom   *gesture,
                                                      GdkEventSequence *sequence,
                                                      ClayContainerBox *box);
static void   clay_container_box_zoom_gesture_update (GtkGestureZoom   *gesture,
                                                      GdkEventSequence *sequence,
                                                      ClayContainerBox *box);


G_DEFINE_TYPE_WITH_CODE (ClayContainerBox, clay_container_box,
                         CLAY_TYPE_EDITOR,
                         G_ADD_PRIVATE (ClayContainerBox)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_container_box_view_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_container_box_docked_iface_init))

#define parent_class clay_container_box_parent_class


static void
clay_container_box_class_init (ClayContainerBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_container_box_constructed;
  object_class->finalize     = clay_container_box_finalize;
  object_class->set_property = _clay_container_view_set_property;
  object_class->get_property = _clay_container_view_get_property;

  _clay_container_view_install_properties (object_class);
}

static void
clay_container_box_init (ClayContainerBox *box)
{
  ClayContainerBoxPrivate *priv = GET_PRIVATE (box);
  GtkWidget               *sb;

  box->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), box->scrolled_win, TRUE, TRUE, 0);
  gtk_widget_set_visible (box->scrolled_win, TRUE);

  gtk_widget_add_events (box->scrolled_win, GDK_TOUCHPAD_GESTURE_MASK);

  g_signal_connect (box->scrolled_win, "scroll-event",
                    G_CALLBACK (clay_container_box_scroll_to_zoom),
                    box);

  sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (box->scrolled_win));
  gtk_widget_set_can_focus (sb, FALSE);

  priv->zoom_gesture = gtk_gesture_zoom_new (GTK_WIDGET (box->scrolled_win));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->zoom_gesture),
                                              GTK_PHASE_CAPTURE);

  /* The default signal handler needs to run first to setup scale delta */
  g_signal_connect_after (priv->zoom_gesture, "begin",
                          G_CALLBACK (clay_container_box_zoom_gesture_begin),
                          box);
  g_signal_connect_after (priv->zoom_gesture, "update",
                          G_CALLBACK (clay_container_box_zoom_gesture_update),
                          box);
}

static void
clay_container_box_view_iface_init (ClayContainerViewInterface *iface)
{
}

static void
clay_container_box_docked_iface_init (ClayDockedInterface *iface)
{
  iface->get_preview = clay_container_box_get_preview;
  iface->set_context = clay_container_box_set_context;
}

static void
clay_container_box_constructed (GObject *object)
{
  ClayContainerBox *box = CLAY_CONTAINER_BOX (object);

  /* This is evil: the hash table of "insert_data" is created on
   * demand when ClayContainerView API is used, using a
   * value_free_func that is set in the interface_init functions of
   * its implementors. Therefore, no ClayContainerView API must be
   * called from any init() function, because the interface_init()
   * function of a subclass that sets the right value_free_func might
   * not have been called yet, leaving the insert_data hash table
   * without memory management.
   *
   * Call ClayContainerView API from GObject::constructed() instead,
   * which runs after everything is set up correctly.
   */
  clay_container_view_set_dnd_widget (CLAY_CONTAINER_VIEW (box),
                                      box->scrolled_win);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
clay_container_box_finalize (GObject *object)
{
  ClayContainerBoxPrivate *priv = GET_PRIVATE (object);

  g_clear_object (&priv->zoom_gesture);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
clay_container_box_set_size_request (ClayContainerBox *box,
                                     gint              width,
                                     gint              height)
{
  ClayContainerView      *view;
  GtkScrolledWindowClass *sw_class;
  GtkStyleContext        *sw_style;
  GtkWidget              *sb;
  GtkRequisition          req;
  GtkBorder               border;
  gint                    view_size;
  gint                    scrollbar_width;
  gint                    border_x;
  gint                    border_y;

  g_return_if_fail (CLAY_IS_CONTAINER_BOX (box));

  view = CLAY_CONTAINER_VIEW (box);

  view_size = clay_container_view_get_view_size (view, NULL);

  g_return_if_fail (width  <= 0 || width  >= view_size);
  g_return_if_fail (height <= 0 || height >= view_size);

  sw_class = GTK_SCROLLED_WINDOW_GET_CLASS (box->scrolled_win);

  if (sw_class->scrollbar_spacing >= 0)
    scrollbar_width = sw_class->scrollbar_spacing;
  else
    gtk_widget_style_get (GTK_WIDGET (box->scrolled_win),
                          "scrollbar-spacing", &scrollbar_width,
                          NULL);

  sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (box->scrolled_win));

  gtk_widget_get_preferred_size (sb, &req, NULL);
  scrollbar_width += req.width;

  border_x = border_y = gtk_container_get_border_width (GTK_CONTAINER (box));

  sw_style = gtk_widget_get_style_context (box->scrolled_win);

  gtk_style_context_get_border (sw_style, gtk_widget_get_state_flags (box->scrolled_win), &border);

  border_x += border.left + border.right + scrollbar_width;
  border_y += border.top + border.bottom;

  gtk_widget_set_size_request (box->scrolled_win,
                               width  > 0 ? width  + border_x : -1,
                               height > 0 ? height + border_y : -1);
}

static void
clay_container_box_set_context (ClayDocked  *docked,
                                ClayContext *context)
{
  clay_container_view_set_context (CLAY_CONTAINER_VIEW (docked), context);
}

static GtkWidget *
clay_container_box_get_preview (ClayDocked   *docked,
                                ClayContext  *context,
                                GtkIconSize   size)
{
  ClayContainerView *view = CLAY_CONTAINER_VIEW (docked);
  ClayContainer     *container;
  GtkWidget         *preview;
  gint               width;
  gint               height;
  gint               border_width = 1;
  const gchar       *prop_name;

  container = clay_container_view_get_container (view);

  g_return_val_if_fail (container != NULL, NULL);

  gtk_icon_size_lookup (size, &width, &height);

  prop_name =
    clay_context_type_to_prop_name (clay_container_get_child_type (container));

  preview = clay_prop_view_new (G_OBJECT (context), prop_name,
                                context, height);
  CLAY_VIEW (preview)->renderer->size = -1;

  clay_container_view_get_view_size (view, &border_width);

  border_width = MIN (1, border_width);

  clay_view_renderer_set_size_full (CLAY_VIEW (preview)->renderer,
                                    width, height, border_width);

  return preview;
}

/* We want to zoom on each 1/4 scroll events to roughly match zooming
 * behavior  of the main canvas. Each step of CLAY_VIEW_SIZE_* is
 * approximately  of sqrt(2) = 1.4 relative change. The main canvas
 * on the other hand has zoom step equal to ZOOM_MIN_STEP=1.1
 * (see clay_zoom_model_zoom_step).
 */
#define SCROLL_ZOOM_STEP_SIZE 0.25

static gboolean
clay_container_box_scroll_to_zoom (GtkWidget        *widget,
                                   GdkEventScroll   *event,
                                   ClayContainerBox *box)
{
  ClayContainerBoxPrivate *priv = GET_PRIVATE (box);
  ClayContainerView       *view = CLAY_CONTAINER_VIEW (box);
  gint                     view_border_width;
  gint                     view_size;

  if ((event->state & GDK_CONTROL_MASK) == 0)
    return FALSE;

  if (event->direction == GDK_SCROLL_UP)
    priv->zoom_accum_scroll_delta -= SCROLL_ZOOM_STEP_SIZE;
  else if (event->direction == GDK_SCROLL_DOWN)
    priv->zoom_accum_scroll_delta += SCROLL_ZOOM_STEP_SIZE;
  else if (event->direction == GDK_SCROLL_SMOOTH)
    priv->zoom_accum_scroll_delta += event->delta_y * SCROLL_ZOOM_STEP_SIZE;
  else
    return FALSE;

  view_size = clay_container_view_get_view_size (view, &view_border_width);

  if (priv->zoom_accum_scroll_delta > 1)
    {
      priv->zoom_accum_scroll_delta -= 1;

      view_size = clay_view_size_get_smaller (view_size);
    }
  else if (priv->zoom_accum_scroll_delta < -1)
    {
      priv->zoom_accum_scroll_delta += 1;

      view_size = clay_view_size_get_larger (view_size);
    }
  else
    {
      return TRUE;
    }

  clay_container_view_set_view_size (view, view_size, view_border_width);

  return TRUE;
}

#define ZOOM_GESTURE_STEP_SIZE 1.2

static void
clay_container_box_zoom_gesture_begin (GtkGestureZoom   *gesture,
                                       GdkEventSequence *sequence,
                                       ClayContainerBox *box)
{
  ClayContainerBoxPrivate *priv = GET_PRIVATE (box);

  priv->zoom_last_value = gtk_gesture_zoom_get_scale_delta (gesture);
}

static void
clay_container_box_zoom_gesture_update (GtkGestureZoom   *gesture,
                                        GdkEventSequence *sequence,
                                        ClayContainerBox *box)
{
  ClayContainerBoxPrivate *priv = GET_PRIVATE (box);
  ClayContainerView       *view = CLAY_CONTAINER_VIEW (box);
  gdouble                  current_scale;
  gdouble                  last_value;
  gdouble                  min_increase;
  gdouble                  max_decrease;
  gint                     view_border_width;
  gint                     view_size;

  last_value   = priv->zoom_last_value;
  min_increase = last_value * ZOOM_GESTURE_STEP_SIZE;
  max_decrease = last_value / ZOOM_GESTURE_STEP_SIZE;

  view_size = clay_container_view_get_view_size (view, &view_border_width);

  current_scale = gtk_gesture_zoom_get_scale_delta (gesture);

  if (current_scale > min_increase)
    {
      last_value = min_increase;
      view_size  = clay_view_size_get_larger (view_size);
    }
  else if (current_scale < max_decrease)
    {
      last_value = max_decrease;
      view_size  = clay_view_size_get_smaller (view_size);
    }
  else
    {
      return;
    }

  priv->zoom_last_value = last_value;

  clay_container_view_set_view_size (view, view_size, view_border_width);
}
