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
#include "tools/tool_manager.h"

#include "dialogs/dialogs-types.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-scale.h"
#include "display/claydisplayshell-transform.h"
#include "display/clayimagewindow.h"

#include "menus/menus.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockable.h"
#include "widgets/claydockbook.h"
#include "widgets/claydockcontainer.h"
#include "widgets/claydocked.h"
#include "widgets/claydockwindow.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claysessioninfo-aux.h"
#include "widgets/claysessionmanaged.h"
#include "widgets/claytoolbox.h"
#include "widgets/claytooloptionseditor.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayer-new.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"

#include "claycoreapp.h"

#include "clay-app-test-utils.h"
#include "tests.h"


#define CLAY_UI_WINDOW_POSITION_EPSILON 30
#define CLAY_UI_POSITION_EPSILON        1
#define CLAY_UI_ZOOM_EPSILON            0.01

#define ADD_TEST(function) \
  g_test_add_data_func ("/clay-ui/" #function, clay, function);


/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define CLAY_PAUSE (g_usleep (20 * 1000 * 1000))


typedef gboolean (*ClayUiTestFunc) (GObject *object);


static void            clay_ui_synthesize_delete_event          (GtkWidget         *widget);
static GtkWidget     * clay_ui_find_window                      (ClayDialogFactory *dialog_factory,
                                                                 ClayUiTestFunc     predicate);
static gboolean        clay_ui_not_toolbox_window               (GObject           *object);
static gboolean        clay_ui_multicolumn_not_toolbox_window   (GObject           *object);
static void            clay_ui_switch_window_mode               (Clay              *clay);


/**
 * tool_options_editor_updates:
 * @data:
 *
 * Makes sure that the tool options editor is updated when the tool
 * changes.
 **/
static void
tool_options_editor_updates (gconstpointer data)
{
  Clay                  *clay         = Clay (data);
  ClayDisplay           *display      = CLAY_DISPLAY (clay_get_empty_display (clay));
  ClayDisplayShell      *shell        = clay_display_get_shell (display);
  GtkWidget             *toplevel     = gtk_widget_get_toplevel (GTK_WIDGET (shell));
  ClayUIManager         *ui_manager   = menus_get_image_manager_singleton (clay);
  GtkWidget             *dockable     = clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                                                        clay_widget_get_monitor (toplevel),
                                                                        NULL /*ui_manager*/,
                                                                        toplevel,
                                                                        "clay-tool-options",
                                                                        -1 /*view_size*/,
                                                                        FALSE /*present*/);
  ClayToolOptionsEditor *editor       = CLAY_TOOL_OPTIONS_EDITOR (gtk_bin_get_child (GTK_BIN (dockable)));

  /* First select the rect select tool */
  clay_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-rect-select");
  g_assert_cmpstr (CLAY_HELP_TOOL_RECT_SELECT,
                   ==,
                   clay_tool_options_editor_get_tool_options (editor)->
                   tool_info->help_id);

  /* Change tool and make sure the change is taken into account by the
   * tool options editor
   */
  clay_ui_manager_activate_action (ui_manager,
                                   "tools",
                                   "tools-ellipse-select");
  g_assert_cmpstr (CLAY_HELP_TOOL_ELLIPSE_SELECT,
                   ==,
                   clay_tool_options_editor_get_tool_options (editor)->
                   tool_info->help_id);
}

