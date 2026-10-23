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

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/claycoreconfig.h"
#include "config/claydisplayconfig.h"
#include "config/claydisplayoptions.h"

#include "gegl/clay-babl.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claychannel.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-grid.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-snap.h"
#include "core/claypickable.h"
#include "core/clayprojectable.h"
#include "core/clayprojection.h"
#include "core/claytemplate.h"

#include "menus/menus.h"

#include "widgets/claydevices.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "tools/tool_manager.h"

#include "claycanvas.h"
#include "claycanvascanvasboundary.h"
#include "claycanvaslayerboundary.h"
#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-appearance.h"
#include "claydisplayshell-callbacks.h"
#include "claydisplayshell-cursor.h"
#include "claydisplayshell-dnd.h"
#include "claydisplayshell-expose.h"
#include "claydisplayshell-filter.h"
#include "claydisplayshell-handlers.h"
#include "claydisplayshell-items.h"
#include "claydisplayshell-profile.h"
#include "claydisplayshell-progress.h"
#include "claydisplayshell-render.h"
#include "claydisplayshell-rotate.h"
#include "claydisplayshell-rulers.h"
#include "claydisplayshell-scale.h"
#include "claydisplayshell-scroll.h"
#include "claydisplayshell-scrollbars.h"
#include "claydisplayshell-selection.h"
#include "claydisplayshell-title.h"
#include "claydisplayshell-tool-events.h"
#include "claydisplayshell-transform.h"
#include "clayimagewindow.h"
#include "claymotionbuffer.h"
#include "claystatusbar.h"

#include "about.h"
#include "clay-log.h"
#include "clay-priorities.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_POPUP_MANAGER,
  PROP_INITIAL_MONITOR,
  PROP_DISPLAY,
  PROP_UNIT,
  PROP_TITLE,
  PROP_STATUS,
  PROP_SHOW_ALL,
  PROP_INFINITE_CANVAS
};

enum
{
  SCALED,
  SCROLLED,
  ROTATED,
  RECONNECT,
  LAST_SIGNAL
};

struct _ClayDisplayShellPrivate
{
  GdkMonitor *initial_monitor;
};


typedef struct _ClayDisplayShellOverlay ClayDisplayShellOverlay;

struct _ClayDisplayShellOverlay
{
  ClayDisplayShell *shell;
  gdouble           image_x;
  gdouble           image_y;
  ClayHandleAnchor  anchor;
  gint              spacing_x;
  gint              spacing_y;
};


/*  local function prototypes  */

static void      clay_color_managed_iface_init     (ClayColorManagedInterface *iface);

static void      clay_display_shell_constructed    (GObject          *object);
static void      clay_display_shell_dispose        (GObject          *object);
static void      clay_display_shell_finalize       (GObject          *object);
static void      clay_display_shell_set_property   (GObject          *object,
                                                    guint             property_id,
                                                    const GValue     *value,
                                                    GParamSpec       *pspec);
static void      clay_display_shell_get_property   (GObject          *object,
                                                    guint             property_id,
                                                    GValue           *value,
                                                    GParamSpec       *pspec);

static void      clay_display_shell_style_updated  (GtkWidget        *widget);
static void      clay_display_shell_unrealize      (GtkWidget        *widget);
static void      clay_display_shell_unmap          (GtkWidget        *widget);
static void      clay_display_shell_screen_changed (GtkWidget        *widget,
                                                    GdkScreen        *previous);
static gboolean  clay_display_shell_popup_menu     (GtkWidget        *widget);

static void      clay_display_shell_real_scaled    (ClayDisplayShell *shell);
static void      clay_display_shell_real_scrolled  (ClayDisplayShell *shell);
static void      clay_display_shell_real_rotated   (ClayDisplayShell *shell);

static const guint8 *
                 clay_display_shell_get_icc_profile(ClayColorManaged *managed,
                                                    gsize            *len);
static ClayColorProfile *
               clay_display_shell_get_color_profile(ClayColorManaged *managed);
static void      clay_display_shell_profile_changed(ClayColorManaged *managed);
static void    clay_display_shell_simulation_profile_changed
                                                   (ClayColorManaged *managed);
static void    clay_display_shell_simulation_intent_changed
                                                   (ClayColorManaged *managed);
static void    clay_display_shell_simulation_bpc_changed
                                                   (ClayColorManaged *managed);
static void      clay_display_shell_zoom_button_callback
                                                   (ClayDisplayShell *shell,
                                                    GtkWidget        *zoom_button);
static void      clay_display_shell_sync_config    (ClayDisplayShell  *shell,
                                                    ClayDisplayConfig *config);

static void    clay_display_shell_overlay_allocate (GtkWidget        *child,
                                                    GtkAllocation    *allocation,
                                                    ClayDisplayShellOverlay *overlay);
static void      clay_display_shell_remove_overlay (GtkWidget        *canvas,
                                                    GtkWidget        *child,
                                                    ClayDisplayShell *shell);
static void   clay_display_shell_transform_overlay (ClayDisplayShell *shell,
                                                    GtkWidget        *child,
                                                    gdouble          *x,
                                                    gdouble          *y);
static gboolean clay_display_shell_draw            (ClayDisplayShell *shell,
                                                    cairo_t          *cr,
                                                    gpointer         *data);


G_DEFINE_TYPE_WITH_CODE (ClayDisplayShell, clay_display_shell,
                         GTK_TYPE_EVENT_BOX,
                         G_ADD_PRIVATE (ClayDisplayShell)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROGRESS,
                                                clay_display_shell_progress_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_COLOR_MANAGED,
                                                clay_color_managed_iface_init))


#define parent_class clay_display_shell_parent_class

static guint display_shell_signals[LAST_SIGNAL] = { 0 };


