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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "display/display-types.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/clayimagewindow.h"

#include "menus/menus.h"

#include "widgets/clayuimanager.h"
#include "widgets/claydialogfactory.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayer-new.h"

#include "tests.h"

#include "clay-app-test-utils.h"


#ifdef GDK_WINDOWING_QUARTZ
// only to get keycode definitions from HIToolbox/Events.h
#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>
#endif /* GDK_WINDOWING_QUARTZ */

void
clay_test_utils_set_env_to_subdir (const gchar *root_env_var,
                                   const gchar *subdir,
                                   const gchar *target_env_var)
{
  const gchar *root_dir   = NULL;
  gchar       *target_dir = NULL;

  /* Get root dir */
  root_dir = g_getenv (root_env_var);
  if (! root_dir)
    g_printerr ("*\n"
                "*  The env var %s is not set, you are probably running\n"
                "*  in a debugger. Set it manually, e.g.:\n"
                "*\n"
                "*    set env %s=%s/source/clay\n"
                "*\n",
                root_env_var,
                root_env_var, g_get_home_dir ());

  /* Construct path and setup target env var */
  target_dir = g_build_filename (root_dir, subdir, NULL);
  g_setenv (target_env_var, target_dir, TRUE);
  g_free (target_dir);
}

void
clay_test_utils_set_env_to_subpath (const gchar *root_env_var1,
                                    const gchar *root_env_var2,
                                    const gchar *subdir,
                                    const gchar *target_env_var)
{
  const gchar *root_dir1   = NULL;
  const gchar *root_dir2   = NULL;
  gchar       *target_dir1 = NULL;
  gchar       *target_dir2 = NULL;
  gchar       *target_path = NULL;

  /* Get root dir */
  root_dir1 = g_getenv (root_env_var1);
  if (! root_dir1)
    g_printerr ("*\n"
                "*  The env var %s is not set, you are probably running\n"
                "*  in a debugger. Set it manually, e.g.:\n"
                "*\n"
                "*    set env %s=%s/source/clay\n"
                "*\n",
                root_env_var1,
                root_env_var1, g_get_home_dir ());

  root_dir2 = g_getenv (root_env_var2);
  if (! root_dir2)
    g_printerr ("*\n"
                "*  The env var %s is not set, you are probably running\n"
                "*  in a debugger. Set it manually, e.g.:\n"
                "*\n"
                "*    set env %s=%s/source/clay\n"
                "*\n",
                root_env_var2,
                root_env_var2, g_get_home_dir ());

  /* Construct path and setup target env var */
  target_dir1 = g_build_filename (root_dir1, subdir, NULL);
  target_dir2 = g_build_filename (root_dir2, subdir, NULL);

  target_path = g_strconcat (target_dir1, G_SEARCHPATH_SEPARATOR_S,
                             target_dir2, NULL);

  g_free (target_dir1);
  g_free (target_dir2);

  g_setenv (target_env_var, target_path, TRUE);
  g_free (target_path);
}


/**
 * clay_test_utils_set_clay3_directory:
 * @root_env_var: Either "CLAY_TESTING_ABS_TOP_SRCDIR" or
 *                "CLAY_TESTING_ABS_TOP_BUILDDIR"
 * @subdir:       Subdir, may be %NULL
 *
 * Sets CLAY3_DIRECTORY to the source dir @root_env_var/@subdir. The
 * environment variables is set up by the test runner, see Makefile.am
 **/
void
clay_test_utils_set_clay3_directory (const gchar *root_env_var,
                                     const gchar *subdir)
{
  clay_test_utils_set_env_to_subdir (root_env_var,
                                     subdir,
                                     "CLAY3_DIRECTORY" /*target_env_var*/);
}

/**
 * clay_test_utils_setup_menus_path:
 *
 * Sets CLAY_TESTING_MENUS_PATH to "$top_srcdir/menus:$top_builddir/menus".
 **/
void
clay_test_utils_setup_menus_path (void)
{
  /* CLAY_TESTING_ABS_TOP_SRCDIR is set by the automake test runner,
   * see Makefile.am
   */
  clay_test_utils_set_env_to_subpath ("CLAY_TESTING_ABS_TOP_SRCDIR",
                                      "CLAY_TESTING_ABS_TOP_BUILDDIR",
                                      "menus",
                                      "CLAY_TESTING_MENUS_PATH");
}

/**
 * clay_test_utils_create_image:
 * @clay:   A #Clay instance.
 * @width:  Width of image (and layer)
 * @height: Height of image (and layer)
 *
 * Creates a new image of a given size with one layer of same size and
 * a display.
 *
 * Returns: The new #ClayImage.
 **/
void
clay_test_utils_create_image (Clay *clay,
                              gint  width,
                              gint  height)
{
  ClayImage *image;
  ClayLayer *layer;

  image = clay_image_new (clay, width, height,
                          CLAY_RGB, CLAY_PRECISION_U8_NON_LINEAR);

  layer = clay_layer_new (image,
                          width,
                          height,
                          clay_image_get_layer_format (image, TRUE),
                          "layer1",
                          1.0,
                          CLAY_LAYER_MODE_NORMAL);

  clay_image_add_layer (image,
                        layer,
                        NULL /*parent*/,
                        0 /*position*/,
                        FALSE /*push_undo*/);

  clay_create_display (clay,
                       image,
                       clay_unit_pixel (),
                       1.0 /*scale*/,
                       NULL);
}

