/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claynavigationeditor.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/claydisplayconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimageproxy.h"

#include "widgets/claydocked.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymenufactory.h"
#include "widgets/claynavigationview.h"
#include "widgets/clayuimanager.h"
#include "widgets/clayviewrenderer.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-appearance.h"
#include "claydisplayshell-scale.h"
#include "claydisplayshell-scroll.h"
#include "claydisplayshell-transform.h"
#include "claynavigationeditor.h"

#include "clay-intl.h"


static void        clay_navigation_editor_docked_iface_init            (ClayDockedInterface  *iface);

static void        clay_navigation_editor_dispose                      (GObject              *object);

static void        clay_navigation_editor_set_context                  (ClayDocked           *docked,
                                                                        ClayContext          *context);

static GtkWidget * clay_navigation_editor_new_private                  (ClayMenuFactory      *menu_factory,
                                                                        ClayDisplayShell     *shell);

static void        clay_navigation_editor_set_shell                    (ClayNavigationEditor *editor,
                                                                        ClayDisplayShell     *shell);
static gboolean    clay_navigation_editor_button_release               (GtkWidget            *widget,
                                                                        GdkEventButton       *bevent,
                                                                        ClayDisplayShell     *shell);
static void        clay_navigation_editor_marker_changed               (ClayNavigationView   *view,
                                                                        gdouble               center_x,
                                                                        gdouble               center_y,
                                                                        gdouble               width,
                                                                        gdouble               height,
                                                                        ClayNavigationEditor *editor);
static void        clay_navigation_editor_zoom                         (ClayNavigationView   *view,
                                                                        ClayZoomType          direction,
                                                                        gdouble               delta,
                                                                        ClayNavigationEditor *editor);
static void        clay_navigation_editor_scroll                       (ClayNavigationView   *view,
                                                                        GdkEventScroll       *sevent,
                                                                        ClayNavigationEditor *editor);

static void        clay_navigation_editor_zoom_adj_changed             (GtkAdjustment        *adj,
                                                                        ClayNavigationEditor *editor);

static void        clay_navigation_editor_shell_infinite_canvas_notify (ClayDisplayShell     *shell,
                                                                        const GParamSpec     *pspec,
                                                                        ClayNavigationEditor *editor);
static void        clay_navigation_editor_shell_scaled                 (ClayDisplayShell     *shell,
                                                                        ClayNavigationEditor *editor);
static void        clay_navigation_editor_shell_scrolled               (ClayDisplayShell     *shell,
                                                                        ClayNavigationEditor *editor);
static void        clay_navigation_editor_shell_rotated                (ClayDisplayShell     *shell,
                                                                        ClayNavigationEditor *editor);
static void        clay_navigation_editor_shell_reconnect              (ClayDisplayShell     *shell,
                                                                        ClayNavigationEditor *editor);

static void        clay_navigation_editor_viewable_size_changed        (ClayViewable         *viewable,
                                                                        ClayNavigationEditor *editor);

static void        clay_navigation_editor_options_show_canvas_notify   (ClayDisplayOptions   *options,
                                                                        const GParamSpec     *pspec,
                                                                        ClayNavigationEditor *editor);

static void        clay_navigation_editor_update_marker                (ClayNavigationEditor *editor);


G_DEFINE_TYPE_WITH_CODE (ClayNavigationEditor, clay_navigation_editor,
                         CLAY_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_navigation_editor_docked_iface_init))

#define parent_class clay_navigation_editor_parent_class


static void
clay_navigation_editor_class_init (ClayNavigationEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clay_navigation_editor_dispose;
}

static void
clay_navigation_editor_docked_iface_init (ClayDockedInterface *iface)
{
  iface->set_context = clay_navigation_editor_set_context;
}