static void
clay_display_shell_class_init (ClayDisplayShellClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  display_shell_signals[SCALED] =
    g_signal_new ("scaled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDisplayShellClass, scaled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  display_shell_signals[SCROLLED] =
    g_signal_new ("scrolled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDisplayShellClass, scrolled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  display_shell_signals[ROTATED] =
    g_signal_new ("rotated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDisplayShellClass, rotated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  display_shell_signals[RECONNECT] =
    g_signal_new ("reconnect",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDisplayShellClass, reconnect),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed        = clay_display_shell_constructed;
  object_class->dispose            = clay_display_shell_dispose;
  object_class->finalize           = clay_display_shell_finalize;
  object_class->set_property       = clay_display_shell_set_property;
  object_class->get_property       = clay_display_shell_get_property;

  widget_class->unrealize          = clay_display_shell_unrealize;
  widget_class->unmap              = clay_display_shell_unmap;
  widget_class->screen_changed     = clay_display_shell_screen_changed;
  widget_class->style_updated      = clay_display_shell_style_updated;
  widget_class->popup_menu         = clay_display_shell_popup_menu;

  klass->scaled                    = clay_display_shell_real_scaled;
  klass->scrolled                  = clay_display_shell_real_scrolled;
  klass->rotated                   = clay_display_shell_real_rotated;
  klass->reconnect                 = NULL;

  g_object_class_install_property (object_class, PROP_POPUP_MANAGER,
                                   g_param_spec_object ("popup-manager",
                                                        NULL, NULL,
                                                        CLAY_TYPE_UI_MANAGER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_MONITOR,
                                   g_param_spec_object ("initial-monitor",
                                                        NULL, NULL,
                                                        GDK_TYPE_MONITOR,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY,
                                   g_param_spec_object ("display", NULL, NULL,
                                                        CLAY_TYPE_DISPLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UNIT,
                                   clay_param_spec_unit ("unit", NULL, NULL,
                                                         TRUE, FALSE,
                                                         clay_unit_pixel (),
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        CLAY_NAME,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_string ("status", NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SHOW_ALL,
                                   g_param_spec_boolean ("show-all",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_INFINITE_CANVAS,
                                   g_param_spec_boolean ("infinite-canvas",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("button-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              GTK_ICON_SIZE_MENU,
                                                              CLAY_PARAM_READABLE));


  gtk_widget_class_set_css_name (widget_class, "ClayDisplayShell");
}

static void
clay_color_managed_iface_init (ClayColorManagedInterface *iface)
{
  iface->get_icc_profile            = clay_display_shell_get_icc_profile;
  iface->get_color_profile          = clay_display_shell_get_color_profile;
  iface->profile_changed            = clay_display_shell_profile_changed;
  iface->simulation_profile_changed = clay_display_shell_simulation_profile_changed;
  iface->simulation_intent_changed  = clay_display_shell_simulation_intent_changed;
  iface->simulation_bpc_changed     = clay_display_shell_simulation_bpc_changed;
}

static void
clay_display_shell_init (ClayDisplayShell *shell)
{
  const gchar *env;

  shell->priv = clay_display_shell_get_instance_private (shell);
  shell->priv->initial_monitor = NULL;

  shell->options            = g_object_new (CLAY_TYPE_DISPLAY_OPTIONS, NULL);
  shell->fullscreen_options = g_object_new (CLAY_TYPE_DISPLAY_OPTIONS_FULLSCREEN, NULL);
  shell->no_image_options   = g_object_new (CLAY_TYPE_DISPLAY_OPTIONS_NO_IMAGE, NULL);

  shell->zoom        = clay_zoom_model_new ();
  shell->dot_for_dot = TRUE;
  shell->scale_x     = 1.0;
  shell->scale_y     = 1.0;
  shell->unit        = clay_unit_pixel ();

  shell->show_image  = TRUE;

  shell->show_all    = FALSE;

  clay_display_shell_items_init (shell);

  shell->cursor_handedness = CLAY_HANDEDNESS_RIGHT;
  shell->current_cursor    = (ClayCursorType) -1;
  shell->tool_cursor       = CLAY_TOOL_CURSOR_NONE;
  shell->cursor_modifier   = CLAY_CURSOR_MODIFIER_NONE;
  shell->override_cursor   = (ClayCursorType) -1;

  shell->filter_format     = babl_format ("R'G'B'A float");
  shell->filter_profile    = clay_babl_get_builtin_color_profile (CLAY_RGB,
                                                                  CLAY_TRC_NON_LINEAR);

  shell->render_scale      = 1;

  shell->render_buf_width  = 256;
  shell->render_buf_height = 256;

  shell->snapped_side_horizontal      = CLAY_ARRANGE_HFILL;
  shell->snapped_layer_horizontal     = NULL;
  shell->snapped_side_vertical        = CLAY_ARRANGE_HFILL;
  shell->snapped_layer_vertical       = NULL;
  shell->equidistance_side_horizontal = CLAY_ARRANGE_HFILL;
  shell->near_layer_horizontal1       = NULL;
  shell->near_layer_horizontal2       = NULL;
  shell->equidistance_side_vertical   = CLAY_ARRANGE_HFILL;
  shell->near_layer_vertical1         = NULL;
  shell->near_layer_vertical2         = NULL;
  shell->drawn                        = FALSE;

  env = g_getenv ("CLAY_DISPLAY_RENDER_BUF_SIZE");

  if (env)
    {
      gint width  = atoi (env);
      gint height = width;

      env = strchr (env, 'x');
      if (env)
        height = atoi (env + 1);

      if (width  > 0 && width  <= 8192 &&
          height > 0 && height <= 8192)
        {
          shell->render_buf_width  = width;
          shell->render_buf_height = height;
        }
    }

  shell->motion_buffer   = clay_motion_buffer_new ();

  g_signal_connect (shell->motion_buffer, "stroke",
                    G_CALLBACK (clay_display_shell_buffer_stroke),
                    shell);
  g_signal_connect (shell->motion_buffer, "hover",
                    G_CALLBACK (clay_display_shell_buffer_hover),
                    shell);

  shell->zoom_focus_point = NULL;

  gtk_widget_set_events (GTK_WIDGET (shell), (GDK_POINTER_MOTION_MASK    |
                                              GDK_BUTTON_PRESS_MASK      |
                                              GDK_KEY_PRESS_MASK         |
                                              GDK_KEY_RELEASE_MASK       |
                                              GDK_FOCUS_CHANGE_MASK      |
                                              GDK_VISIBILITY_NOTIFY_MASK |
                                              GDK_SCROLL_MASK            |
                                              GDK_SMOOTH_SCROLL_MASK));

  /*  zoom model callback  */
  g_signal_connect_swapped (shell->zoom, "zoomed",
                            G_CALLBACK (clay_display_shell_scale_update),
                            shell);

  /*  active display callback  */
  g_signal_connect (shell, "button-press-event",
                    G_CALLBACK (clay_display_shell_events),
                    shell);
  g_signal_connect (shell, "button-release-event",
                    G_CALLBACK (clay_display_shell_events),
                    shell);
  g_signal_connect (shell, "key-press-event",
                    G_CALLBACK (clay_display_shell_events),
                    shell);

  g_signal_connect (shell, "draw",
                    G_CALLBACK (clay_display_shell_draw),
                    NULL);

  clay_help_connect (GTK_WIDGET (shell), NULL, clay_standard_help_func,
                     CLAY_HELP_IMAGE_WINDOW, NULL, NULL);
}

static void
clay_display_shell_constructed (GObject *object)
{
  ClayDisplayShell  *shell = CLAY_DISPLAY_SHELL (object);
  ClayDisplayConfig *config;
  ClayImage         *image;
  GtkWidget         *grid;
  GtkWidget         *gtk_image;
  GtkIconSize        button_icon_size;
  ClayAction        *action;
  gint               image_width;
  gint               image_height;
  gint               shell_width;
  gint               shell_height;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_UI_MANAGER (shell->popup_manager));
  clay_assert (CLAY_IS_DISPLAY (shell->display));

  config = shell->display->config;
  image  = clay_display_get_image (shell->display);

  clay_display_shell_profile_init (shell);

  gtk_widget_style_get (GTK_WIDGET (shell),
                        "button-icon-size", &button_icon_size,
                        NULL);

  if (image)
    {
      image_width  = clay_image_get_width  (image);
      image_height = clay_image_get_height (image);
    }
  else
    {
      /* These values are arbitrary. The width is determined by the
       * menubar and the height is chosen to give a window aspect
       * ratio of roughly 3:1 (as requested by the UI team).
       * It must be smaller than the minimal supported display
       * dimension (1280x720 a.k.a. 720p).
       */
      image_width  = CLAY_DEFAULT_IMAGE_WIDTH;
      image_height = CLAY_DEFAULT_IMAGE_HEIGHT / 4;
    }

  shell->dot_for_dot = config->default_dot_for_dot;

  if (config->monitor_res_from_gdk)
    {
      clay_get_monitor_resolution (shell->priv->initial_monitor,
                                   &shell->monitor_xres, &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = config->monitor_xres;
      shell->monitor_yres = config->monitor_yres;
    }

  /* adjust the initial scale -- so that window fits on screen. */
  if (image)
    {
      clay_display_shell_set_initial_scale (shell,
                                            (gdouble) gtk_widget_get_scale_factor (GTK_WIDGET (shell)),
                                            &shell_width, &shell_height);
    }
  else
    {
      shell_width  = -1;
      shell_height = image_height;
    }

  clay_display_shell_sync_config (shell, config);

  /*  the grid containing everything  */
  grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (shell), grid);
  gtk_widget_set_visible (grid, TRUE);

  /*  the horizontal scrollbar  */
  shell->hsbdata = gtk_adjustment_new (0, 0, image_width,
                                       1, 1, image_width);
  shell->hsb = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, shell->hsbdata);
  gtk_widget_set_can_focus (shell->hsb, FALSE);

  /*  the vertical scrollbar  */
  shell->vsbdata = gtk_adjustment_new (0, 0, image_height,
                                       1, 1, image_height);
  shell->vsb = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, shell->vsbdata);
  gtk_widget_set_can_focus (shell->vsb, FALSE);

  /*  the menu popup button  */
  shell->origin = gtk_event_box_new ();
  gtk_image = gtk_image_new_from_icon_name (CLAY_ICON_MENU_RIGHT,
                                            button_icon_size);
  gtk_container_add (GTK_CONTAINER (shell->origin), gtk_image);
  gtk_widget_set_visible (gtk_image, TRUE);

  g_signal_connect (shell->origin, "button-press-event",
                    G_CALLBACK (clay_display_shell_origin_button_press),
                    shell);

  clay_help_set_help_data (shell->origin,
                           _("Access the image menu"),
                           CLAY_HELP_IMAGE_WINDOW_ORIGIN);

  /*  the canvas  */
  shell->canvas = clay_canvas_new (config);
  gtk_widget_set_size_request (shell->canvas, shell_width, shell_height);
  gtk_container_set_border_width (GTK_CONTAINER (shell->canvas), 10);

  g_signal_connect (shell->canvas, "remove",
                    G_CALLBACK (clay_display_shell_remove_overlay),
                    shell);

  clay_display_shell_dnd_init (shell);
  clay_display_shell_selection_init (shell);

  shell->zoom_gesture = gtk_gesture_zoom_new (GTK_WIDGET (shell->canvas));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (shell->zoom_gesture),
                                              GTK_PHASE_CAPTURE);
  shell->zoom_gesture_active = FALSE;

  shell->rotate_gesture = gtk_gesture_rotate_new (GTK_WIDGET (shell->canvas));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (shell->rotate_gesture),
                                              GTK_PHASE_CAPTURE);
  shell->rotate_gesture_active = FALSE;

  /*  the horizontal ruler  */
  shell->hrule = clay_ruler_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_events (GTK_WIDGET (shell->hrule),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);
  clay_ruler_add_track_widget (CLAY_RULER (shell->hrule), shell->canvas);

  g_signal_connect (shell->hrule, "button-press-event",
                    G_CALLBACK (clay_display_shell_hruler_button_press),
                    shell);

  clay_help_set_help_data (shell->hrule, NULL, CLAY_HELP_IMAGE_WINDOW_RULER);

  /*  the vertical ruler  */
  shell->vrule = clay_ruler_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_events (GTK_WIDGET (shell->vrule),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);
  clay_ruler_add_track_widget (CLAY_RULER (shell->vrule), shell->canvas);

  g_signal_connect (shell->vrule, "button-press-event",
                    G_CALLBACK (clay_display_shell_vruler_button_press),
                    shell);

  clay_help_set_help_data (shell->vrule, NULL, CLAY_HELP_IMAGE_WINDOW_RULER);

  /*  set the rulers as track widgets for each other, so we don't end up
   *  with one ruler wrongly being stuck a few pixels off while we are
   *  hovering the other
   */
  clay_ruler_add_track_widget (CLAY_RULER (shell->hrule), shell->vrule);
  clay_ruler_add_track_widget (CLAY_RULER (shell->vrule), shell->hrule);

  clay_devices_add_widget (shell->display->clay, shell->hrule);
  clay_devices_add_widget (shell->display->clay, shell->vrule);

  g_signal_connect (shell->canvas, "grab-notify",
                    G_CALLBACK (clay_display_shell_canvas_grab_notify),
                    shell);

  clay_widget_set_native_handle (GTK_WIDGET (shell), &shell->window_handle);

  g_signal_connect (shell->canvas, "realize",
                    G_CALLBACK (clay_display_shell_canvas_realize),
                    shell);
  g_signal_connect (shell->canvas, "size-allocate",
                    G_CALLBACK (clay_display_shell_canvas_size_allocate),
                    shell);
  g_signal_connect (shell->canvas, "draw",
                    G_CALLBACK (clay_display_shell_canvas_draw),
                    shell);

  g_signal_connect_object (shell->display->clay->config,
                           "notify::theme",
                           G_CALLBACK (clay_display_shell_style_updated),
                           shell, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (shell->display->clay->config,
                           "notify::override-theme-icon-size",
                           G_CALLBACK (clay_display_shell_style_updated),
                           shell, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_object (shell->display->clay->config,
                           "notify::custom-icon-size",
                           G_CALLBACK (clay_display_shell_style_updated),
                           shell, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  g_signal_connect (shell->canvas, "enter-notify-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "leave-notify-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "proximity-in-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "proximity-out-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "focus-in-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "focus-out-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "button-press-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "button-release-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "scroll-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "motion-notify-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "key-press-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "key-release-event",
                    G_CALLBACK (clay_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->zoom_gesture, "begin",
                    G_CALLBACK (clay_display_shell_zoom_gesture_begin),
                    shell);
  g_signal_connect (shell->zoom_gesture, "update",
                    G_CALLBACK (clay_display_shell_zoom_gesture_update),
                    shell);
  g_signal_connect (shell->zoom_gesture, "end",
                    G_CALLBACK (clay_display_shell_zoom_gesture_end),
                    shell);
  g_signal_connect (shell->rotate_gesture, "begin",
                    G_CALLBACK (clay_display_shell_rotate_gesture_begin),
                    shell);
  g_signal_connect (shell->rotate_gesture, "update",
                    G_CALLBACK (clay_display_shell_rotate_gesture_update),
                    shell);
  g_signal_connect (shell->rotate_gesture, "end",
                    G_CALLBACK (clay_display_shell_rotate_gesture_end),
                    shell);

  /*  the zoom button  */
  shell->zoom_button = g_object_new (GTK_TYPE_CHECK_BUTTON,
                                     "draw-indicator", FALSE,
                                     "relief",         GTK_RELIEF_NONE,
                                     "width-request",  18,
                                     "height-request", 18,
                                     NULL);
  gtk_widget_set_can_focus (shell->zoom_button, FALSE);
  gtk_image = gtk_image_new_from_icon_name (CLAY_ICON_ZOOM_FOLLOW_WINDOW,
                                            button_icon_size);
  gtk_container_add (GTK_CONTAINER (shell->zoom_button), gtk_image);
  gtk_widget_set_visible (gtk_image, TRUE);

  g_signal_connect_swapped (shell->zoom_button, "toggled",
                            G_CALLBACK (clay_display_shell_zoom_button_callback),
                            shell);

  clay_help_set_help_data (shell->zoom_button,
                           _("Zoom image when window size changes"),
                           CLAY_HELP_IMAGE_WINDOW_ZOOM_FOLLOW_BUTTON);

  /*  the quick mask button  */
  shell->quick_mask_button = g_object_new (GTK_TYPE_CHECK_BUTTON,
                                           "draw-indicator", FALSE,
                                           "relief",         GTK_RELIEF_NONE,
                                           "width-request",  18,
                                           "height-request", 18,
                                           NULL);
  gtk_widget_set_can_focus (shell->quick_mask_button, FALSE);
  gtk_image = gtk_image_new_from_icon_name (CLAY_ICON_QUICK_MASK_OFF,
                                            button_icon_size);
  gtk_container_add (GTK_CONTAINER (shell->quick_mask_button), gtk_image);
  gtk_widget_set_visible (gtk_image, TRUE);

  g_signal_connect (shell->quick_mask_button, "toggled",
                    G_CALLBACK (clay_display_shell_quick_mask_toggled),
                    shell);
  g_signal_connect (shell->quick_mask_button, "button-press-event",
                    G_CALLBACK (clay_display_shell_quick_mask_button_press),
                    shell);

  action = clay_ui_manager_find_action (shell->popup_manager,
                                        "quick-mask", "quick-mask-toggle");
  if (action)
    clay_widget_set_accel_help (shell->quick_mask_button, action);
  else
    clay_help_set_help_data (shell->quick_mask_button,
                             _("Toggle Quick Mask"),
                             CLAY_HELP_IMAGE_WINDOW_QUICK_MASK_BUTTON);

  /*  the navigation window button  */
  shell->nav_ebox = gtk_event_box_new ();
  gtk_image = gtk_image_new_from_icon_name (CLAY_ICON_DIALOG_NAVIGATION,
                                            button_icon_size);
  gtk_container_add (GTK_CONTAINER (shell->nav_ebox), gtk_image);
  gtk_widget_set_visible (gtk_image, TRUE);

  g_signal_connect (shell->nav_ebox, "button-press-event",
                    G_CALLBACK (clay_display_shell_navigation_button_press),
                    shell);

  clay_help_set_help_data (shell->nav_ebox,
                           _("Navigate the image display"),
                           CLAY_HELP_IMAGE_WINDOW_NAV_BUTTON);

  /*  the statusbar  */
  shell->statusbar = clay_statusbar_new ();
  clay_statusbar_set_shell (CLAY_STATUSBAR (shell->statusbar), shell);
  clay_help_set_help_data (shell->statusbar, NULL,
                           CLAY_HELP_IMAGE_WINDOW_STATUS_BAR);
  CLAY_STATUSBAR (shell->statusbar)->icon_size = button_icon_size;

  /*  pack all the widgets  */
  gtk_grid_attach (GTK_GRID (grid), shell->origin, 0, 0, 1, 1);

  gtk_widget_set_hexpand (shell->hrule, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->hrule, 1, 0, 1, 1);

  gtk_widget_set_vexpand (shell->vrule, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->vrule, 0, 1, 1, 1);

  gtk_widget_set_hexpand (shell->canvas, TRUE);
  gtk_widget_set_vexpand (shell->canvas, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->canvas, 1, 1, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), shell->zoom_button, 2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), shell->quick_mask_button, 0, 2, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), shell->vsb, 2, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), shell->hsb, 1, 2, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), shell->nav_ebox, 2, 2, 1, 1);

  gtk_widget_set_hexpand (shell->statusbar, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->statusbar, 0, 3, 3, 1);

  /*  show everything that is always shown */
  gtk_widget_set_visible (GTK_WIDGET (shell->canvas), TRUE);

  if (image)
    {
      clay_display_shell_connect (shell);

      /* After connecting to the image we want to center it. Since we
       * not even finished creating the display shell, we can safely
       * assume we will get a size-allocate later.
       */
      shell->size_allocate_center_image = TRUE;
    }
  else
    {
#if 0
      /* Disabled because it sets GDK_POINTER_MOTION_HINT on
       * shell->canvas. For info see Bug 677375
       */
      clay_help_set_help_data (shell->canvas,
                               _("Drop image files here to open them"),
                               NULL);
#endif

      clay_statusbar_empty (CLAY_STATUSBAR (shell->statusbar));
    }

  /* make sure the information is up-to-date */
  clay_display_shell_scale_update (shell);

  clay_display_shell_set_show_all (shell, config->default_show_all);

  /* This was only useful during construction and should not be further used. */
  shell->priv->initial_monitor = NULL;
}

static void
clay_display_shell_dispose (GObject *object)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (object);

  if (shell->display && clay_display_get_shell (shell->display))
    clay_display_shell_disconnect (shell);

  shell->popup_manager = NULL;

  if (shell->selection)
    clay_display_shell_selection_free (shell);

  clay_display_shell_filter_set (shell, NULL);

  if (shell->filter_idle_id)
    {
      g_source_remove (shell->filter_idle_id);
      shell->filter_idle_id = 0;
    }

  g_clear_object (&shell->zoom_gesture);
  g_clear_object (&shell->rotate_gesture);

  g_clear_pointer (&shell->render_cache,       cairo_surface_destroy);
  g_clear_pointer (&shell->render_cache_valid, cairo_region_destroy);

  g_clear_pointer (&shell->render_surface, cairo_surface_destroy);
  g_clear_pointer (&shell->mask_surface,   cairo_surface_destroy);
  g_clear_pointer (&shell->checkerboard,   cairo_pattern_destroy);

  clay_display_shell_profile_finalize (shell);

  g_clear_object (&shell->filter_buffer);
  shell->filter_data   = NULL;
  shell->filter_stride = 0;

  g_clear_object (&shell->mask);

  clay_display_shell_items_free (shell);

  g_clear_object (&shell->motion_buffer);

  if (shell->zoom_focus_point)
    {
      g_slice_free (GdkPoint, shell->zoom_focus_point);
      shell->zoom_focus_point = NULL;
    }

  if (shell->title_idle_id)
    {
      g_source_remove (shell->title_idle_id);
      shell->title_idle_id = 0;
    }

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  g_clear_pointer (&shell->nav_popup, gtk_widget_destroy);

  if (shell->blink_timeout_id)
    {
      g_source_remove (shell->blink_timeout_id);
      shell->blink_timeout_id = 0;
    }

  shell->display = NULL;

  clay_widget_free_native_handle (GTK_WIDGET (shell), &shell->window_handle);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_display_shell_finalize (GObject *object)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (object);

  g_clear_object (&shell->zoom);
  g_clear_pointer (&shell->rotate_transform,   g_free);
  g_clear_pointer (&shell->rotate_untransform, g_free);
  g_clear_object (&shell->options);
  g_clear_object (&shell->fullscreen_options);
  g_clear_object (&shell->no_image_options);
  g_clear_pointer (&shell->title,  g_free);
  g_clear_pointer (&shell->status, g_free);
  g_clear_weak_pointer (&shell->picked_layer);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_display_shell_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (object);

  switch (property_id)
    {
    case PROP_POPUP_MANAGER:
      shell->popup_manager = g_value_get_object (value);
      break;
    case PROP_INITIAL_MONITOR:
      shell->priv->initial_monitor = g_value_get_object (value);
      break;
    case PROP_DISPLAY:
      shell->display = g_value_get_object (value);
      break;
    case PROP_UNIT:
      clay_display_shell_set_unit (shell, g_value_get_object (value));
      break;
    case PROP_TITLE:
      g_set_str (&shell->title, g_value_get_string (value));
      break;
    case PROP_STATUS:
      g_set_str (&shell->status, g_value_get_string (value));
      break;
    case PROP_SHOW_ALL:
      clay_display_shell_set_show_all (shell, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_display_shell_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (object);

  switch (property_id)
    {
    case PROP_POPUP_MANAGER:
      g_value_set_object (value, shell->popup_manager);
      break;
    case PROP_INITIAL_MONITOR:
      g_value_set_object (value, shell->priv->initial_monitor);
      break;
    case PROP_DISPLAY:
      g_value_set_object (value, shell->display);
      break;
    case PROP_UNIT:
      g_value_set_object (value, shell->unit);
      break;
    case PROP_TITLE:
      g_value_set_string (value, shell->title);
      break;
    case PROP_STATUS:
      g_value_set_string (value, shell->status);
      break;
    case PROP_SHOW_ALL:
      g_value_set_boolean (value, shell->show_all);
      break;
    case PROP_INFINITE_CANVAS:
      g_value_set_boolean (value,
                           clay_display_shell_get_infinite_canvas (shell));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_display_shell_style_updated (GtkWidget *widget)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (widget);
  GtkIconSize       icon_size;
  gint              pixel_size;
  GList            *children;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (GTK_WIDGET (shell),
                        "button-icon-size", &icon_size,
                        NULL);
  gtk_icon_size_lookup (icon_size, &pixel_size, NULL);

  if (shell->origin)
    {
      children = gtk_container_get_children (GTK_CONTAINER (shell->origin));
      gtk_image_set_pixel_size (GTK_IMAGE (children->data), pixel_size);
      g_list_free (children);
    }

  if (shell->zoom_button)
    {
      children = gtk_container_get_children (GTK_CONTAINER (shell->zoom_button));
      gtk_image_set_pixel_size (GTK_IMAGE (children->data), pixel_size);
      g_list_free (children);
    }

  if (shell->quick_mask_button)
    {
      children = gtk_container_get_children (GTK_CONTAINER (shell->quick_mask_button));
      gtk_image_set_pixel_size (GTK_IMAGE (children->data), pixel_size);
      g_list_free (children);
    }

  if (shell->nav_ebox)
    {
      children = gtk_container_get_children (GTK_CONTAINER (shell->nav_ebox));
      gtk_image_set_pixel_size (GTK_IMAGE (children->data), pixel_size);
      g_list_free (children);
    }

  if (shell->statusbar)
    {
      ClayStatusbar *statusbar = CLAY_STATUSBAR (shell->statusbar);

      statusbar->icon_size = icon_size;
      GTK_WIDGET_GET_CLASS (statusbar)->style_updated (GTK_WIDGET (statusbar));
    }
}

static void
clay_display_shell_unrealize (GtkWidget *widget)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (widget);

  if (shell->nav_popup)
    gtk_widget_unrealize (shell->nav_popup);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);

  shell->drawn = FALSE;
  g_signal_connect (shell, "draw",
                    G_CALLBACK (clay_display_shell_draw),
                    NULL);
}

static void
clay_display_shell_unmap (GtkWidget *widget)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (widget);

  clay_display_shell_selection_undraw (shell);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
clay_display_shell_screen_changed (GtkWidget *widget,
                                   GdkScreen *previous)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (widget);

  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous);

  if (shell->display->config->monitor_res_from_gdk)
    {
      clay_get_monitor_resolution (clay_widget_get_monitor (widget),
                                   &shell->monitor_xres,
                                   &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = shell->display->config->monitor_xres;
      shell->monitor_yres = shell->display->config->monitor_yres;
    }
}

static gboolean
clay_display_shell_popup_menu (GtkWidget *widget)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (widget);

  clay_context_set_display (clay_get_user_context (shell->display->clay),
                            shell->display);

  clay_ui_manager_ui_popup_at_widget (shell->popup_manager,
                                      "/image-menubar",
                                      NULL, NULL,
                                      shell->origin,
                                      GDK_GRAVITY_EAST,
                                      GDK_GRAVITY_NORTH_WEST,
                                      NULL,
                                      NULL, NULL);

  return TRUE;
}

static void
clay_display_shell_real_scaled (ClayDisplayShell *shell)
{
  ClayContext *user_context;

  if (! shell->display)
    return;

  clay_display_shell_title_update (shell);

  user_context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (user_context))
    {
      clay_display_shell_update_priority_rect (shell);

      clay_ui_manager_update (shell->popup_manager, shell->display);
    }
}

static void
clay_display_shell_real_scrolled (ClayDisplayShell *shell)
{
  ClayContext *user_context;

  if (! shell->display)
    return;

  clay_display_shell_title_update (shell);

  user_context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (user_context))
    {
      clay_display_shell_update_priority_rect (shell);

    }
}