/**
 * clay_test_utils_synthesize_key_event:
 * @widget: Widget to target.
 * @keyval: Keyval, e.g. GDK_Return
 *
 * Simulates a keypress and release with gdk_test_simulate_key().
 **/
void
clay_test_utils_synthesize_key_event (GtkWidget *widget,
                                      guint      keyval)
{
#if defined(GDK_WINDOWING_QUARTZ)

GdkKeymapKey *keys   = NULL;
gint          n_keys = 0;
gint          i;
CGEventRef    keyUp, keyDown;

if (gdk_keymap_get_entries_for_keyval (gdk_keymap_get_for_display (gdk_display_get_default ()), keyval, &keys, &n_keys))
  {
    /* XXX not in use yet */
    CGEventRef commandDown	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Command, true);
    CGEventRef commandUp	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Command, false);

    CGEventRef shiftDown	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Shift, true);
    CGEventRef shiftUp		=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Shift, false);

    CGEventRef optionDown	=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Option, true);
    CGEventRef optionUp		=	CGEventCreateKeyboardEvent (NULL, (CGKeyCode)kVK_Option, false);

    for (i = 0; i < n_keys; i++)
      {
        /* Option press. */
        if (keys[i].group)
          {
            CGEventPost (kCGHIDEventTap, optionDown);
          }
        /* Shift press. */
        if (keys[i].level)
          {
            CGEventPost(kCGHIDEventTap, shiftDown);
          }
        keyDown = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)keys[i].keycode, true);
        keyUp = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)keys[i].keycode, false);
        /* Key pressed. */
        CGEventPost (kCGHIDEventTap, keyDown);
        CFRelease (keyDown);
        usleep (100);
        /* key released */
        CGEventPost (kCGHIDEventTap, keyUp);
        CFRelease (keyUp);

        /* Shift release. */
        if (keys[i].level)
          {
            CGEventPost (kCGHIDEventTap, shiftDown);
          }

        /* Option release. */
        if (keys[i].group)
          {
            CGEventPost (kCGHIDEventTap, optionUp);
          }
        /* No need to loop for alternative keycodes. We want only one
         * key generated. */
        break;
      }
    CFRelease (commandDown);
    CFRelease (commandUp);
    CFRelease (shiftDown);
    CFRelease (shiftUp);
    CFRelease (optionDown);
    CFRelease (optionUp);
    g_free (keys);
  }
else
  {
    g_warning ("%s: no macOS key mapping found for keyval %d.", G_STRFUNC, keyval);
  }

#else /* ! GDK_WINDOWING_QUARTZ */

  gtk_test_widget_send_key(widget, keyval, 0);

#endif /* ! GDK_WINDOWING_QUARTZ */
}

/**
 * clay_test_utils_get_ui_manager:
 * @clay: The #Clay instance.
 *
 * Returns the "best" #ClayUIManager to use when performing
 * actions. It gives the ui manager of the empty display if it exists,
 * otherwise it gives it the ui manager of the first display.
 *
 * Returns: The #ClayUIManager.
 **/
ClayUIManager *
clay_test_utils_get_ui_manager (Clay *clay)
{
  return menus_get_image_manager_singleton (clay);
}

/**
 * clay_test_utils_create_image_from_dalog:
 * @clay:
 *
 * Creates a new image using the "New image" dialog, and then returns
 * the #ClayImage created.
 *
 * Returns: The created #ClayImage.
 **/
ClayImage *
clay_test_utils_create_image_from_dialog (Clay *clay)
{
  ClayImage     *image            = NULL;
  GtkWidget     *new_image_dialog = NULL;
  guint          n_initial_images = g_list_length (clay_get_image_iter (clay));
  guint          n_images         = -1;
  gint           tries_left       = 100;
  ClayUIManager *ui_manager       = clay_test_utils_get_ui_manager (clay);

  /* Bring up the new image dialog */
  clay_ui_manager_activate_action (ui_manager,
                                   "image",
                                   "image-new");
  clay_test_run_mainloop_until_idle ();

  /* Get the GtkWindow of the dialog */
  new_image_dialog =
    clay_dialog_factory_dialog_raise (clay_dialog_factory_get_singleton (),
                                      gdk_display_get_monitor (gdk_display_get_default (), 0),
                                      NULL,
                                      "clay-image-new-dialog",
                                      -1 /*view_size*/);

  /* Press the OK button. It will take a while for the image to be
   * created so loop for a while
   */
  gtk_dialog_response (GTK_DIALOG (new_image_dialog), GTK_RESPONSE_OK);
  do
    {
      g_usleep (20 * 1000);
      clay_test_run_mainloop_until_idle ();
      n_images = g_list_length (clay_get_image_iter (clay));
    }
  while (tries_left-- &&
         n_images != n_initial_images + 1);

  /* Make sure there now is one image more than initially */
  g_assert_cmpint (n_images,
                   ==,
                   n_initial_images + 1);

  image = CLAY_IMAGE (clay_get_image_iter (clay)->data);

  return image;
}