static void
clay_navigation_editor_init (ClayNavigationEditor *editor)
{
  GtkWidget *frame;

  editor->context       = NULL;
  editor->shell         = NULL;
  editor->scale_timeout = 0;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  editor->view = clay_view_new_by_types (NULL,
                                         CLAY_TYPE_NAVIGATION_VIEW,
                                         CLAY_TYPE_IMAGE_PROXY,
                                         CLAY_VIEW_SIZE_MEDIUM, 0, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_set_visible (editor->view, TRUE);

  g_signal_connect (editor->view, "marker-changed",
                    G_CALLBACK (clay_navigation_editor_marker_changed),
                    editor);
  g_signal_connect (editor->view, "zoom",
                    G_CALLBACK (clay_navigation_editor_zoom),
                    editor);
  g_signal_connect (editor->view, "scroll",
                    G_CALLBACK (clay_navigation_editor_scroll),
                    editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
clay_navigation_editor_dispose (GObject *object)
{
  ClayNavigationEditor *editor = CLAY_NAVIGATION_EDITOR (object);

  if (editor->shell)
    clay_navigation_editor_set_shell (editor, NULL);

  if (editor->scale_timeout)
    {
      g_source_remove (editor->scale_timeout);
      editor->scale_timeout = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_navigation_editor_display_changed (ClayContext          *context,
                                        ClayDisplay          *display,
                                        ClayNavigationEditor *editor)
{
  ClayDisplayShell *shell = NULL;

  if (display && clay_display_get_image (display))
    shell = clay_display_get_shell (display);

  clay_navigation_editor_set_shell (editor, shell);
}

static void
clay_navigation_editor_image_chaged (ClayContext          *context,
                                     ClayImage            *image,
                                     ClayNavigationEditor *editor)
{
  ClayDisplay      *display = clay_context_get_display (context);
  ClayDisplayShell *shell   = NULL;

  if (display && image)
    shell = clay_display_get_shell (display);

  clay_navigation_editor_set_shell (editor, shell);
}

static void
clay_navigation_editor_set_context (ClayDocked  *docked,
                                    ClayContext *context)
{
  ClayNavigationEditor *editor  = CLAY_NAVIGATION_EDITOR (docked);
  ClayDisplay          *display = NULL;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_navigation_editor_display_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->context,
                                            clay_navigation_editor_image_chaged,
                                            editor);
    }

  editor->context = context;

  if (editor->context)
    {
      g_signal_connect (context, "display-changed",
                        G_CALLBACK (clay_navigation_editor_display_changed),
                        editor);
      /* make sure to also call clay_navigation_editor_set_shell() when the
       * last image is closed, even though the display isn't changed, so that
       * the editor is properly cleared.
       */
      g_signal_connect (context, "image-changed",
                        G_CALLBACK (clay_navigation_editor_image_chaged),
                        editor);

      display = clay_context_get_display (context);
    }

  clay_view_renderer_set_context (CLAY_VIEW (editor->view)->renderer,
                                  context);

  clay_navigation_editor_display_changed (editor->context,
                                          display,
                                          editor);
}


/*  public functions  */

GtkWidget *
clay_navigation_editor_new (ClayMenuFactory *menu_factory)
{
  return clay_navigation_editor_new_private (menu_factory, NULL);
}

void
clay_navigation_editor_popup (ClayDisplayShell *shell,
                              GtkWidget        *widget,
                              GdkEvent         *event,
                              gint              click_x,
                              gint              click_y)
{
  GtkStyleContext      *style = gtk_widget_get_style_context (widget);
  ClayNavigationEditor *editor;
  ClayNavigationView   *view;
  gint                  x, y;
  gint                  view_marker_center_x, view_marker_center_y;
  gint                  view_marker_width, view_marker_height;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (! shell->nav_popup)
    {
      GtkWidget *frame;

      shell->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (shell->nav_popup), frame);
      gtk_widget_set_visible (frame, TRUE);

      editor =
        CLAY_NAVIGATION_EDITOR (clay_navigation_editor_new_private (NULL,
                                                                    shell));
      gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (editor));
      gtk_widget_set_visible (GTK_WIDGET (editor), TRUE);

      g_signal_connect (editor->view, "button-release-event",
                        G_CALLBACK (clay_navigation_editor_button_release),
                        shell);
    }
  else
    {
      GtkWidget *bin = gtk_bin_get_child (GTK_BIN (shell->nav_popup));

      editor = CLAY_NAVIGATION_EDITOR (gtk_bin_get_child (GTK_BIN (bin)));
    }

  view = CLAY_NAVIGATION_VIEW (editor->view);

  /* Set poup screen */
  gtk_window_set_screen (GTK_WINDOW (shell->nav_popup),
                         gtk_widget_get_screen (widget));
  gtk_window_set_transient_for (GTK_WINDOW (shell->nav_popup),
                                GTK_WINDOW (gtk_widget_get_toplevel (widget)));

  clay_navigation_view_get_local_marker (view,
                                         &view_marker_center_x,
                                         &view_marker_center_y,
                                         &view_marker_width,
                                         &view_marker_height);
  /* Position the popup */
  {
    GdkMonitor   *monitor;
    GdkRectangle  workarea;
    GtkBorder     border;
    gint          x_origin, y_origin;
    gint          popup_width, popup_height;
    gint          border_width, border_height;
    gint          screen_click_x, screen_click_y;

    monitor = clay_widget_get_monitor (widget);
    gdk_monitor_get_workarea (monitor, &workarea);

    gdk_window_get_origin (gtk_widget_get_window (widget),
                           &x_origin, &y_origin);

    gtk_style_context_get_border (style, gtk_widget_get_state_flags (widget),
                                  &border);

    screen_click_x = x_origin + click_x;
    screen_click_y = y_origin + click_y;
    border_width   = 2 * border.left;
    border_height  = 2 * border.top;
    popup_width    = CLAY_VIEW (view)->renderer->width  - 2 * border_width;
    popup_height   = CLAY_VIEW (view)->renderer->height - 2 * border_height;

    x = screen_click_x -
        border_width -
        view_marker_center_x;

    y = screen_click_y -
        border_height -
        view_marker_center_y;

    /* When the image is zoomed out and overscrolled, the above
     * calculation risks positioning the popup far far away from the
     * click coordinate. We don't want that, so perform some clamping.
     */
    x = CLAMP (x, screen_click_x - popup_width,  screen_click_x);
    y = CLAMP (y, screen_click_y - popup_height, screen_click_y);

    /* If the popup doesn't fit into the screen, we have a problem.
     * We move the popup onscreen and risk that the pointer is not
     * in the square representing the viewable area anymore. Moving
     * the pointer will make the image scroll by a large amount,
     * but then it works as usual. Probably better than a popup that
     * is completely unusable in the lower right of the screen.
     *
     * Warping the pointer would be another solution ...
     */
    x = CLAMP (x, workarea.x, workarea.x + workarea.width  - popup_width);
    y = CLAMP (y, workarea.y, workarea.y + workarea.height - popup_height);

    gtk_window_move (GTK_WINDOW (shell->nav_popup), x, y);
  }

  gtk_widget_set_visible (shell->nav_popup, TRUE);
  gdk_display_flush (gtk_widget_get_display (shell->nav_popup));

  /* fill in then grab pointer */
  clay_navigation_view_set_motion_offset (view, 0, 0);
  clay_navigation_view_grab_pointer (view, event);
}


/*  private functions  */

static GtkWidget *
clay_navigation_editor_new_private (ClayMenuFactory  *menu_factory,
                                    ClayDisplayShell *shell)
{
  ClayNavigationEditor *editor;

  g_return_val_if_fail (menu_factory == NULL ||
                        CLAY_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (shell == NULL || CLAY_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (menu_factory || shell, NULL);

  if (shell)
    {
      Clay              *clay   = shell->display->clay;
      ClayDisplayConfig *config = shell->display->config;
      ClayView          *view;

      editor = g_object_new (CLAY_TYPE_NAVIGATION_EDITOR, NULL);

      view = CLAY_VIEW (editor->view);

      clay_view_renderer_set_size (view->renderer,
                                   config->nav_preview_size * 3,
                                   view->renderer->border_width);
      clay_view_renderer_set_context (view->renderer,
                                      clay_get_user_context (clay));
      clay_view_renderer_set_color_config (view->renderer,
                                           clay_display_shell_get_color_config (shell));

      clay_navigation_editor_set_shell (editor, shell);

    }
  else
    {
      GtkWidget *hscale;
      GtkWidget *hbox;

      editor = g_object_new (CLAY_TYPE_NAVIGATION_EDITOR,
                             "menu-factory",    menu_factory,
                             "menu-identifier", "<NavigationEditor>",
                             NULL);

      gtk_widget_set_size_request (editor->view,
                                   CLAY_VIEW_SIZE_HUGE,
                                   CLAY_VIEW_SIZE_HUGE);
      clay_view_set_expand (CLAY_VIEW (editor->view), TRUE);

      /* the editor buttons */

      editor->zoom_out_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor), "view",
                                       "view-zoom-out", NULL);

      editor->zoom_in_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor), "view",
                                       "view-zoom-in", NULL);

      editor->zoom_100_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor), "view",
                                       "view-zoom-1-1", NULL);

      editor->zoom_fit_in_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor), "view",
                                       "view-zoom-fit-in", NULL);

      editor->zoom_fill_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor), "view",
                                       "view-zoom-fill", NULL);

      editor->shrink_wrap_button =
        clay_editor_add_action_button (CLAY_EDITOR (editor), "view",
                                       "view-shrink-wrap", NULL);

      /* the zoom scale */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_end (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      editor->zoom_adjustment = gtk_adjustment_new (0.0, -8.0, 8.0,
                                                    0.5, 1.0, 0.0);

      g_signal_connect (editor->zoom_adjustment, "value-changed",
                        G_CALLBACK (clay_navigation_editor_zoom_adj_changed),
                        editor);

      hscale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL,
                              editor->zoom_adjustment);
      gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox), hscale, TRUE, TRUE, 0);
      gtk_widget_set_visible (hscale, TRUE);

      /* the zoom label */

      editor->zoom_label = gtk_label_new ("100%");
      gtk_label_set_width_chars (GTK_LABEL (editor->zoom_label), 7);
      gtk_box_pack_start (GTK_BOX (hbox), editor->zoom_label, FALSE, FALSE, 0);
      gtk_widget_set_visible (editor->zoom_label, TRUE);
    }

  return GTK_WIDGET (editor);
}