static void
clay_display_shell_real_rotated (ClayDisplayShell *shell)
{
  ClayContext *user_context;

  if (! shell->display)
    return;

  clay_display_shell_title_update (shell);

  user_context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (user_context))
    {
      clay_display_shell_update_priority_rect (shell);

      clay_ui_manager_update (shell->popup_manager, shell->display);
    }
}

static const guint8 *
clay_display_shell_get_icc_profile (ClayColorManaged *managed,
                                    gsize            *len)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (managed);
  ClayImage        *image = clay_display_get_image (shell->display);

  if (image)
    return clay_color_managed_get_icc_profile (CLAY_COLOR_MANAGED (image), len);

  return NULL;
}

static ClayColorProfile *
clay_display_shell_get_color_profile (ClayColorManaged *managed)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (managed);
  ClayImage        *image = clay_display_get_image (shell->display);

  if (image)
    return clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));

  return NULL;
}

static void
clay_display_shell_profile_changed (ClayColorManaged *managed)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (managed);

  clay_display_shell_profile_update (shell);
  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

static void
clay_display_shell_simulation_profile_changed (ClayColorManaged *managed)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (managed);

  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

static void
clay_display_shell_simulation_intent_changed (ClayColorManaged *managed)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (managed);

  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

static void
clay_display_shell_simulation_bpc_changed (ClayColorManaged *managed)
{
  ClayDisplayShell *shell = CLAY_DISPLAY_SHELL (managed);

  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

static void
clay_display_shell_zoom_button_callback (ClayDisplayShell *shell,
                                         GtkWidget        *zoom_button)
{
  shell->zoom_on_resize =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (zoom_button));

  if (shell->zoom_on_resize &&
      clay_display_shell_scale_image_is_within_viewport (shell, NULL, NULL))
    {
      /* Implicitly make a View -> Fit Image in Window */
      clay_display_shell_scale_fit_in (shell);
    }
}

