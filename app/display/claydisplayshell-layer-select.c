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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/claylayer.h"

#include "widgets/clayview.h"
#include "widgets/clayviewrenderer.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-layer-select.h"

#include "clay-intl.h"


typedef struct
{
  GtkWidget *window;
  GtkWidget *view;
  GtkWidget *label;

  ClayImage *image;
  GList     *orig_layers;
} LayerSelect;


/*  local function prototypes  */

static LayerSelect * layer_select_new       (ClayDisplayShell *shell,
                                             ClayImage        *image,
                                             GList            *layers,
                                             gint              view_size);
static void          layer_select_destroy   (LayerSelect      *layer_select,
                                             GdkEvent         *event);
static void          layer_select_advance   (LayerSelect      *layer_select,
                                             gint              move);
static gboolean      layer_select_events    (GtkWidget        *widget,
                                             GdkEvent         *event,
                                             LayerSelect      *layer_select);


/*  public functions  */

void
clay_display_shell_layer_select_init (ClayDisplayShell *shell,
                                      GdkEvent         *event,
                                      gint              move)
{
  LayerSelect   *layer_select;
  ClayImage     *image;
  GList         *layers;
  GdkGrabStatus  status;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (event != NULL);

  image = clay_display_get_image (shell->display);

  layers = clay_image_get_selected_layers (image);

  if (! layers)
    return;

  layer_select = layer_select_new (shell, image, layers,
                                   image->clay->config->layer_preview_size);
  layer_select_advance (layer_select, move);

  gtk_window_set_screen (GTK_WINDOW (layer_select->window),
                         gtk_widget_get_screen (GTK_WIDGET (shell)));

  gtk_widget_set_visible (layer_select->window, TRUE);

  status = gdk_seat_grab (gdk_event_get_seat (event),
                          gtk_widget_get_window (layer_select->window),
                          GDK_SEAT_CAPABILITY_KEYBOARD,
                          FALSE, NULL, event, NULL, NULL);

  if (status != GDK_GRAB_SUCCESS)
    {
      g_printerr ("gdk_keyboard_grab failed with status %d\n", status);
      layer_select_destroy (layer_select, event);
    }
}


/*  private functions  */

static LayerSelect *
layer_select_new (ClayDisplayShell *shell,
                  ClayImage        *image,
                  GList            *layers,
                  gint              view_size)
{
  LayerSelect *layer_select;
  GtkWidget   *frame1;
  GtkWidget   *frame2;
  GtkWidget   *hbox;

  layer_select = g_slice_new0 (LayerSelect);

  layer_select->image       = image;
  layer_select->orig_layers = g_list_copy (layers);

  layer_select->window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_role (GTK_WINDOW (layer_select->window), "clay-layer-select");
  gtk_window_set_title (GTK_WINDOW (layer_select->window), _("Layer Select"));
  gtk_window_set_position (GTK_WINDOW (layer_select->window), GTK_WIN_POS_MOUSE);
  gtk_widget_set_events (layer_select->window,
                         GDK_KEY_PRESS_MASK   |
                         GDK_KEY_RELEASE_MASK |
                         GDK_BUTTON_PRESS_MASK);

  g_signal_connect (layer_select->window, "event",
                    G_CALLBACK (layer_select_events),
                    layer_select);

  frame1 = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (layer_select->window), frame1);
  gtk_widget_set_visible (frame1, TRUE);

  frame2 = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame1), frame2);
  gtk_widget_set_visible (frame2, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  /*  the view  */
  layer_select->view =
    clay_view_new_by_types (clay_get_user_context (image->clay),
                            CLAY_TYPE_VIEW,
                            CLAY_TYPE_LAYER,
                            view_size, 1, FALSE);
  clay_view_renderer_set_color_config (CLAY_VIEW (layer_select->view)->renderer,
                                       clay_display_shell_get_color_config (shell));
  clay_view_set_viewable (CLAY_VIEW (layer_select->view),
                          g_list_length (layers) == 1 ? CLAY_VIEWABLE (layers->data) : NULL);
  gtk_box_pack_start (GTK_BOX (hbox), layer_select->view, FALSE, FALSE, 0);
  gtk_widget_set_visible (layer_select->view, TRUE);

  /*  the layer name label */
  layer_select->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), layer_select->label, FALSE, FALSE, 0);
  gtk_widget_set_visible (layer_select->label, TRUE);

  return layer_select;
}

