/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools/tools-types.h"

#include "tools/clayrectangleoptions.h"
#include "tools/tool_manager.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-callbacks.h"
#include "display/claydisplayshell-scale.h"
#include "display/claydisplayshell-tool-events.h"
#include "display/claydisplayshell-transform.h"
#include "display/clayimagewindow.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockable.h"
#include "widgets/claydockbook.h"
#include "widgets/claydocked.h"
#include "widgets/claydockwindow.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claytoolbox.h"
#include "widgets/claytooloptionseditor.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"

#include "claycoreapp.h"

#include "clay-app-test-utils.h"
#include "tests.h"


#define CLAY_TEST_IMAGE_WIDTH            150
#define CLAY_TEST_IMAGE_HEIGHT           267

/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define CLAY_PAUSE (g_usleep (2 * 1000 * 1000))

#define ADD_TEST(function) \
  g_test_add ("/clay-tools/" #function, \
              ClayTestFixture, \
              clay, \
              clay_tools_setup_image, \
              function, \
              clay_tools_teardown_image);


typedef struct
{
  int avoid_sizeof_zero;
} ClayTestFixture;


static void               clay_tools_setup_image                         (ClayTestFixture  *fixture,
                                                                          gconstpointer     data);
static void               clay_tools_teardown_image                      (ClayTestFixture  *fixture,
                                                                          gconstpointer     data);
static void               clay_tools_synthesize_image_click_drag_release (ClayDisplayShell *shell,
                                                                          gdouble           start_image_x,
                                                                          gdouble           start_image_y,
                                                                          gdouble           end_image_x,
                                                                          gdouble           end_image_y,
                                                                          gint              button,
                                                                          GdkModifierType   modifiers);
static ClayDisplay      * clay_test_get_only_display                     (Clay             *clay);
static ClayImage        * clay_test_get_only_image                       (Clay             *clay);
static ClayDisplayShell * clay_test_get_only_display_shell               (Clay             *clay);


static void
clay_tools_setup_image (ClayTestFixture *fixture,
                        gconstpointer    data)
{
  Clay *clay = Clay (data);

  clay_test_utils_create_image (clay,
                                CLAY_TEST_IMAGE_WIDTH,
                                CLAY_TEST_IMAGE_HEIGHT);
  clay_test_run_mainloop_until_idle ();
}

static void
clay_tools_teardown_image (ClayTestFixture *fixture,
                           gconstpointer    data)
{
  Clay *clay = Clay (data);

  g_object_unref (clay_test_get_only_image (clay));
  clay_display_close (clay_test_get_only_display (clay));
  clay_test_run_mainloop_until_idle ();
}

/**
 * clay_tools_set_tool:
 * @clay:
 * @tool_id:
 * @display:
 *
 * Makes sure the given tool is the active tool and that the passed
 * display is the focused tool display.
 **/
static void
clay_tools_set_tool (Clay        *clay,
                     const gchar *tool_id,
                     ClayDisplay *display)
{
  /* Activate tool and setup active display for the new tool */
  clay_context_set_tool (clay_get_user_context (clay),
                         clay_get_tool_info (clay, tool_id));
  tool_manager_focus_display_active (clay, display);
}

/**
 * clay_test_get_only_display:
 * @clay:
 *
 * Asserts that there only is one image and display and then
 * returns the display.
 *
 * Returns: The #ClayDisplay.
 **/
static ClayDisplay *
clay_test_get_only_display (Clay *clay)
{
  g_assert_true (g_list_length (clay_get_image_iter (clay)) == 1);
  g_assert_true (g_list_length (clay_get_display_iter (clay)) == 1);

  return CLAY_DISPLAY (clay_get_display_iter (clay)->data);
}

/**
 * clay_test_get_only_display_shell:
 * @clay:
 *
 * Asserts that there only is one image and display shell and then
 * returns the display shell.
 *
 * Returns: The #ClayDisplayShell.
 **/
static ClayDisplayShell *
clay_test_get_only_display_shell (Clay *clay)
{
  return clay_display_get_shell (clay_test_get_only_display (clay));
}

/**
 * clay_test_get_only_image:
 * @clay:
 *
 * Asserts that there is only one image and returns that.
 *
 * Returns: The #ClayImage.
 **/
static ClayImage *
clay_test_get_only_image (Clay *clay)
{
  g_assert_true (g_list_length (clay_get_image_iter (clay)) == 1);
  g_assert_true (g_list_length (clay_get_display_iter (clay)) == 1);

  return CLAY_IMAGE (clay_get_image_iter (clay)->data);
}