static void
clay_display_shell_sync_config (ClayDisplayShell  *shell,
                                ClayDisplayConfig *config)
{
  clay_config_sync (G_OBJECT (config->default_view),
                    G_OBJECT (shell->options), 0);
  clay_config_sync (G_OBJECT (config->default_fullscreen_view),
                    G_OBJECT (shell->fullscreen_options), 0);
}

static void
clay_display_shell_overlay_allocate (GtkWidget               *child,
                                     GtkAllocation           *allocation,
                                     ClayDisplayShellOverlay *overlay)
{
  GtkRequisition req;
  gdouble        x, y;

  clay_display_shell_transform_overlay (overlay->shell, child, &x, &y);
  gtk_widget_get_preferred_size (child, &req, NULL);

  x = CLAMP (x, 0.0, (gdouble) overlay->shell->disp_width  - req.width);
  y = CLAMP (y, 0.0, (gdouble) overlay->shell->disp_height - req.height);

  clay_overlay_box_set_child_position (CLAY_OVERLAY_BOX (overlay->shell->canvas),
                                       child, x, y);
}

static void
clay_display_shell_remove_overlay (GtkWidget        *canvas,
                                   GtkWidget        *child,
                                   ClayDisplayShell *shell)
{
  ClayDisplayShellOverlay *overlay;

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  g_signal_handlers_disconnect_by_func (child,
                                        clay_display_shell_overlay_allocate,
                                        overlay);

  shell->children = g_list_remove (shell->children, child);
}