static void
clay_navigation_editor_set_shell (ClayNavigationEditor *editor,
                                  ClayDisplayShell     *shell)
{
  g_return_if_fail (CLAY_IS_NAVIGATION_EDITOR (editor));
  g_return_if_fail (! shell || CLAY_IS_DISPLAY_SHELL (shell));

  if (shell == editor->shell)
    return;

  if (editor->shell)
    {
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            clay_navigation_editor_shell_infinite_canvas_notify,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            clay_navigation_editor_shell_scaled,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            clay_navigation_editor_shell_scrolled,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            clay_navigation_editor_shell_rotated,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            clay_navigation_editor_shell_reconnect,
                                            editor);

      g_signal_handlers_disconnect_by_func (editor->shell->options,
                                            clay_navigation_editor_options_show_canvas_notify,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell->fullscreen_options,
                                            clay_navigation_editor_options_show_canvas_notify,
                                            editor);
    }
  else if (shell)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
    }

  editor->shell = shell;

  if (editor->shell)
    {
      ClayImage *image = clay_display_get_image (shell->display);

      g_clear_object (&editor->image_proxy);

      if (image)
        {
          editor->image_proxy = clay_image_proxy_new (image);

          g_signal_connect (
            editor->image_proxy, "size-changed",
            G_CALLBACK (clay_navigation_editor_viewable_size_changed),
            editor);
        }

      clay_view_set_viewable (CLAY_VIEW (editor->view),
                              CLAY_VIEWABLE (editor->image_proxy));

      g_signal_connect (editor->shell, "notify::infinite-canvas",
                        G_CALLBACK (clay_navigation_editor_shell_infinite_canvas_notify),
                        editor);
      g_signal_connect (editor->shell, "scaled",
                        G_CALLBACK (clay_navigation_editor_shell_scaled),
                        editor);
      g_signal_connect (editor->shell, "scrolled",
                        G_CALLBACK (clay_navigation_editor_shell_scrolled),
                        editor);
      g_signal_connect (editor->shell, "rotated",
                        G_CALLBACK (clay_navigation_editor_shell_rotated),
                        editor);
      g_signal_connect (editor->shell, "reconnect",
                        G_CALLBACK (clay_navigation_editor_shell_reconnect),
                        editor);

      g_signal_connect (editor->shell->options, "notify::show-canvas-boundary",
                        G_CALLBACK (clay_navigation_editor_options_show_canvas_notify),
                        editor);
      g_signal_connect (editor->shell->fullscreen_options, "notify::show-canvas-boundary",
                        G_CALLBACK (clay_navigation_editor_options_show_canvas_notify),
                        editor);

      clay_navigation_editor_shell_scaled (editor->shell, editor);
    }
  else
    {
      clay_view_set_viewable (CLAY_VIEW (editor->view), NULL);
      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);

      g_clear_object (&editor->image_proxy);
    }

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static gboolean
clay_navigation_editor_button_release (GtkWidget        *widget,
                                       GdkEventButton   *bevent,
                                       ClayDisplayShell *shell)
{
  if (bevent->button == 1)
    {
      gtk_widget_set_visible (shell->nav_popup, FALSE);
    }

  return FALSE;
}