static void
clay_test_synthesize_tool_button_event (ClayDisplayShell *shell,
                                       gint              x,
                                       gint              y,
                                       gint              button,
                                       gint              modifiers,
                                       GdkEventType      button_event_type)
{
  GdkEvent   *event   = gdk_event_new (button_event_type);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  g_assert_true (button_event_type == GDK_BUTTON_PRESS ||
                 button_event_type == GDK_BUTTON_RELEASE);

  gdk_event_set_device (event, gdk_seat_get_pointer (seat));

  event->button.window     = g_object_ref (window);
  event->button.send_event = TRUE;
  event->button.time       = gtk_get_current_event_time ();
  event->button.x          = x;
  event->button.y          = y;
  event->button.axes       = NULL;
  event->button.state      = 0;
  event->button.button     = button;
  event->button.x_root     = -1;
  event->button.y_root     = -1;

  clay_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
clay_test_synthesize_tool_motion_event (ClayDisplayShell *shell,
                                        gint              x,
                                        gint              y,
                                        gint              modifiers)
{
  GdkEvent   *event   = gdk_event_new (GDK_MOTION_NOTIFY);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  gdk_event_set_device (event, gdk_seat_get_pointer (seat));

  event->motion.window     = g_object_ref (window);
  event->motion.send_event = TRUE;
  event->motion.time       = gtk_get_current_event_time ();
  event->motion.x          = x;
  event->motion.y          = y;
  event->motion.axes       = NULL;
  event->motion.state      = GDK_BUTTON1_MASK | modifiers;
  event->motion.is_hint    = FALSE;
  event->motion.x_root     = -1;
  event->motion.y_root     = -1;

  clay_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
clay_test_synthesize_tool_crossing_event (ClayDisplayShell *shell,
                                          gint              x,
                                          gint              y,
                                          gint              modifiers,
                                          GdkEventType      crossing_event_type)
{
  GdkEvent   *event   = gdk_event_new (crossing_event_type);
  GdkWindow  *window  = gtk_widget_get_window (GTK_WIDGET (shell->canvas));
  GdkDisplay *display = gdk_window_get_display (window);
  GdkSeat    *seat    = gdk_display_get_default_seat (display);

  g_assert_true (crossing_event_type == GDK_ENTER_NOTIFY ||
                 crossing_event_type == GDK_LEAVE_NOTIFY);

  gdk_event_set_device (event, gdk_seat_get_pointer (seat));

  event->crossing.window     = g_object_ref (window);
  event->crossing.send_event = TRUE;
  event->crossing.subwindow  = NULL;
  event->crossing.time       = gtk_get_current_event_time ();
  event->crossing.x          = x;
  event->crossing.y          = y;
  event->crossing.x_root     = -1;
  event->crossing.y_root     = -1;
  event->crossing.mode       = GDK_CROSSING_NORMAL;
  event->crossing.detail     = GDK_NOTIFY_UNKNOWN;
  event->crossing.focus      = TRUE;
  event->crossing.state      = modifiers;

  clay_display_shell_canvas_tool_events (shell->canvas,
                                         event,
                                         shell);
  gdk_event_free (event);
}

static void
clay_tools_synthesize_image_click_drag_release (ClayDisplayShell *shell,
                                                gdouble           start_image_x,
                                                gdouble           start_image_y,
                                                gdouble           end_image_x,
                                                gdouble           end_image_y,
                                                gint              button /*1..3*/,
                                                GdkModifierType   modifiers)
{
  gdouble start_canvas_x  = -1.0;
  gdouble start_canvas_y  = -1.0;
  gdouble middle_canvas_x = -1.0;
  gdouble middle_canvas_y = -1.0;
  gdouble end_canvas_x    = -1.0;
  gdouble end_canvas_y    = -1.0;

  /* Transform coordinates */
  clay_display_shell_transform_xy_f (shell,
                                     start_image_x,
                                     start_image_y,
                                     &start_canvas_x,
                                     &start_canvas_y);
  clay_display_shell_transform_xy_f (shell,
                                     end_image_x,
                                     end_image_y,
                                     &end_canvas_x,
                                     &end_canvas_y);
  middle_canvas_x = (start_canvas_x + end_canvas_x) / 2;
  middle_canvas_y = (start_canvas_y + end_canvas_y) / 2;

  /* Enter notify */
  clay_test_synthesize_tool_crossing_event (shell,
                                            (int)start_canvas_x,
                                            (int)start_canvas_y,
                                            modifiers,
                                            GDK_ENTER_NOTIFY);

  /* Button press */
  clay_test_synthesize_tool_button_event (shell,
                                          (int)start_canvas_x,
                                          (int)start_canvas_y,
                                          button,
                                          modifiers,
                                          GDK_BUTTON_PRESS);

  /* Move events */
  clay_test_synthesize_tool_motion_event (shell,
                                          (int)start_canvas_x,
                                          (int)start_canvas_y,
                                          modifiers);
  clay_test_synthesize_tool_motion_event (shell,
                                          (int)middle_canvas_x,
                                          (int)middle_canvas_y,
                                          modifiers);
  clay_test_synthesize_tool_motion_event (shell,
                                          (int)end_canvas_x,
                                          (int)end_canvas_y,
                                          modifiers);

  /* Button release */
  clay_test_synthesize_tool_button_event (shell,
                                          (int)end_canvas_x,
                                          (int)end_canvas_y,
                                          button,
                                          modifiers,
                                          GDK_BUTTON_RELEASE);

  /* Leave notify */
  clay_test_synthesize_tool_crossing_event (shell,
                                            (int)start_canvas_x,
                                            (int)start_canvas_y,
                                            modifiers,
                                            GDK_LEAVE_NOTIFY);

  /* Process them */
  clay_test_run_mainloop_until_idle ();
}

/**
 * crop_tool_can_crop:
 * @fixture:
 * @data:
 *
 * Make sure it's possible to crop at all. Regression test for
 * "Bug 315255 - SIGSEGV, while doing a crop".
 **/
static void
crop_tool_can_crop (ClayTestFixture *fixture,
                    gconstpointer    data)
{
  Clay             *clay  = Clay (data);
  ClayImage        *image = clay_test_get_only_image (clay);
  ClayDisplayShell *shell = clay_test_get_only_display_shell (clay);

  gint cropped_x = 10;
  gint cropped_y = 10;
  gint cropped_w = 20;
  gint cropped_h = 30;

  /* Fit display and pause and let it stabilize (two idlings seems to
   * always be enough)
   */
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "view",
                                   "view-shrink-wrap");
  clay_test_run_mainloop_until_idle ();
  clay_test_run_mainloop_until_idle ();

  /* Activate crop tool */
  clay_tools_set_tool (clay, "clay-crop-tool", shell->display);

  /* Do the crop rect */
  clay_tools_synthesize_image_click_drag_release (shell,
                                                  cropped_x,
                                                  cropped_y,
                                                  cropped_x + cropped_w,
                                                  cropped_y + cropped_h,
                                                  1 /*button*/,
                                                  0 /*modifiers*/);

  /* Crop */
  clay_test_utils_synthesize_key_event (GTK_WIDGET (shell), GDK_KEY_Return);
  clay_test_run_mainloop_until_idle ();

  /* Make sure the new image has the expected size */
  g_assert_cmpint (cropped_w, ==, clay_image_get_width (image));
  g_assert_cmpint (cropped_h, ==, clay_image_get_height (image));
}

/**
 * crop_tool_can_crop:
 * @fixture:
 * @data:
 *
 * Make sure it's possible to change width of crop rect in tool
 * options without there being a pending rectangle. Regression test
 * for "Bug 322396 - Crop dimension entering causes crash".
 **/
static void
crop_set_width_without_pending_rect (ClayTestFixture *fixture,
                                     gconstpointer    data)
{
  Clay                 *clay    = Clay (data);
  ClayDisplay          *display = clay_test_get_only_display (clay);
  ClayToolInfo         *tool_info;
  ClayRectangleOptions *rectangle_options;
  GtkWidget            *size_entry;

  /* Activate crop tool */
  clay_tools_set_tool (clay, "clay-crop-tool", display);

  /* Get tool options */
  tool_info         = clay_get_tool_info (clay, "clay-crop-tool");
  rectangle_options = CLAY_RECTANGLE_OPTIONS (tool_info->tool_options);

  /* Find 'Width' or 'Height' GtkTextEntry in tool options */
  size_entry = clay_rectangle_options_get_size_entry (rectangle_options);

  /* Set arbitrary non-0 value */
  clay_size_entry_set_value (CLAY_SIZE_ENTRY (size_entry),
                             0 /*field*/,
                             42.0 /*lower*/);

  /* If we don't crash, everything s fine */
}

int main(int argc, char **argv)
{
  Clay *clay   = NULL;
  gint  result = -1;

  clay_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  clay_test_utils_setup_menus_path ();

  /* Start up Clay */
  clay = clay_init_for_gui_testing (TRUE /*show_gui*/);
  clay_test_run_mainloop_until_idle ();

  /* Add tests */
  ADD_TEST (crop_tool_can_crop);
  ADD_TEST (crop_set_width_without_pending_rect);

  /* Run the tests and return status */
  g_application_run (clay->app, 0, NULL);
  result = clay_core_app_get_exit_status (CLAY_CORE_APP (clay->app));

  g_application_quit (G_APPLICATION (clay->app));
  g_clear_object (&clay->app);

  return result;
}