static void
clay_display_shell_transform_overlay (ClayDisplayShell *shell,
                                      GtkWidget        *child,
                                      gdouble          *x,
                                      gdouble          *y)
{
  ClayDisplayShellOverlay *overlay;
  GtkRequisition           requisition;

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  clay_display_shell_transform_xy_f (shell,
                                     overlay->image_x,
                                     overlay->image_y,
                                     x, y);

  gtk_widget_get_preferred_size (child, &requisition, NULL);

  switch (overlay->anchor)
    {
    case CLAY_HANDLE_ANCHOR_CENTER:
      *x -= requisition.width  / 2;
      *y -= requisition.height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH:
      *x -= requisition.width / 2;
      *y += overlay->spacing_y;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH_WEST:
      *x += overlay->spacing_x;
      *y += overlay->spacing_y;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y += overlay->spacing_y;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH:
      *x -= requisition.width / 2;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH_WEST:
      *x += overlay->spacing_x;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case CLAY_HANDLE_ANCHOR_WEST:
      *x += overlay->spacing_x;
      *y -= requisition.height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y -= requisition.height / 2;
      break;

    default:
      *x -= overlay->spacing_x;
      *y -= overlay->spacing_y;
      break;
    }
}

static gboolean
clay_display_shell_draw (ClayDisplayShell *shell,
                         cairo_t          *cr,
                         gpointer         *data)
{
  g_signal_handlers_disconnect_by_func (G_OBJECT (shell),
                                        G_CALLBACK (clay_display_shell_draw),
                                        data);

  shell->drawn = TRUE;

  return FALSE;
}

/*  public functions  */

GtkWidget *
clay_display_shell_new (ClayDisplay   *display,
                        ClayUnit      *unit,
                        gdouble        scale,
                        ClayUIManager *popup_manager,
                        GdkMonitor    *monitor)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (CLAY_IS_UI_MANAGER (popup_manager), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return g_object_new (CLAY_TYPE_DISPLAY_SHELL,
                       "popup-manager",   popup_manager,
                       "initial-monitor", monitor,
                       "display",         display,
                       "unit",            unit,
                       NULL);
}

void
clay_display_shell_add_overlay (ClayDisplayShell *shell,
                                GtkWidget        *child,
                                gdouble           image_x,
                                gdouble           image_y,
                                ClayHandleAnchor  anchor,
                                gint              spacing_x,
                                gint              spacing_y)
{
  ClayDisplayShellOverlay *overlay;
  gdouble                  x, y;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (shell));

  overlay = g_new0 (ClayDisplayShellOverlay, 1);

  overlay->shell     = shell;
  overlay->image_x   = image_x;
  overlay->image_y   = image_y;
  overlay->anchor    = anchor;
  overlay->spacing_x = spacing_x;
  overlay->spacing_y = spacing_y;

  g_object_set_data_full (G_OBJECT (child), "image-coords-overlay", overlay,
                          (GDestroyNotify) g_free);

  shell->children = g_list_prepend (shell->children, child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (clay_display_shell_overlay_allocate),
                    overlay);

  clay_display_shell_transform_overlay (shell, child, &x, &y);

  clay_overlay_box_add_child (CLAY_OVERLAY_BOX (shell->canvas), child, 0.0, 0.0);
  clay_overlay_box_set_child_position (CLAY_OVERLAY_BOX (shell->canvas),
                                       child, x, y);
}

void
clay_display_shell_move_overlay (ClayDisplayShell *shell,
                                 GtkWidget        *child,
                                 gdouble           image_x,
                                 gdouble           image_y,
                                 ClayHandleAnchor  anchor,
                                 gint              spacing_x,
                                 gint              spacing_y)
{
  ClayDisplayShellOverlay *overlay;
  gdouble                  x, y;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (shell));

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  g_return_if_fail (overlay != NULL);

  overlay->image_x   = image_x;
  overlay->image_y   = image_y;
  overlay->anchor    = anchor;
  overlay->spacing_x = spacing_x;
  overlay->spacing_y = spacing_y;

  clay_display_shell_transform_overlay (shell, child, &x, &y);

  clay_overlay_box_set_child_position (CLAY_OVERLAY_BOX (shell->canvas),
                                       child, x, y);
}

ClayImageWindow *
clay_display_shell_get_window (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return CLAY_IMAGE_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (shell),
                                                     CLAY_TYPE_IMAGE_WINDOW));
}

ClayStatusbar *
clay_display_shell_get_statusbar (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return CLAY_STATUSBAR (shell->statusbar);
}

ClayColorConfig *
clay_display_shell_get_color_config (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return shell->color_config;
}

void
clay_display_shell_present (ClayDisplayShell *shell)
{
  ClayImageWindow *window;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  window = clay_display_shell_get_window (shell);

  if (window)
    {
      clay_image_window_set_active_shell (window, shell);

      gtk_window_present (GTK_WINDOW (window));

      /* macOS ([GdkQuartzView updateLayer]) have cluncky timing for rendering. This
         fixes: `Gtk-WARNING: Drawing a gadget with negative dimensions`. See: #13203*/
      if (gtk_widget_get_realized (GTK_WIDGET (window)))
        {
          GtkAllocation allocation;

          gtk_widget_get_preferred_size (GTK_WIDGET (window), NULL, NULL);
          gtk_widget_get_allocation (GTK_WIDGET (window), &allocation);
          gtk_widget_size_allocate (GTK_WIDGET (window), &allocation);
        }
    }
}