static void
clay_navigation_editor_marker_changed (ClayNavigationView   *view,
                                       gdouble               center_x,
                                       gdouble               center_y,
                                       gdouble               width,
                                       gdouble               height,
                                       ClayNavigationEditor *editor)
{
  ClayViewRenderer *renderer = CLAY_VIEW (editor->view)->renderer;

  if (editor->shell)
    {
      if (clay_display_get_image (editor->shell->display))
        {
          GeglRectangle bounding_box;

          bounding_box = clay_image_proxy_get_bounding_box (
            CLAY_IMAGE_PROXY (renderer->viewable));

          center_x += bounding_box.x;
          center_y += bounding_box.y;

          clay_display_shell_scroll_center_image_xy (editor->shell,
                                                     center_x, center_y);
        }
    }
}

static void
clay_navigation_editor_zoom (ClayNavigationView   *view,
                             ClayZoomType          direction,
                             gdouble               delta,
                             ClayNavigationEditor *editor)
{
  g_return_if_fail (direction != CLAY_ZOOM_TO);

  if (editor->shell)
    {
      if (clay_display_get_image (editor->shell->display))
        clay_display_shell_scale (editor->shell,
                                  direction,
                                  delta,
                                  CLAY_ZOOM_FOCUS_BEST_GUESS);
    }
}

