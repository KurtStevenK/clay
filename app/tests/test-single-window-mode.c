/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * test-single-window-mode.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "dialogs/dialogs-types.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-scale.h"
#include "display/claydisplayshell-transform.h"
#include "display/clayimagewindow.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockable.h"
#include "widgets/claydockbook.h"
#include "widgets/claydockcontainer.h"
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


#define ADD_TEST(function) \
  g_test_add_data_func ("/clay-single-window-mode/" #function, clay, function);


/* Put this in the code below when you want the test to pause so you
 * can do measurements of widgets on the screen for example
 */
#define CLAY_PAUSE (g_usleep (20 * 1000 * 1000))


/**
 * new_dockable_not_in_new_window:
 * @data:
 *
 * Test that in single-window mode, new dockables are not put in new
 * windows (they should end up in the single image window).
 **/
static void
new_dockable_not_in_new_window (gconstpointer data)
{
  Clay              *clay             = Clay (data);
  ClayDialogFactory *factory          = clay_dialog_factory_get_singleton ();
  gint               dialogs_before   = 0;
  gint               toplevels_before = 0;
  gint               dialogs_after    = 0;
  gint               toplevels_after  = 0;
  GList             *dialogs;
  GList             *iter;

  clay_test_run_mainloop_until_idle ();

  /* Count dialogs before we create the dockable */
  dialogs        = clay_dialog_factory_get_open_dialogs (factory);
  dialogs_before = g_list_length (dialogs);
  for (iter = dialogs; iter; iter = g_list_next (iter))
    {
      if (gtk_widget_is_toplevel (iter->data))
        toplevels_before++;
    }

  /* Create a dockable */
  clay_ui_manager_activate_action (clay_test_utils_get_ui_manager (clay),
                                   "dialogs",
                                   "dialogs-undo-history");
  clay_test_run_mainloop_until_idle ();

  /* Count dialogs after we created the dockable */
  dialogs       = clay_dialog_factory_get_open_dialogs (factory);
  dialogs_after = g_list_length (dialogs);
  for (iter = dialogs; iter; iter = g_list_next (iter))
    {
      if (gtk_widget_is_toplevel (iter->data))
        toplevels_after++;
    }

  /* We got one more session managed dialog ... */
  g_assert_cmpint (dialogs_before + 1, ==, dialogs_after);
  /* ... but no new toplevels */
  g_assert_cmpint (toplevels_before, ==, toplevels_after);
}

int main(int argc, char **argv)
{
  Clay  *clay   = NULL;
  gint   result = -1;

  clay_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  clay_test_utils_setup_menus_path ();

  /* Launch Clay in single-window mode */
  g_setenv ("CLAY_TESTING_SESSIONRC_NAME", "sessionrc-2-8-single-window", TRUE /*overwrite*/);
  clay = clay_init_for_gui_testing (TRUE /*show_gui*/);
  clay_test_run_mainloop_until_idle ();

  ADD_TEST (new_dockable_not_in_new_window);

  /* Run the tests and return status */
  g_application_run (clay->app, 0, NULL);
  result = clay_core_app_get_exit_status (CLAY_CORE_APP (clay->app));

  /* Exit properly so we don't break script-fu plug-in wire */
  g_application_quit (G_APPLICATION (clay->app));
  g_clear_object (&clay->app);

  return result;
}