static void
layer_select_destroy (LayerSelect *layer_select,
                      GdkEvent    *event)
{
  gdk_seat_ungrab (gdk_event_get_seat (event));

  gtk_widget_destroy (layer_select->window);

  /* Flush only if selection actually changed. */
  if (g_list_length (layer_select->orig_layers) !=
      g_list_length (clay_image_get_selected_layers (layer_select->image)))
    {
      clay_image_flush (layer_select->image);
    }
  else
    {
      GList *layers;
      GList *iter;

      layers = clay_image_get_selected_layers (layer_select->image);

      for (iter = layers; iter; iter = iter->next)
        if (! g_list_find (layer_select->orig_layers, iter->data))
          {
            clay_image_flush (layer_select->image);
            break;
          }
    }
  g_list_free (layer_select->orig_layers);

  g_slice_free (LayerSelect, layer_select);
}

static void
layer_select_advance (LayerSelect *layer_select,
                      gint         move)
{
  GList     *next_layers = NULL;
  GList     *selected_layers;
  GList     *iter;
  GList     *layers;
  gint       n_layers;
  gint       index;

  if (move == 0)
    return;

  /*  If there is a floating selection, allow no advancement  */
  if (clay_image_get_floating_selection (layer_select->image))
    return;

  selected_layers = clay_image_get_selected_layers (layer_select->image);

  if (! selected_layers)
    return;

  layers   = clay_image_get_layer_list (layer_select->image);
  n_layers = g_list_length (layers);

  for (iter = selected_layers; iter; iter = iter->next)
    {
      ClayLayer *next_layer;

      index = g_list_index (layers, iter->data);
      index += move;

      if (index < 0)
        index = n_layers - 1;
      else if (index >= n_layers)
        index = 0;

      next_layer = g_list_nth_data (layers, index);

      if (next_layer && ! g_list_find (next_layers, next_layer))
        next_layers = g_list_prepend (next_layers, next_layer);
    }
  g_list_free (layers);

  clay_image_set_selected_layers (layer_select->image, next_layers);
  selected_layers = clay_image_get_selected_layers (layer_select->image);

  if (selected_layers)
    {
      if (g_list_length (selected_layers) == 1)
        {
          clay_view_set_viewable (CLAY_VIEW (layer_select->view),
                                  CLAY_VIEWABLE (selected_layers->data));
          gtk_label_set_text (GTK_LABEL (layer_select->label),
                              clay_object_get_name (selected_layers->data));
        }
      else
        {
          clay_view_set_viewable (CLAY_VIEW (layer_select->view), NULL);
          gtk_label_set_text (GTK_LABEL (layer_select->label),
                              move > 0 ? _("Layer Selection Moved Down") :
                                         _("Layer Selection Moved Up"));
        }
    }
}

static gboolean
layer_select_events (GtkWidget   *widget,
                     GdkEvent    *event,
                     LayerSelect *layer_select)
{
  GdkEventKey *kevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      layer_select_destroy (layer_select, event);
      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
        {
        case GDK_KEY_Tab:
          layer_select_advance (layer_select, 1);
          break;
        case GDK_KEY_ISO_Left_Tab:
          layer_select_advance (layer_select, -1);
          break;
        }
      return TRUE;
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
        {
        case GDK_KEY_Alt_L: case GDK_KEY_Alt_R:
          kevent->state &= ~GDK_MOD1_MASK;
          break;
        case GDK_KEY_Control_L: case GDK_KEY_Control_R:
          kevent->state &= ~GDK_CONTROL_MASK;
          break;
        case GDK_KEY_Shift_L: case GDK_KEY_Shift_R:
          kevent->state &= ~GDK_SHIFT_MASK;
          break;
        }

      if (! (kevent->state & GDK_CONTROL_MASK))
        layer_select_destroy (layer_select, event);

      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}