static void
clay_navigation_editor_scroll (ClayNavigationView   *view,
                               GdkEventScroll       *sevent,
                               ClayNavigationEditor *editor)
{
  if (editor->shell)
    {
      gdouble value_x;
      gdouble value_y;

      clay_scroll_adjustment_values (sevent,
                                     editor->shell->hsbdata,
                                     editor->shell->vsbdata,
                                     &value_x, &value_y);

      gtk_adjustment_set_value (editor->shell->hsbdata, value_x);
      gtk_adjustment_set_value (editor->shell->vsbdata, value_y);
    }
}

static gboolean
clay_navigation_editor_zoom_adj_changed_timeout (gpointer data)
{
  ClayNavigationEditor *editor = CLAY_NAVIGATION_EDITOR (data);
  GtkAdjustment        *adj    = editor->zoom_adjustment;

  if (clay_display_get_image (editor->shell->display))
    clay_display_shell_scale (editor->shell,
                              CLAY_ZOOM_TO,
                              pow (2.0, gtk_adjustment_get_value (adj)),
                              CLAY_ZOOM_FOCUS_BEST_GUESS);

  editor->scale_timeout = 0;

  return FALSE;
}

static void
clay_navigation_editor_zoom_adj_changed (GtkAdjustment        *adj,
                                         ClayNavigationEditor *editor)
{
  if (editor->scale_timeout)
    g_source_remove (editor->scale_timeout);

  editor->scale_timeout =
    g_idle_add (clay_navigation_editor_zoom_adj_changed_timeout,
                editor);
}