static void
create_new_image_via_dialog (gconstpointer data)
{
  Clay      *clay = Clay (data);
  ClayImage *image;
  ClayLayer *layer;

  image = clay_test_utils_create_image_from_dialog (clay);

  /* Add a layer to the image to make it more useful in later tests */
  layer = clay_layer_new (image,
                          clay_image_get_width (image),
                          clay_image_get_height (image),
                          clay_image_get_layer_format (image, TRUE),
                          "Layer for testing",
                          CLAY_OPACITY_OPAQUE,
                          CLAY_LAYER_MODE_NORMAL);

  clay_image_add_layer (image, layer,
                        CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_test_run_mainloop_until_idle ();
}

static void
keyboard_zoom_focus (gconstpointer data)
{
  Clay              *clay    = Clay (data);
  ClayDisplay       *display = CLAY_DISPLAY (clay_get_display_iter (clay)->data);
  ClayDisplayShell  *shell   = clay_display_get_shell (display);
  gint               image_x;
  gint               image_y;
  gint               shell_x_before_zoom;
  gint               shell_y_before_zoom;
  gdouble            factor_before_zoom;
  gint               shell_x_after_zoom;
  gint               shell_y_after_zoom;
  gdouble            factor_after_zoom;

  /* We need to use a point that is within the visible (exposed) part
   * of the canvas
   */
  image_x = 400;
  image_y = 50;

  /* Setup zoom focus on the bottom right part of the image. We avoid
   * 0,0 because that's essentially a particularly easy special case.
   */
  clay_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_before_zoom,
                                   &shell_y_before_zoom);
  clay_display_shell_push_zoom_focus_pointer_pos (shell,
                                                  shell_x_before_zoom,
                                                  shell_y_before_zoom);
  factor_before_zoom = clay_zoom_model_get_factor (shell->zoom);

  /* Do the zoom */
  clay_test_utils_synthesize_key_event (GTK_WIDGET (shell), GDK_KEY_plus);
  clay_test_run_mainloop_until_idle ();

  /* Make sure the zoom focus point remained fixed */
  clay_display_shell_transform_xy (shell,
                                   image_x,
                                   image_y,
                                   &shell_x_after_zoom,
                                   &shell_y_after_zoom);
  factor_after_zoom = clay_zoom_model_get_factor (shell->zoom);

  /* First of all make sure a zoom happened at all. If this assert
   * fails, it means that the zoom didn't happen. Possible causes:
   *
   *  * gdk_test_simulate_key() failed to map 'GDK_KEY_plus' to the proper
   *    'plus' X keysym, probably because it is mapped to a keycode
   *    with modifiers like 'shift'. Run "xmodmap -pk | grep plus" to
   *    find out. Make sure 'plus' is the first keysym for the given
   *    keycode. If not, use "xmodmap <keycode> = plus" to correct it.
   */
  g_assert_cmpfloat (fabs (factor_before_zoom - factor_after_zoom),
                     >=,
                     CLAY_UI_ZOOM_EPSILON);
}

static void
restore_recently_closed_multi_column_dock (gconstpointer data)
{
  Clay      *clay                          = Clay (data);
  GtkWidget *dock_window                   = NULL;
  gint       n_session_infos_before_close  = -1;
  gint       n_session_infos_after_close   = -1;
  gint       n_session_infos_after_restore = -1;
  GList     *session_infos                 = NULL;

  /* Find a non-toolbox dock window */
  dock_window = clay_ui_find_window (clay_dialog_factory_get_singleton (),
                                     clay_ui_multicolumn_not_toolbox_window);
  g_assert_true (dock_window != NULL);

  /* Count number of docks */
  session_infos = clay_dialog_factory_get_session_infos (clay_dialog_factory_get_singleton ());
  n_session_infos_before_close = g_list_length (session_infos);

  /* Close one of the dock windows */
  clay_ui_synthesize_delete_event (GTK_WIDGET (dock_window));
  clay_test_run_mainloop_until_idle ();

  /* Make sure the number of session infos went down */
  session_infos = clay_dialog_factory_get_session_infos (clay_dialog_factory_get_singleton ());
  n_session_infos_after_close = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_before_close,
                   >,
                   n_session_infos_after_close);

  /* Restore the (only available) closed dock and make sure the session
   * infos in the global dock factory are increased again
   */
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "windows",
                                   /* FIXME: This is severely hardcoded */
                                   "windows-recent-0003");
  clay_test_run_mainloop_until_idle ();
  session_infos = clay_dialog_factory_get_session_infos (clay_dialog_factory_get_singleton ());
  n_session_infos_after_restore = g_list_length (session_infos);
  g_assert_cmpint (n_session_infos_after_close,
                   <,
                   n_session_infos_after_restore);
}

/**
 * tab_toggle_dont_change_dock_window_position:
 * @data:
 *
 * Makes sure that when dock windows are hidden with Tab and shown
 * again, their positions and sizes are not changed. We don't really
 * use Tab though, we only simulate its effect.
 **/
static void
tab_toggle_dont_change_dock_window_position (gconstpointer data)
{
  Clay      *clay          = Clay (data);
  GtkWidget *dock_window   = NULL;
  gint       x_before_hide = -1;
  gint       y_before_hide = -1;
  gint       w_before_hide = -1;
  gint       h_before_hide = -1;
  gint       x_after_show  = -1;
  gint       y_after_show  = -1;
  gint       w_after_show  = -1;
  gint       h_after_show  = -1;

  /* Find a non-toolbox dock window */
  dock_window = clay_ui_find_window (clay_dialog_factory_get_singleton (),
                                     clay_ui_not_toolbox_window);
  g_assert_true (dock_window != NULL);
  g_assert_true (gtk_widget_get_visible (dock_window));

  /* Get the position and size */
  clay_test_run_mainloop_until_idle ();
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_before_hide,
                           &y_before_hide);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_before_hide,
                       &h_before_hide);

  /* Hide all dock windows */
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "windows",
                                   "windows-hide-docks");
  clay_test_run_mainloop_until_idle ();
  g_assert_true (! gtk_widget_get_visible (dock_window));

  /* Show them again */
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "windows",
                                   "windows-hide-docks");
  clay_test_run_mainloop_until_idle ();
  g_assert_true (gtk_widget_get_visible (dock_window));

  /* Get the position and size again and make sure it's the same as
   * before
   */
  gtk_window_get_position (GTK_WINDOW (dock_window),
                           &x_after_show,
                           &y_after_show);
  gtk_window_get_size (GTK_WINDOW (dock_window),
                       &w_after_show,
                       &h_after_show);
  g_assert_cmpint ((int)abs (x_before_hide - x_after_show), <=, CLAY_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (y_before_hide - y_after_show), <=, CLAY_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (w_before_hide - w_after_show), <=, CLAY_UI_WINDOW_POSITION_EPSILON);
  g_assert_cmpint ((int)abs (h_before_hide - h_after_show), <=, CLAY_UI_WINDOW_POSITION_EPSILON);
}