void
clay_display_shell_reconnect (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_DISPLAY (shell->display));
  g_return_if_fail (clay_display_get_image (shell->display) != NULL);

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  g_signal_emit (shell, display_shell_signals[RECONNECT], 0);

  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (shell));
  clay_display_shell_simulation_profile_changed (CLAY_COLOR_MANAGED (shell));
  clay_display_shell_simulation_intent_changed (CLAY_COLOR_MANAGED (shell));
  clay_display_shell_simulation_bpc_changed (CLAY_COLOR_MANAGED (shell));

  clay_display_shell_scroll_clamp_and_update (shell);

  clay_display_shell_scaled (shell);

  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

static gboolean
clay_display_shell_blink (ClayDisplayShell *shell)
{
  shell->blink_timeout_id = 0;

  if (shell->blink)
    {
      shell->blink = FALSE;
    }
  else
    {
      shell->blink = TRUE;

      shell->blink_timeout_id =
        g_timeout_add (100, (GSourceFunc) clay_display_shell_blink, shell);
    }

  clay_display_shell_expose_full (shell);

  return FALSE;
}

void
clay_display_shell_empty (ClayDisplayShell *shell)
{
  ClayContext     *user_context;
  ClayImageWindow *window;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_DISPLAY (shell->display));
  g_return_if_fail (clay_display_get_image (shell->display) == NULL);

  window = clay_display_shell_get_window (shell);

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  clay_display_shell_selection_undraw (shell);

  clay_display_shell_unset_cursor (shell);

  clay_display_shell_filter_set (shell, NULL);

  clay_display_shell_sync_config (shell, shell->display->config);

  clay_display_shell_appearance_update (shell);
  clay_image_window_update_tabs (window);
#if 0
  clay_help_set_help_data (shell->canvas,
                           _("Drop image files here to open them"), NULL);
#endif

  clay_statusbar_empty (CLAY_STATUSBAR (shell->statusbar));

  shell->flip_horizontally = FALSE;
  shell->flip_vertically   = FALSE;
  shell->rotate_angle      = 0.0;
  clay_display_shell_rotate_update_transform (shell);

  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);

  user_context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (user_context))
    clay_ui_manager_update (shell->popup_manager, shell->display);

  if (clay_widget_animation_enabled ())
    {
      shell->blink_timeout_id =
        g_timeout_add (1403230, (GSourceFunc) clay_display_shell_blink, shell);
    }
}

static gboolean
clay_display_shell_fill_idle (ClayDisplayShell *shell)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  shell->fill_idle_id = 0;

  if (GTK_IS_WINDOW (toplevel))
    {
      clay_display_shell_scale_shrink_wrap (shell, TRUE);

      gtk_window_present (GTK_WINDOW (toplevel));
    }

  return FALSE;
}

void
clay_display_shell_fill (ClayDisplayShell *shell,
                         ClayImage        *image,
                         ClayUnit         *unit,
                         gdouble           scale)
{
  ClayDisplayConfig *config;
  ClayImageWindow   *window;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_DISPLAY (shell->display));
  g_return_if_fail (CLAY_IS_IMAGE (image));

  config = shell->display->config;
  window = clay_display_shell_get_window (shell);

  shell->show_image  = TRUE;

  shell->dot_for_dot = config->default_dot_for_dot;

  clay_display_shell_set_unit (shell, unit);
  clay_display_shell_set_initial_scale (shell, scale, NULL, NULL);
  clay_display_shell_scale_update (shell);

  clay_display_shell_sync_config (shell, config);

  clay_image_window_suspend_keep_pos (window);
  clay_display_shell_appearance_update (shell);
  clay_image_window_resume_keep_pos (window);

  clay_image_window_update_tabs (window);
#if 0
  clay_help_set_help_data (shell->canvas, NULL, NULL);
#endif

  clay_statusbar_fill (CLAY_STATUSBAR (shell->statusbar));

  /* make sure a size-allocate always occurs, even when the rulers and
   * scrollbars are hidden.  see issue #4968.
   */
  shell->size_allocate_center_image = TRUE;
  gtk_widget_queue_resize (GTK_WIDGET (shell->canvas));

  if (shell->blink_timeout_id)
    {
      g_source_remove (shell->blink_timeout_id);
      shell->blink_timeout_id = 0;
    }

  shell->fill_idle_id =
    g_idle_add_full (CLAY_PRIORITY_DISPLAY_SHELL_FILL_IDLE,
                     (GSourceFunc) clay_display_shell_fill_idle, shell,
                     NULL);

  clay_display_shell_set_show_all (shell, config->default_show_all);
}

void
clay_display_shell_scaled (ClayDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  clay_display_shell_rotate_update_transform (shell);

  for (list = shell->children; list; list = g_list_next (list))
    {
      GtkWidget *child = list->data;
      gdouble    x, y;

      clay_display_shell_transform_overlay (shell, child, &x, &y);

      clay_overlay_box_set_child_position (CLAY_OVERLAY_BOX (shell->canvas),
                                           child, x, y);
    }

  g_signal_emit (shell, display_shell_signals[SCALED], 0);
}

void
clay_display_shell_scrolled (ClayDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  clay_display_shell_rotate_update_transform (shell);

  for (list = shell->children; list; list = g_list_next (list))
    {
      GtkWidget *child = list->data;
      gdouble    x, y;

      clay_display_shell_transform_overlay (shell, child, &x, &y);

      clay_overlay_box_set_child_position (CLAY_OVERLAY_BOX (shell->canvas),
                                           child, x, y);
    }

  g_signal_emit (shell, display_shell_signals[SCROLLED], 0);
}

void
clay_display_shell_rotated (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  clay_display_shell_rotate_update_transform (shell);

  g_signal_emit (shell, display_shell_signals[ROTATED], 0);
}

void
clay_display_shell_set_unit (ClayDisplayShell *shell,
                             ClayUnit         *unit)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (shell->unit != unit)
    {
      shell->unit = unit;

      clay_display_shell_rulers_update (shell);

      clay_display_shell_scaled (shell);

      g_object_notify (G_OBJECT (shell), "unit");
    }
}

ClayUnit *
clay_display_shell_get_unit (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), clay_unit_pixel ());

  return shell->unit;
}