static void
clay_navigation_editor_shell_infinite_canvas_notify (ClayDisplayShell     *shell,
                                                     const GParamSpec     *pspec,
                                                     ClayNavigationEditor *editor)
{
  clay_navigation_editor_update_marker (editor);

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static void
clay_navigation_editor_shell_scaled (ClayDisplayShell     *shell,
                                     ClayNavigationEditor *editor)
{
  if (editor->zoom_label)
    {
      gchar *str;

      g_object_get (shell->zoom,
                    "percentage", &str,
                    NULL);
      gtk_label_set_text (GTK_LABEL (editor->zoom_label), str);
      g_free (str);
    }

  if (editor->zoom_adjustment)
    {
      gdouble val;

      val = log (clay_zoom_model_get_factor (shell->zoom)) / G_LN2;

      g_signal_handlers_block_by_func (editor->zoom_adjustment,
                                       clay_navigation_editor_zoom_adj_changed,
                                       editor);

      gtk_adjustment_set_value (editor->zoom_adjustment, val);

      g_signal_handlers_unblock_by_func (editor->zoom_adjustment,
                                         clay_navigation_editor_zoom_adj_changed,
                                         editor);
    }

  clay_navigation_editor_update_marker (editor);

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static void
clay_navigation_editor_shell_scrolled (ClayDisplayShell     *shell,
                                       ClayNavigationEditor *editor)
{
  clay_navigation_editor_update_marker (editor);

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static void
clay_navigation_editor_shell_rotated (ClayDisplayShell     *shell,
                                      ClayNavigationEditor *editor)
{
  clay_navigation_editor_update_marker (editor);

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static void
clay_navigation_editor_viewable_size_changed (ClayViewable         *viewable,
                                              ClayNavigationEditor *editor)
{
  clay_navigation_editor_update_marker (editor);

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static void
clay_navigation_editor_options_show_canvas_notify (ClayDisplayOptions   *options,
                                                   const GParamSpec     *pspec,
                                                   ClayNavigationEditor *editor)
{
  clay_navigation_editor_update_marker (editor);
}

static void
clay_navigation_editor_shell_reconnect (ClayDisplayShell     *shell,
                                        ClayNavigationEditor *editor)
{
  ClayImage *image = clay_display_get_image (shell->display);

  g_clear_object (&editor->image_proxy);

  if (image)
    {
      editor->image_proxy = clay_image_proxy_new (image);

      g_signal_connect (
        editor->image_proxy, "size-changed",
        G_CALLBACK (clay_navigation_editor_viewable_size_changed),
        editor);
    }

  clay_view_set_viewable (CLAY_VIEW (editor->view),
                          CLAY_VIEWABLE (editor->image_proxy));

  if (clay_editor_get_ui_manager (CLAY_EDITOR (editor)))
    clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor)),
                            clay_editor_get_popup_data (CLAY_EDITOR (editor)));
}

static void
clay_navigation_editor_update_marker (ClayNavigationEditor *editor)
{
  ClayViewRenderer *renderer = CLAY_VIEW (editor->view)->renderer;
  ClayDisplayShell *shell    = editor->shell;

  if (renderer->dot_for_dot != shell->dot_for_dot)
    clay_view_renderer_set_dot_for_dot (renderer, shell->dot_for_dot);

  if (renderer->viewable)
    {
      ClayNavigationView *view = CLAY_NAVIGATION_VIEW (editor->view);
      ClayImage          *image;
      GeglRectangle       bounding_box;
      gdouble             x, y;
      gdouble             w, h;

      image = clay_image_proxy_get_image (
        CLAY_IMAGE_PROXY (renderer->viewable));

      clay_image_proxy_set_show_all (
        CLAY_IMAGE_PROXY (renderer->viewable),
        clay_display_shell_get_infinite_canvas (shell));

      bounding_box = clay_image_proxy_get_bounding_box (
        CLAY_IMAGE_PROXY (renderer->viewable));

      clay_display_shell_scroll_get_viewport (shell, &x, &y, &w, &h);
      clay_display_shell_untransform_xy_f (shell,
                                           shell->disp_width  / 2,
                                           shell->disp_height / 2,
                                           &x, &y);

      x -= bounding_box.x;
      y -= bounding_box.y;

      clay_navigation_view_set_marker (view,
                                       x, y, w, h,
                                       shell->flip_horizontally,
                                       shell->flip_vertically,
                                       shell->rotate_angle);

      clay_navigation_view_set_canvas (
        view,
        clay_display_shell_get_infinite_canvas (shell) &&
        clay_display_shell_get_show_canvas (shell),
        -bounding_box.x,              -bounding_box.y,
        clay_image_get_width (image), clay_image_get_height (image));
    }
}