static void
switch_to_single_window_mode (gconstpointer data)
{
  Clay *clay = Clay (data);

  /* Switch to single-window mode. We consider this test as passed if
   * we don't get any GLib warnings/errors
   */
  clay_ui_switch_window_mode (clay);
}

static void
switch_back_to_multi_window_mode (gconstpointer data)
{
  Clay *clay = Clay (data);

  /* Switch back to multi-window mode. We consider this test as passed
   * if we don't get any GLib warnings/errors
   */
  clay_ui_switch_window_mode (clay);
}

static void
close_image (gconstpointer data)
{
  Clay *clay       = Clay (data);
  int   undo_count = 4;

  /* Undo all changes so we don't need to find the 'Do you want to
   * save?'-dialog and its 'No' button
   */
  while (undo_count--)
    {
      clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                       "edit",
                                       "edit-undo");
      clay_test_run_mainloop_until_idle ();
    }

  /* Close the image */
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "view",
                                   "view-close");
  clay_test_run_mainloop_until_idle ();

  /* Did it really disappear? */
  g_assert_cmpint (g_list_length (clay_get_image_iter (clay)), ==, 0);
}

/**
 * window_roles:
 * @data:
 *
 * Makes sure that different windows have the right roles specified.
 **/
static void
window_roles (gconstpointer data)
{
  Clay              *clay           = Clay (data);
  ClayDisplay       *display        = CLAY_DISPLAY (clay_get_display_iter (clay)->data);
  ClayDisplayShell  *shell          = clay_display_get_shell (display);
  ClayImageWindow   *window         = clay_display_shell_get_window (shell);
  GdkMonitor        *monitor        = NULL;
  GtkWidget         *dock           = NULL;
  GtkWidget         *toolbox        = NULL;
  ClayDockWindow    *dock_window    = NULL;
  ClayDockWindow    *toolbox_window = NULL;

  monitor        = gdk_display_get_monitor_at_window(gdk_display_get_default (), gtk_widget_get_window(GTK_WIDGET(window)));

  dock           = clay_dock_with_window_new (clay_dialog_factory_get_singleton (),
                                              monitor,
                                              FALSE /*toolbox*/);
  toolbox        = clay_dock_with_window_new (clay_dialog_factory_get_singleton (),
                                              monitor,
                                              TRUE /*toolbox*/);
  dock_window    = clay_dock_window_from_dock (CLAY_DOCK (dock));
  toolbox_window = clay_dock_window_from_dock (CLAY_DOCK (toolbox));

  g_assert_cmpint (g_str_has_prefix (gtk_window_get_role (GTK_WINDOW (dock_window)), "clay-dock-"), ==,
                   TRUE);
  g_assert_cmpint (g_str_has_prefix (gtk_window_get_role (GTK_WINDOW (toolbox_window)), "clay-toolbox-"), ==,
                   TRUE);

  /* When we get here we have a ref count of one, but the signals we
   * emit cause the reference count to become less than zero for some
   * reason. So we're lazy and simply ignore to unref these
  g_object_unref (toolbox);
  g_object_unref (dock);
   */
}

static void
paintbrush_is_standard_tool (gconstpointer data)
{
  Clay         *clay         = Clay (data);
  ClayContext  *user_context = clay_get_user_context (clay);
  ClayToolInfo *tool_info    = clay_context_get_tool (user_context);

  g_assert_cmpstr (tool_info->help_id,
                   ==,
                   "clay-tool-paintbrush");
}


/**
 * swap_tools:
 * @data:
 *
 * Testing switchback to last used tool <shift>X. Tools are taken from clay->tool_info_list.
 * Not all possible combinations of available tools are tested though.
 **/