gboolean
clay_display_shell_snap_coords (ClayDisplayShell *shell,
                                ClayCoords       *coords,
                                gint              snap_offset_x,
                                gint              snap_offset_y,
                                gint              snap_width,
                                gint              snap_height)
{
  ClayImageSnapData snapping_data =
    {
      CLAY_ARRANGE_HFILL,
      NULL,
      CLAY_ARRANGE_HFILL,
      NULL,
      CLAY_ARRANGE_HFILL,
      NULL,
      NULL,
      CLAY_ARRANGE_HFILL,
      NULL,
      NULL
    };
  ClayImage *image;
  gboolean   snap_to_guides       = FALSE;
  gboolean   snap_to_grid         = FALSE;
  gboolean   snap_to_canvas       = FALSE;
  gboolean   snap_to_path      = FALSE;
  gboolean   snap_to_bbox         = FALSE;
  gboolean   snap_to_equidistance = FALSE;
  gboolean   snapped              = FALSE;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  image = clay_display_get_image (shell->display);

  if (clay_display_shell_get_snap_to_guides (shell) &&
      clay_image_get_guides (image))
    {
      snap_to_guides = TRUE;
    }

  if (clay_display_shell_get_snap_to_grid (shell) &&
      clay_image_get_grid (image))
    {
      snap_to_grid = TRUE;
    }

  snap_to_canvas = clay_display_shell_get_snap_to_canvas (shell);

  if (clay_display_shell_get_snap_to_path (shell) &&
      clay_image_get_selected_paths (image))
    {
      snap_to_path = TRUE;
    }

  if (clay_display_shell_get_snap_to_bbox (shell))
    {
      snap_to_bbox = TRUE;
    }

  shell->snapped_side_horizontal = CLAY_ARRANGE_HFILL;
  shell->snapped_side_vertical = CLAY_ARRANGE_HFILL;

  if (clay_display_shell_get_snap_to_equidistance (shell))
    {
      snap_to_equidistance = TRUE;
    }

  shell->equidistance_side_horizontal = CLAY_ARRANGE_HFILL;
  shell->equidistance_side_vertical = CLAY_ARRANGE_HFILL;

  if (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_path || snap_to_bbox || snap_to_equidistance)
    {
      gint    snap_distance;
      gdouble tx, ty;

      snap_distance = shell->display->config->snap_distance;

      if (snap_width > 0 && snap_height > 0)
        {
          snapped = clay_image_snap_rectangle (image,
                                               &snapping_data,
                                               coords->x + snap_offset_x,
                                               coords->y + snap_offset_y,
                                               coords->x + snap_offset_x +
                                               snap_width,
                                               coords->y + snap_offset_y +
                                               snap_height,
                                               &tx,
                                               &ty,
                                               FUNSCALEX (shell, snap_distance),
                                               FUNSCALEY (shell, snap_distance),
                                               snap_to_guides,
                                               snap_to_grid,
                                               snap_to_canvas,
                                               snap_to_path,
                                               snap_to_bbox,
                                               snap_to_equidistance);

          shell->snapped_side_horizontal = snapping_data.snapped_side_horizontal;
          shell->snapped_layer_horizontal = snapping_data.snapped_layer_horizontal;
          shell->snapped_side_vertical = snapping_data.snapped_side_vertical;
          shell->snapped_layer_vertical = snapping_data.snapped_layer_vertical;
          shell->equidistance_side_horizontal = snapping_data.equidistance_side_horizontal;
          shell->equidistance_side_vertical = snapping_data.equidistance_side_vertical;
          shell->near_layer_horizontal1 = snapping_data.near_layer_horizontal1;
          shell->near_layer_horizontal2 = snapping_data.near_layer_horizontal2;
          shell->near_layer_vertical1 = snapping_data.near_layer_vertical1;
          shell->near_layer_vertical2 = snapping_data.near_layer_vertical2;
        }
      else
        {
          snapped = clay_image_snap_point (image,
                                           coords->x + snap_offset_x,
                                           coords->y + snap_offset_y,
                                           &tx,
                                           &ty,
                                           FUNSCALEX (shell, snap_distance),
                                           FUNSCALEY (shell, snap_distance),
                                           snap_to_guides,
                                           snap_to_grid,
                                           snap_to_canvas,
                                           snap_to_path,
                                           snap_to_bbox,
                                           shell->show_all);
        }

      if (snapped)
        {
          coords->x = tx - snap_offset_x;
          coords->y = ty - snap_offset_y;
        }
    }

  return snapped;
}

gboolean
clay_display_shell_mask_bounds (ClayDisplayShell *shell,
                                gint             *x,
                                gint             *y,
                                gint             *width,
                                gint             *height)
{
  ClayImage *image;
  ClayLayer *layer;
  gint       x1, y1;
  gint       x2, y2;
  gdouble    x1_f, y1_f;
  gdouble    x2_f, y2_f;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  image = clay_display_get_image (shell->display);

  /*  If there is a floating selection, handle things differently  */
  if ((layer = clay_image_get_floating_selection (image)))
    {
      gint fs_x;
      gint fs_y;
      gint fs_width;
      gint fs_height;

      clay_item_get_offset (CLAY_ITEM (layer), &fs_x, &fs_y);
      fs_width  = clay_item_get_width  (CLAY_ITEM (layer));
      fs_height = clay_item_get_height (CLAY_ITEM (layer));

      if (! clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                              x, y, width, height))
        {
          *x      = fs_x;
          *y      = fs_y;
          *width  = fs_width;
          *height = fs_height;
        }
      else
        {
          clay_rectangle_union (*x, *y, *width, *height,
                                fs_x, fs_y, fs_width, fs_height,
                                x, y, width, height);
        }
    }
  else if (! clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                               x, y, width, height))
    {
      return FALSE;
    }

  x1 = *x;
  y1 = *y;
  x2 = *x + *width;
  y2 = *y + *height;

  clay_display_shell_transform_bounds (shell,
                                       x1, y1, x2, y2,
                                       &x1_f, &y1_f, &x2_f, &y2_f);

  /*  Make sure the extents are within bounds  */
  x1 = CLAMP (floor (x1_f), 0, shell->disp_width);
  y1 = CLAMP (floor (y1_f), 0, shell->disp_height);
  x2 = CLAMP (ceil (x2_f),  0, shell->disp_width);
  y2 = CLAMP (ceil (y2_f),  0, shell->disp_height);

  *x      = x1;
  *y      = y1;
  *width  = x2 - x1;
  *height = y2 - y1;

  return (*width > 0) && (*height > 0);
}

void
clay_display_shell_set_show_image (ClayDisplayShell *shell,
                                   gboolean          show_image)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (show_image != shell->show_image)
    {
      shell->show_image = show_image;

      clay_display_shell_expose_full (shell);
    }
}

void
clay_display_shell_set_show_all (ClayDisplayShell *shell,
                                 gboolean          show_all)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (show_all != shell->show_all)
    {
      shell->show_all = show_all;

      if (shell->display && clay_display_get_image (shell->display))
        {
          ClayImage   *image = clay_display_get_image (shell->display);
          ClayContext *user_context;

          if (show_all)
            clay_image_inc_show_all_count (image);
          else
            clay_image_dec_show_all_count (image);

          clay_image_flush (image);

          clay_display_update_bounding_box (shell->display);

          clay_display_shell_update_show_canvas (shell);

          clay_display_shell_scroll_clamp_and_update (shell);
          clay_display_shell_scrollbars_update (shell);

          clay_display_shell_expose_full (shell);

          user_context = clay_get_user_context (shell->display->clay);

          if (shell->display == clay_context_get_display (user_context))
            {
              clay_display_shell_update_priority_rect (shell);

              clay_ui_manager_update (shell->popup_manager, shell->display);
            }
        }

      g_object_notify (G_OBJECT (shell), "show-all");
      g_object_notify (G_OBJECT (shell), "infinite-canvas");
    }
}


ClayPickable *
clay_display_shell_get_pickable (ClayDisplayShell *shell)
{
  ClayImage *image;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  image = clay_display_get_image (shell->display);

  if (image)
    {
      if (! shell->show_all)
        return CLAY_PICKABLE (image);
      else
        return CLAY_PICKABLE (clay_image_get_projection (image));
    }

  return NULL;
}

ClayPickable *
clay_display_shell_get_canvas_pickable (ClayDisplayShell *shell)
{
  ClayImage *image;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  image = clay_display_get_image (shell->display);

  if (image)
    {
      if (! clay_display_shell_get_infinite_canvas (shell))
        return CLAY_PICKABLE (image);
      else
        return CLAY_PICKABLE (clay_image_get_projection (image));
    }

  return NULL;
}

GeglRectangle
clay_display_shell_get_bounding_box (ClayDisplayShell *shell)
{
  GeglRectangle  bounding_box = {};
  ClayImage     *image;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), bounding_box);

  image = clay_display_get_image (shell->display);

  if (image)
    {
      if (! shell->show_all)
        {
          bounding_box.width  = clay_image_get_width  (image);
          bounding_box.height = clay_image_get_height (image);
        }
      else
        {
          bounding_box = clay_projectable_get_bounding_box (
            CLAY_PROJECTABLE (image));
        }
    }

  return bounding_box;
}

gboolean
clay_display_shell_get_infinite_canvas (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->show_all &&
         ! clay_display_shell_get_padding_in_show_all (shell);
}

void
clay_display_shell_update_priority_rect (ClayDisplayShell *shell)
{
  ClayImage *image;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  image = clay_display_get_image (shell->display);

  if (image)
    {
      ClayProjection *projection = clay_image_get_projection (image);
      gint            x, y;
      gint            width, height;

      clay_display_shell_untransform_viewport (shell, ! shell->show_all,
                                               &x, &y, &width, &height);
      clay_projection_set_priority_rect (projection, x, y, width, height);
    }
}

void
clay_display_shell_flush (ClayDisplayShell *shell)
{
  ClayImageWindow *window;
  ClayContext     *context;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  window = clay_display_shell_get_window (shell);

  clay_display_shell_title_update (shell);

  clay_canvas_layer_boundary_set_layers (CLAY_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                         clay_image_get_selected_layers (clay_display_get_image (shell->display)));

  clay_canvas_canvas_boundary_set_image (CLAY_CANVAS_CANVAS_BOUNDARY (shell->canvas_boundary),
                                         clay_display_get_image (shell->display));

  context = clay_get_user_context (shell->display->clay);

  if (shell->display == clay_context_get_display (context))
    {
      if (window && clay_image_window_get_active_shell (window) == shell)
        {
          /* Only update the main actions for the active shell of the
           * active image window.
           */
          ClayUIManager *manager = menus_get_image_manager_singleton (shell->display->clay);

          clay_ui_manager_update (manager, shell->display);
        }

      clay_ui_manager_update (shell->popup_manager, shell->display);
    }
}

/**
 * clay_display_shell_pause:
 * @shell: a display shell
 *
 * This function increments the pause count or the display shell.
 * If it was zero coming in, then the function pauses the active tool,
 * so that operations on the display can take place without corrupting
 * anything that the tool has drawn.  It "undraws" the current tool
 * drawing, and must be followed by clay_display_shell_resume() after
 * the operation in question is completed.
 **/
void
clay_display_shell_pause (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  shell->paused_count++;

  if (shell->paused_count == 1)
    {
      /*  pause the currently active tool  */
      tool_manager_control_active (shell->display->clay,
                                   CLAY_TOOL_ACTION_PAUSE,
                                   shell->display);
    }
}

/**
 * clay_display_shell_resume:
 * @shell: a display shell
 *
 * This function decrements the pause count for the display shell.
 * If this brings it to zero, then the current tool is resumed.
 * It is an error to call this function without having previously
 * called clay_display_shell_pause().
 **/
void
clay_display_shell_resume (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->paused_count > 0);

  shell->paused_count--;

  if (shell->paused_count == 0)
    {
      /* start the currently active tool */
      tool_manager_control_active (shell->display->clay,
                                   CLAY_TOOL_ACTION_RESUME,
                                   shell->display);
    }
}

/**
 * clay_display_shell_set_highlight:
 * @shell:     a #ClayDisplayShell
 * @highlight: a rectangle in image coordinates that should be brought out
 * @opacity:   how much to hide the unselected area
 *
 * This function sets an area of the image that should be
 * accentuated. The actual implementation is to dim all pixels outside
 * this rectangle. Passing %NULL for @highlight unsets the rectangle.
 **/
void
clay_display_shell_set_highlight (ClayDisplayShell   *shell,
                                  const GdkRectangle *highlight,
                                  gdouble             opacity)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (highlight)
    {
      clay_canvas_item_begin_change (shell->passe_partout);

      clay_canvas_rectangle_set (shell->passe_partout,
                                 highlight->x,
                                 highlight->y,
                                 highlight->width,
                                 highlight->height);
      g_object_set (shell->passe_partout, "opacity", opacity, NULL);

      clay_canvas_item_set_visible (shell->passe_partout, TRUE);

      clay_canvas_item_end_change (shell->passe_partout);
    }
  else
    {
      clay_canvas_item_set_visible (shell->passe_partout, FALSE);
    }
}

/**
 * clay_display_shell_set_mask:
 * @shell: a #ClayDisplayShell
 * @mask:  a #ClayDrawable (1 byte per pixel)
 * @color: the color to use for drawing the mask
 * @inverted: %TRUE if the mask should be drawn inverted
 *
 * Previews a mask originating at offset_x, offset_x. Depending on
 * @inverted, pixels that are selected or not selected are tinted with
 * the given color.
 **/
void
clay_display_shell_set_mask (ClayDisplayShell *shell,
                             GeglBuffer       *mask,
                             gint              offset_x,
                             gint              offset_y,
                             GeglColor        *color,
                             gboolean          inverted)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (mask == NULL || GEGL_IS_BUFFER (mask));
  g_return_if_fail (mask == NULL || GEGL_IS_COLOR (color));

  if (mask)
    g_object_ref (mask);

  if (shell->mask)
    g_object_unref (shell->mask);

  shell->mask = mask;

  shell->mask_offset_x = offset_x;
  shell->mask_offset_y = offset_y;

  g_clear_object (&shell->mask_color);
  if (mask)
    shell->mask_color = gegl_color_duplicate (color);

  shell->mask_inverted = inverted;

  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

/**
 * clay_display_shell_is_drawn:
 * @shell:
 *
 * This should be run when you need to verify that the shell or its
 * display were actually drawn. gtk_widget_is_visible(),
 * gtk_widget_get_mapped() or gtk_widget_get_realized() are not enough
 * because they may all return TRUE before the window is actually on
 * screen.
 *
 * Returns: whether @shell was actually drawn on screen, i.e. the "draw"
 * signal has run.
 */
gboolean
clay_display_shell_is_drawn (ClayDisplayShell *shell)
{
  return shell->drawn;
}

/**
 * clay_display_shell_get_overlay_corners:
 * @shell:          a #ClayDisplayShell
 * @child:          a child widget of the shell
 * @image_x:        the x coordinate in image coordinates
 * @image_y:        the y coordinate in image coordinates
 * @top_left_x:     return location for the top left x coordinate in image coordinates
 * @top_left_y:     return location for the top left y coordinate in image coordinates
 * @bottom_right_x: return location for the bottom right x coordinate in image coordinates
 * @bottom_right_y: return location for the bottom right y coordinate in image coordinates
 *
 * This function calculates the corners of an overlay widget in image coordinates.
 **/
void
clay_display_shell_get_overlay_corners (ClayDisplayShell *shell,
                                        GtkWidget        *child,
                                        gdouble           image_x,
                                        gdouble           image_y,
                                        gdouble          *top_left_x,
                                        gdouble          *top_left_y,
                                        gdouble          *bottom_right_x,
                                        gdouble          *bottom_right_y)
{
  ClayDisplayShellOverlay *overlay;
  GtkRequisition           req;
  gdouble                  tl_disp_x, tl_disp_y;
  gdouble                  br_disp_x, br_disp_y;
  gdouble                  ix1, iy1;
  gdouble                  ix2, iy2;
  gdouble                  ix3, iy3;
  gdouble                  ix4, iy4;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  clay_display_shell_transform_xy_f (shell,
                                     image_x, image_y,
                                     &tl_disp_x, &tl_disp_y);

  gtk_widget_get_preferred_size (child, &req, NULL);

  switch (overlay->anchor)
    {
    case CLAY_HANDLE_ANCHOR_CENTER:
      tl_disp_x -= req.width  / 2;
      tl_disp_y -= req.height / 2;
      break;
    case CLAY_HANDLE_ANCHOR_NORTH:
      tl_disp_x -= req.width  / 2;
      tl_disp_y += overlay->spacing_y;
      break;
    case CLAY_HANDLE_ANCHOR_NORTH_WEST:
      tl_disp_x += overlay->spacing_x;
      tl_disp_y += overlay->spacing_y;
      break;
    case CLAY_HANDLE_ANCHOR_NORTH_EAST:
      tl_disp_x -= req.width + overlay->spacing_x;
      tl_disp_y += overlay->spacing_y;
      break;
    case CLAY_HANDLE_ANCHOR_SOUTH:
      tl_disp_x -= req.width  / 2;
      tl_disp_y -= req.height + overlay->spacing_y;
      break;
    case CLAY_HANDLE_ANCHOR_SOUTH_WEST:
      tl_disp_x += overlay->spacing_x;
      tl_disp_y -= req.height + overlay->spacing_y;
      break;
    case CLAY_HANDLE_ANCHOR_SOUTH_EAST:
      tl_disp_x -= req.width + overlay->spacing_x;
      tl_disp_y -= req.height + overlay->spacing_y;
      break;
    case CLAY_HANDLE_ANCHOR_WEST:
      tl_disp_x += overlay->spacing_x;
      tl_disp_y -= req.height / 2;
      break;
    case CLAY_HANDLE_ANCHOR_EAST:
      tl_disp_x -= req.width + overlay->spacing_x;
      tl_disp_y -= req.height / 2;
      break;
    default:
      tl_disp_x -= overlay->spacing_x;
      tl_disp_y -= overlay->spacing_y;
      break;
    }

  br_disp_x = tl_disp_x + req.width;
  br_disp_y = tl_disp_y + req.height;

  clay_display_shell_untransform_xy_f (shell,
                                       tl_disp_x, tl_disp_y,
                                       &ix1, &iy1);
  clay_display_shell_untransform_xy_f (shell,
                                       br_disp_x, tl_disp_y,
                                       &ix2, &iy2);
  clay_display_shell_untransform_xy_f (shell,
                                       tl_disp_x, br_disp_y,
                                       &ix3, &iy3);
  clay_display_shell_untransform_xy_f (shell,
                                       br_disp_x, br_disp_y,
                                       &ix4, &iy4);

  *top_left_x     = MIN (MIN (ix1, ix2), MIN (ix3, ix4));
  *top_left_y     = MIN (MIN (iy1, iy2), MIN (iy3, iy4));
  *bottom_right_x = MAX (MAX (ix1, ix2), MAX (ix3, ix4));
  *bottom_right_y = MAX (MAX (iy1, iy2), MAX (iy3, iy4));
}