static void
swap_tools (gconstpointer data)
{
  Clay         *clay                  = Clay (data);
  ClayContext  *user_context          = clay_get_user_context (clay);
  ClayToolInfo *first_tool_info       = NULL;
  ClayToolInfo *second_tool_info      = NULL;
  GList        *tool_info_list        = NULL;
  guint         i, j                  = 0;

  /* Arbitrarily testing only the first 10 available tools from clay->tool_info_list
   * because there are tools which aren't settable within the reduced framework of
   * clay_init_for_gui_testing's environment!
   */
  guint         tool_info_list_length = 10;  /* Value should actually be g_list_length (tool_info_list) */

  tool_info_list = clay_get_tool_info_iter (clay);

  for (i = 0; i < tool_info_list_length; i++)
    {
      /* Getting the first reference tool */
      first_tool_info = g_list_nth_data (tool_info_list, i);

      for (j = 0; j < tool_info_list_length; j++)
        {
          /* Now get the second tool. Set both one after another and loop the second tool through
           * all available tools to test switchback.
           */
          second_tool_info = g_list_nth_data (tool_info_list, j);

          if (first_tool_info != second_tool_info)
            {
              clay_context_set_tool (user_context, first_tool_info);
              g_assert (clay_context_get_tool (user_context) == first_tool_info);

              clay_context_set_tool (user_context, second_tool_info);
              g_assert (clay_context_get_tool (user_context) == second_tool_info);

              tool_manager_swap_tools (clay, FALSE);
              g_assert (clay_context_get_tool (user_context) == first_tool_info);
            }
        }
    }
}

/**
 * clay_ui_synthesize_delete_event:
 * @widget:
 *
 * Synthesize a delete event to @widget.
 **/
static void
clay_ui_synthesize_delete_event (GtkWidget *widget)
{
  GdkWindow *window = NULL;
  GdkEvent *event = NULL;

  window = gtk_widget_get_window (widget);
  g_assert_true (window);

  event = gdk_event_new (GDK_DELETE);
  event->any.window     = g_object_ref (window);
  event->any.send_event = TRUE;
  gtk_main_do_event (event);
  gdk_event_free (event);
}

static GtkWidget *
clay_ui_find_window (ClayDialogFactory *dialog_factory,
                     ClayUiTestFunc     predicate)
{
  GList     *iter        = NULL;
  GtkWidget *dock_window = NULL;

  g_return_val_if_fail (predicate != NULL, NULL);

  for (iter = clay_dialog_factory_get_session_infos (dialog_factory);
       iter;
       iter = g_list_next (iter))
    {
      GtkWidget *widget = clay_session_info_get_widget (iter->data);

      if (predicate (G_OBJECT (widget)))
        {
          dock_window = widget;
          break;
        }
    }

  return dock_window;
}

static gboolean
clay_ui_not_toolbox_window (GObject *object)
{
  return (CLAY_IS_DOCK_WINDOW (object) &&
          ! clay_dock_window_has_toolbox (CLAY_DOCK_WINDOW (object)));
}

static gboolean
clay_ui_multicolumn_not_toolbox_window (GObject *object)
{
  gboolean           not_toolbox_window;
  ClayDockWindow    *dock_window;
  ClayDockContainer *dock_container;
  GList             *docks;

  if (! CLAY_IS_DOCK_WINDOW (object))
    return FALSE;

  dock_window    = CLAY_DOCK_WINDOW (object);
  dock_container = CLAY_DOCK_CONTAINER (object);
  docks          = clay_dock_container_get_docks (dock_container);

  not_toolbox_window = (! clay_dock_window_has_toolbox (dock_window) &&
                        g_list_length (docks) > 1);

  g_list_free (docks);

  return not_toolbox_window;
}

static void
clay_ui_switch_window_mode (Clay *clay)
{
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "windows",
                                   "windows-use-single-window-mode");
  clay_test_run_mainloop_until_idle ();

  /* Add a small sleep to let things stabilize */
  g_usleep (500 * 1000);
  clay_test_run_mainloop_until_idle ();
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

  /* Add tests. Note that the order matters. For example,
   * 'paintbrush_is_standard_tool' can't be after
   * 'tool_options_editor_updates'
   */
  ADD_TEST (paintbrush_is_standard_tool);
  ADD_TEST (tool_options_editor_updates);
  ADD_TEST (create_new_image_via_dialog);
  ADD_TEST (keyboard_zoom_focus);
  ADD_TEST (restore_recently_closed_multi_column_dock);
  ADD_TEST (tab_toggle_dont_change_dock_window_position);
  ADD_TEST (switch_to_single_window_mode);
  ADD_TEST (switch_back_to_multi_window_mode);
  ADD_TEST (close_image);
  ADD_TEST (window_roles);
  ADD_TEST (swap_tools);

  /* Run the tests and return status */
  g_application_run (clay->app, 0, NULL);
  result = clay_core_app_get_exit_status (CLAY_CORE_APP (clay->app));

  g_application_quit (G_APPLICATION (clay->app));
  g_clear_object (&clay->app);

  return result;
}
