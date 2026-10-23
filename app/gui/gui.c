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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"
#include "libclaywidgets/claywidgets-private.h"

#include "gui-types.h"
#include "clayapp.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claytoolinfo.h"

#include "plug-in/clayenvirontable.h"
#include "plug-in/claypluginmanager.h"

#include "display/claycanvas-style.h"
#include "display/claydisplay.h"
#include "display/claydisplay-foreach.h"
#include "display/claydisplayshell.h"
#include "display/claystatusbar.h"

#include "tools/clay-tools.h"
#include "tools/claytool.h"
#include "tools/tool_manager.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/clayaction-history.h"
#include "widgets/clayclipboard.h"
#include "widgets/claycolorselectorpalette.h"
#include "widgets/claycontrollermanager.h"
#include "widgets/claycontrollers.h"
#include "widgets/claydevices.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydnd.h"
#include "widgets/clayrender.h"
#include "widgets/clayhelp.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymenufactory.h"
#include "widgets/claymessagebox.h"
#include "widgets/clayradioaction.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claytranslationstore.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "actions/actions.h"
#include "actions/windows-commands.h"

#include "menus/menus.h"

#include "dialogs/dialogs.h"

#include "clayuiconfigurer.h"
#include "gui.h"
#include "gui-unique.h"
#include "gui-vtable.h"
#include "icon-themes.h"
#include "modifiers.h"
#include "session.h"
#include "splash.h"
#include "themes.h"

#ifdef G_OS_WIN32
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#import <AppKit/AppKit.h>
#include <gdk/quartz/gdkquartz-cocoa-access.h>

/* Forward declare since we are building against old SDKs. */
#if !defined(MAC_OS_X_VERSION_10_12) || \
    MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12

@interface NSWindow(ForwardDeclarations)
+ (void)setAllowsAutomaticWindowTabbing:(BOOL)allow;
@end

#endif

#endif /* GDK_WINDOWING_QUARTZ */

#include "clay-intl.h"


/*  local function prototypes  */

static gchar     * gui_sanity_check              (void);
static void        gui_help_func                 (const gchar        *help_id,
                                                  gpointer            help_data);
static GeglColor * gui_get_background_func       (void);
static GeglColor * gui_get_foreground_func       (void);

static void        gui_initialize_after_callback (Clay               *clay,
                                                  ClayInitStatusFunc  callback);

static void        gui_restore_callback          (Clay               *clay,
                                                  ClayInitStatusFunc  callback);
static void        gui_restore_after_callback    (Clay               *clay,
                                                  ClayInitStatusFunc  callback);

static gboolean    gui_exit_callback             (Clay               *clay,
                                                  gboolean            force);
static gboolean    gui_exit_after_callback       (Clay               *clay,
                                                  gboolean            force);

static void        gui_show_help_button_notify   (ClayGuiConfig      *gui_config,
                                                  GParamSpec         *pspec,
                                                  Clay               *clay);
static void        gui_user_manual_notify        (ClayGuiConfig      *gui_config,
                                                  GParamSpec         *pspec,
                                                  Clay               *clay);
static void        gui_single_window_mode_notify (ClayGuiConfig      *gui_config,
                                                  GParamSpec         *pspec,
                                                  ClayUIConfigurer   *ui_configurer);

static void        gui_clipboard_changed         (Clay               *clay);

static void        gui_menu_show_tooltip         (ClayUIManager      *manager,
                                                  const gchar        *tooltip,
                                                  Clay               *clay);
static void        gui_menu_hide_tooltip         (ClayUIManager      *manager,
                                                  Clay               *clay);

static void        gui_display_changed           (ClayContext        *context,
                                                  ClayDisplay        *display,
                                                  Clay               *clay);

static void        gui_check_unique_accelerators (Clay               *clay);

#ifdef GDK_WINDOWING_QUARTZ
static gboolean    gui_macos_shortcut            (const GdkEventKey  *kevent);
static void        gui_macos_event_handler       (GdkEvent           *event,
                                                  gpointer            data);
#endif


/*  private variables  */

static Clay             *the_gui_clay     = NULL;
static ClayUIConfigurer *ui_configurer    = NULL;
static GdkMonitor       *initial_monitor  = NULL;

typedef struct
{
  GtkWidget  *window;
  GdkMonitor *monitor;
} ClayPlaceData;

static GdkMonitor *
clay_startup_monitor (Clay *clay)
{
  GdkDisplay *display;
  gint        n_monitors;
  gint        index;

  display = gdk_display_get_default ();
  n_monitors = gdk_display_get_n_monitors (display);

  if (n_monitors < 1)
    return NULL;

  index = CLAY_GUI_CONFIG (clay->config)->startup_monitor;

  if (index < 0 || index >= n_monitors)
    {
      GdkMonitor *primary = gdk_display_get_primary_monitor (display);

      if (primary)
        return primary;

      return gdk_display_get_monitor (display, 0);
    }

  return gdk_display_get_monitor (display, index);
}

static gboolean
clay_place_on_configure (GtkWidget         *widget,
                         GdkEventConfigure *event,
                         ClayPlaceData     *place)
{
  GdkRectangle work;
  gint         width;
  gint         height;

  if (! GDK_IS_MONITOR (place->monitor))
    return FALSE;

  gdk_monitor_get_workarea (place->monitor, &work);

  if (event->width < 1000)
    return FALSE;

  /* Leave a 32 px desktop margin. On a 2560x1440 screen the window is
   * about 2496 wide, and shorter than 1400 so it stays above the taskbar.
   */
  width  = MAX (work.width  - 64, 640);
  height = MAX (work.height - 64, 480);

  g_signal_handlers_disconnect_by_func (widget, clay_place_on_configure, place);

  gtk_window_unmaximize (GTK_WINDOW (widget));
  gtk_window_set_position (GTK_WINDOW (widget), GTK_WIN_POS_NONE);
  gtk_window_set_geometry_hints (GTK_WINDOW (widget), NULL, NULL, 0);
  gtk_window_set_gravity (GTK_WINDOW (widget), GDK_GRAVITY_NORTH_WEST);
  gtk_window_move (GTK_WINDOW (widget), work.x + 32, work.y + 32);
  gtk_window_resize (GTK_WINDOW (widget), width, height);

  g_clear_object (&place->window);
  g_clear_object (&place->monitor);
  g_free (place);

  return FALSE;
}


/*  public functions  */

void
gui_libs_init (GOptionContext *context)
{
  g_return_if_fail (context != NULL);

  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  /*  make the ClayDisplay type known by name early, needed for the PDB */
  g_type_class_ref (CLAY_TYPE_DISPLAY);
}

void
gui_abort (const gchar *abort_message)
{
  GtkWidget *dialog;
  GtkWidget *box;

  g_return_if_fail (abort_message != NULL);

  dialog = clay_dialog_new (_("Clay Message"), "clay-abort",
                            NULL, GTK_DIALOG_MODAL, NULL, NULL,

                            _("_OK"), GTK_RESPONSE_OK,

                            NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  box = g_object_new (CLAY_TYPE_MESSAGE_BOX,
                      "icon-name",    CLAY_ICON_WILBER_EEK,
                      "border-width", 12,
                      NULL);

  clay_message_box_set_text (CLAY_MESSAGE_BOX (box), "%s", abort_message);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      box, TRUE, TRUE, 0);
  gtk_widget_set_visible (box, TRUE);

  clay_dialog_run (CLAY_DIALOG (dialog));

  exit (EXIT_FAILURE);
}

/**
 * gui_init:
 * @clay:
 * @no_splash:
 * @test_base_dir: a base prefix directory.
 *
 * @test_base_dir should be set to %NULL in all our codebase except for
 * unit testing calls.
 */
ClayInitStatusFunc
gui_init (Clay         *clay,
          gboolean      no_splash,
          ClayApp      *app,
          const gchar  *test_base_dir,
          const gchar  *system_lang_l10n)
{
  ClayInitStatusFunc  status_callback = NULL;
  gchar              *abort_message;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (the_gui_clay == NULL, NULL);
  g_return_val_if_fail (CLAY_IS_APP (app) || app == NULL, NULL);

  abort_message = gui_sanity_check ();
  if (abort_message)
    gui_abort (abort_message);

  the_gui_clay = clay;

  /* Normally this should have been taken care of during command line
   * parsing as a post-parse hook of gtk_get_option_group(), using the
   * system locales.
   * But user config may have overridden the language, therefore we must
   * check the widget directions again.
   */
  gtk_widget_set_default_direction (gtk_get_locale_direction ());

  gui_unique_init (clay);
  clay_translation_store_initialize (system_lang_l10n);

  /*  initialize icon themes before clay_widgets_init() so we avoid
   *  setting the configured theme twice
   */
  icon_themes_init (clay);

  clay_widgets_init (gui_help_func,
                     gui_get_foreground_func,
                     gui_get_background_func,
                     NULL, test_base_dir);

  g_type_class_ref (CLAY_TYPE_COLOR_SELECT);

  /*  disable automatic startup notification  */
  gtk_window_set_auto_startup_notification (FALSE);

#ifdef GDK_WINDOWING_QUARTZ
  /* Before the first window is created (typically the splash window),
   * we need to disable automatic tabbing behavior introduced on Sierra.
   * This is known to cause all kinds of weird issues (see for instance
   * Bugzilla #776294) and needs proper GTK+ support if we would want to
   * enable it.
   */
  if ([NSWindow respondsToSelector:@selector(setAllowsAutomaticWindowTabbing:)])
    [NSWindow setAllowsAutomaticWindowTabbing:NO];

  /* respect the macOS "Show scroll bars: Always" system preference. See: #16291 */
  if ([NSScroller preferredScrollerStyle] == NSScrollerStyleLegacy)
    g_object_set (gtk_settings_get_default (),
                  "gtk-overlay-scrolling", FALSE,
                  NULL);

  /* standard macOS shortcuts (handled by the OS) */
  gdk_event_handler_set (gui_macos_event_handler, NULL, NULL);
#endif /* GDK_WINDOWING_QUARTZ */

  clay_dnd_init (clay);

  themes_init (clay);
  clay_canvas_styles_init ();

  initial_monitor = clay_startup_monitor (clay);

  if (! no_splash)
    {
      splash_create (clay, clay->be_verbose, initial_monitor, app);
      status_callback = splash_update;
    }

  g_signal_connect_after (clay, "initialize",
                          G_CALLBACK (gui_initialize_after_callback),
                          NULL);

  g_signal_connect (clay, "restore",
                    G_CALLBACK (gui_restore_callback),
                    NULL);
  g_signal_connect_after (clay, "restore",
                          G_CALLBACK (gui_restore_after_callback),
                          NULL);

  g_signal_connect (clay, "exit",
                    G_CALLBACK (gui_exit_callback),
                    NULL);
  g_signal_connect_after (clay, "exit",
                          G_CALLBACK (gui_exit_after_callback),
                          NULL);

  return status_callback;
}

/*
 * gui_recover:
 * @n_recoveries: number of recovered files.
 *
 * Query the user interactively if files were saved from a previous
 * crash, asking whether to try and recover or discard them.
 *
 * Returns: TRUE if answer is to try and recover, FALSE otherwise.
 */
gboolean
gui_recover (gint n_recoveries)
{
  GtkWidget *dialog;
  GtkWidget *box;
  gboolean   recover;

  dialog = clay_dialog_new (_("Image Recovery"), "clay-recovery",
                            NULL, GTK_DIALOG_MODAL, NULL, NULL,
                            _("_Discard"), GTK_RESPONSE_CANCEL,
                            _("_Recover"), GTK_RESPONSE_OK,
                            NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_OK);

  box = clay_message_box_new (CLAY_ICON_WILBER_EEK);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      box, TRUE, TRUE, 0);
  gtk_widget_set_visible (box, TRUE);

  clay_message_box_set_primary_text (CLAY_MESSAGE_BOX (box),
                                     _("Eeek! It looks like Clay recovered from a crash!"));

  clay_message_box_set_text (CLAY_MESSAGE_BOX (box),
                             /* TRANSLATORS: even if English singular form does
                              * not use %d, you can use %d for translation in
                              * any singular/plural form of your language if
                              * suited. It will just work and be replaced by the
                              * number of images as expected.
                              */
                             ngettext ("An image was salvaged from the crash. "
                                       "Do you want to try and recover it?",
                                       "%d images were salvaged from the crash. "
                                       "Do you want to try and recover them?",
                                       n_recoveries), n_recoveries);

  recover = (clay_dialog_run (CLAY_DIALOG (dialog)) == GTK_RESPONSE_OK);
  gtk_widget_destroy (dialog);

  return recover;
}

GdkMonitor *
gui_get_initial_monitor (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), 0);

  return initial_monitor;
}


/*  private functions  */

static gchar *
gui_sanity_check (void)
{
#define GTK_REQUIRED_MAJOR 3
#define GTK_REQUIRED_MINOR 24
#define GTK_REQUIRED_MICRO 0

  const gchar *mismatch = gtk_check_version (GTK_REQUIRED_MAJOR,
                                             GTK_REQUIRED_MINOR,
                                             GTK_REQUIRED_MICRO);

  if (mismatch)
    {
      return g_strdup_printf
        ("%s\n\n"
         "Clay requires GTK version %d.%d.%d or later.\n"
         "Installed GTK version is %d.%d.%d.\n\n"
         "Somehow you or your software packager managed\n"
         "to install Clay with an older GTK version.\n\n"
         "Please upgrade to GTK version %d.%d.%d or later.",
         mismatch,
         GTK_REQUIRED_MAJOR, GTK_REQUIRED_MINOR, GTK_REQUIRED_MICRO,
         gtk_major_version, gtk_minor_version, gtk_micro_version,
         GTK_REQUIRED_MAJOR, GTK_REQUIRED_MINOR, GTK_REQUIRED_MICRO);
    }

#undef GTK_REQUIRED_MAJOR
#undef GTK_REQUIRED_MINOR
#undef GTK_REQUIRED_MICRO

  return NULL;
}

#ifdef GDK_WINDOWING_QUARTZ
/* GDKquartz intercepts every key combo before either AppKit or macOS gets
   to see it, so let's redirect some baked in shortcuts back to macOS */
static gboolean
gui_macos_shortcut (const GdkEventKey *kevent)
{
  GdkModifierType primary  = clay_get_primary_accelerator_mask ();
  gboolean        redirect = FALSE;

  /* on macOS, Ctrl+F2 is used instead of Alt. See: #13502 */
  if (kevent->keyval == GDK_KEY_F2       &&
      (kevent->state & GDK_CONTROL_MASK) &&
      ! (kevent->state & primary))
    redirect = TRUE;

  /* on macOS, Cmd+Shift+/ opens the Help menu. See: #7111 */
  else if ((kevent->keyval == GDK_KEY_question ||
            (kevent->keyval == GDK_KEY_slash && (kevent->state & GDK_SHIFT_MASK))) &&
           (kevent->state & primary))
    redirect = TRUE;

  if (redirect)
    {
      NSEvent *nsevent = gdk_quartz_event_get_nsevent ((GdkEvent *) kevent);
      if (nsevent)
        [NSApp sendEvent:nsevent];

      return TRUE;
    }

  /* on macOS, Ctrl+Cmd+Space opens the Emoji viewer */
  if ((kevent->keyval == GDK_KEY_space || kevent->keyval == GDK_KEY_KP_Space) &&
      (kevent->state & GDK_CONTROL_MASK)                                      &&
      (kevent->state & primary))
    {
      [NSApp orderFrontCharacterPalette:nil];

      return TRUE;
    }

  return FALSE;
}

static void
gui_macos_event_handler (GdkEvent *event,
                         gpointer  data)
{
  if (event->type == GDK_KEY_PRESS &&
      gui_macos_shortcut ((const GdkEventKey *) event))
    return;

  gtk_main_do_event (event);
}
#endif /* GDK_WINDOWING_QUARTZ */

static void
gui_help_func (const gchar *help_id,
               gpointer     help_data)
{
  g_return_if_fail (CLAY_IS_CLAY (the_gui_clay));

  clay_help (the_gui_clay, NULL, NULL, help_id);
}

static GeglColor *
gui_get_foreground_func (void)
{
  GeglColor *color;

  g_return_val_if_fail (CLAY_IS_CLAY (the_gui_clay), FALSE);

  color = clay_context_get_foreground (clay_get_user_context (the_gui_clay));

  return gegl_color_duplicate (color);
}

static GeglColor *
gui_get_background_func (void)
{
  GeglColor *color;

  g_return_val_if_fail (CLAY_IS_CLAY (the_gui_clay), FALSE);

  color = clay_context_get_background (clay_get_user_context (the_gui_clay));

  return gegl_color_duplicate (color);
}

static void
gui_initialize_after_callback (Clay               *clay,
                               ClayInitStatusFunc  status_callback)
{
  const gchar *name = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

#if defined (GDK_WINDOWING_X11)
  name = "DISPLAY";
#elif defined (GDK_WINDOWING_DIRECTFB) || defined (GDK_WINDOWING_FB)
  name = "GDK_DISPLAY";
#endif

  /* TODO: Need to care about display migration with GTK+ 2.2 at some point */

  if (name)
    {
      const gchar *display = gdk_display_get_name (gdk_display_get_default ());

      clay_environ_table_add (clay->plug_in_manager->environ_table,
                              name, display, NULL);
    }

  clay_tools_init (clay);

  clay_context_set_tool (clay_get_user_context (clay),
                         clay_tool_info_get_standard (clay));
}

static void
gui_restore_callback (Clay               *clay,
                      ClayInitStatusFunc  status_callback)
{
  ClayDisplayConfig *display_config = CLAY_DISPLAY_CONFIG (clay->config);
  ClayGuiConfig     *gui_config     = CLAY_GUI_CONFIG (clay->config);

  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  gui_vtable_init (clay);

  clay_dialogs_show_help_button (gui_config->use_help &&
                                 gui_config->show_help_button);

  g_signal_connect (gui_config, "notify::use-help",
                    G_CALLBACK (gui_show_help_button_notify),
                    clay);
  g_signal_connect (gui_config, "notify::user-manual-online",
                    G_CALLBACK (gui_user_manual_notify),
                    clay);
  g_signal_connect (gui_config, "notify::show-help-button",
                    G_CALLBACK (gui_show_help_button_notify),
                    clay);

  g_signal_connect (clay_get_user_context (clay), "display-changed",
                    G_CALLBACK (gui_display_changed),
                    clay);

  /* make sure the monitor resolution is valid */
  if (display_config->monitor_res_from_gdk               ||
      display_config->monitor_xres < CLAY_MIN_RESOLUTION ||
      display_config->monitor_yres < CLAY_MIN_RESOLUTION)
    {
      gdouble xres, yres;

      clay_get_monitor_resolution (initial_monitor, &xres, &yres);

      g_object_set (clay->config,
                    "monitor-xresolution",                      xres,
                    "monitor-yresolution",                      yres,
                    "monitor-resolution-from-windowing-system", TRUE,
                    NULL);
    }

  actions_init (clay);
  menus_init (clay);
  clay_render_init (clay);

  dialogs_init (clay);

  clay_clipboard_init (clay);
  if (clay_get_clipboard_image (clay))
    clay_clipboard_set_image (clay, clay_get_clipboard_image (clay));
  else
    clay_clipboard_set_buffer (clay, clay_get_clipboard_buffer (clay));

  g_signal_connect (clay, "clipboard-changed",
                    G_CALLBACK (gui_clipboard_changed),
                    NULL);

  clay_devices_init (clay);
  clay_controllers_init (clay);
  modifiers_init (clay);
  session_init (clay);

  g_type_class_unref (g_type_class_ref (CLAY_TYPE_COLOR_SELECTOR_PALETTE));

  status_callback (NULL, _("Tool Options"), 1.0);
  clay_tools_restore (clay);
}

static void
gui_restore_after_callback (Clay               *clay,
                            ClayInitStatusFunc  status_callback)
{
  ClayGuiConfig         *gui_config = CLAY_GUI_CONFIG (clay->config);
  ClayControllerManager *controller_manager;
  ClayUIManager         *image_ui_manager;
  ClayDisplay           *display;
#ifdef G_OS_WIN32
  STARTUPINFO            StartupInfo;

  GetStartupInfo (&StartupInfo);
#endif

  if (clay->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  clay->message_handler = CLAY_MESSAGE_BOX;

  /*  load the recent documents after clay_real_restore() because we
   *  need the mime-types implemented by plug-ins
   */
  status_callback (NULL, _("Documents"), 0.9);
  clay_recent_list_load (clay);

  ui_configurer = g_object_new (CLAY_TYPE_UI_CONFIGURER,
                                "clay", clay,
                                NULL);

  image_ui_manager = menus_get_image_manager_singleton (clay);
  clay_ui_manager_update (image_ui_manager, clay);

  if (gui_config->restore_accels)
    menus_restore (clay);

  /* Check that every accelerator is unique. */
  gui_check_unique_accelerators (clay);

  clay_action_history_init (clay);

  g_signal_connect_object (gui_config, "notify::single-window-mode",
                           G_CALLBACK (gui_single_window_mode_notify),
                           ui_configurer, 0);
  g_signal_connect (image_ui_manager, "show-tooltip",
                    G_CALLBACK (gui_menu_show_tooltip),
                    clay);
  g_signal_connect (image_ui_manager, "hide-tooltip",
                    G_CALLBACK (gui_menu_hide_tooltip),
                    clay);

  clay_devices_restore (clay);
  controller_manager = clay_get_controller_manager (clay);
  clay_controller_manager_restore (controller_manager, image_ui_manager);
  modifiers_restore (clay);

  if (status_callback == splash_update)
    splash_destroy ();

  if (clay_get_show_gui (clay))
    {
      ClayDisplayShell *shell;
      GtkWidget        *toplevel;

      /*  create the empty display  */
      display = CLAY_DISPLAY (clay_create_display (clay, NULL,
                                                   clay_unit_pixel (), 1.0,
                                                   G_OBJECT (initial_monitor)));

      shell = clay_display_get_shell (display);

#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
      themes_set_title_bar (clay);
#endif

      if (gui_config->restore_session)
        session_restore (clay, initial_monitor);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

#ifdef G_OS_WIN32
      /* Prevents window from reappearing on start-up if the user
       * requested it to be minimized via window hints
       */
      if (StartupInfo.wShowWindow != SW_SHOWMINIMIZED   &&
          StartupInfo.wShowWindow != SW_SHOWMINNOACTIVE &&
          StartupInfo.wShowWindow != SW_MINIMIZE)
#endif
      /*  move keyboard focus to the display  */
      gtk_window_present (GTK_WINDOW (toplevel));

      /* Session restore runs on "show" and would put this window back
       * on whichever screen it occupied last. Place it after that, on
       * the configured startup screen, with a 20 px margin.
       */
      if (GTK_IS_WINDOW (toplevel) && GDK_IS_MONITOR (initial_monitor))
        {
          ClayPlaceData *place = g_new0 (ClayPlaceData, 1);

          place->window  = g_object_ref (toplevel);
          place->monitor = g_object_ref (initial_monitor);
          g_signal_connect (toplevel, "configure-event",
                            G_CALLBACK (clay_place_on_configure),
                            place);
        }
    }

  /*  indicate that the application has finished loading  */
  gdk_notify_startup_complete ();

  /*  clear startup monitor variables  */
  initial_monitor = NULL;
}

static gboolean
gui_exit_callback (Clay     *clay,
                   gboolean  force)
{
  ClayGuiConfig *gui_config = CLAY_GUI_CONFIG (clay->config);
  ClayTool      *active_tool;

  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  if (! force && clay_displays_dirty (clay))
    {
      ClayContext *context = clay_get_user_context (clay);
      ClayDisplay *display = clay_context_get_display (context);
      GdkMonitor  *monitor = clay_get_monitor_at_pointer ();
      GtkWidget   *parent  = NULL;

      if (display)
        {
          ClayDisplayShell *shell = clay_display_get_shell (display);

          parent = GTK_WIDGET (clay_display_shell_get_window (shell));
        }

      clay_dialog_factory_dialog_raise (clay_dialog_factory_get_singleton (),
                                        monitor, parent, "clay-quit-dialog", -1);

      return TRUE; /* stop exit for now */
    }

  clay->message_handler = CLAY_CONSOLE;

  gui_unique_exit ();

  /* If any modifier is set when quitting (typically when exiting with
   * Ctrl-q for instance!), when serializing the tool options, it will
   * save any alternate value instead of the main one. Make sure that
   * any modifier is reset before saving options.
   */
  active_tool = tool_manager_get_active (clay);
  if  (active_tool && active_tool->focus_display)
    clay_tool_set_modifier_state  (active_tool, 0, active_tool->focus_display);

  if (gui_config->save_session_info)
    session_save (clay, FALSE);

  if (gui_config->save_device_status)
    clay_devices_save (clay, FALSE);

  if (TRUE /* gui_config->save_controllers */)
    {
      ClayControllerManager *controller_manager;

      controller_manager = clay_get_controller_manager (clay);
      clay_controller_manager_save (controller_manager);
    }

  modifiers_save (clay, FALSE);

  g_signal_handlers_disconnect_by_func (clay_get_user_context (clay),
                                        gui_display_changed,
                                        clay);

  clay_displays_delete (clay);

  if (gui_config->save_accels)
    menus_save (clay, FALSE);

  clay_tools_save (clay, gui_config->save_tool_options, FALSE);
  clay_tools_exit (clay);

  return FALSE; /* continue exiting */
}

static gboolean
gui_exit_after_callback (Clay     *clay,
                         gboolean  force)
{
  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_signal_handlers_disconnect_by_func (clay->config,
                                        gui_show_help_button_notify,
                                        clay);
  g_signal_handlers_disconnect_by_func (clay->config,
                                        gui_user_manual_notify,
                                        clay);

  clay_action_history_exit (clay);

  g_object_unref (ui_configurer);
  ui_configurer = NULL;

  /*  exit the clipboard before shutting down the GUI because it runs
   *  a whole lot of code paths. See bug #731389.
   */
  g_signal_handlers_disconnect_by_func (clay,
                                        G_CALLBACK (gui_clipboard_changed),
                                        NULL);
  clay_clipboard_exit (clay);

  session_exit (clay);
  menus_exit (clay);
  actions_exit (clay);
  clay_render_exit (clay);

  clay_controllers_exit (clay);
  modifiers_exit (clay);
  clay_devices_exit (clay);
  dialogs_exit (clay);
  themes_exit (clay);
  clay_canvas_styles_exit ();

  g_type_class_unref (g_type_class_peek (CLAY_TYPE_COLOR_SELECT));

  return FALSE; /* continue exiting */
}

static void
gui_show_help_button_notify (ClayGuiConfig *gui_config,
                             GParamSpec    *param_spec,
                             Clay          *clay)
{
  clay_dialogs_show_help_button (gui_config->use_help &&
                                 gui_config->show_help_button);
}

static void
gui_user_manual_notify (ClayGuiConfig *gui_config,
                        GParamSpec    *param_spec,
                        Clay          *clay)
{
  clay_help_user_manual_changed (clay);
}

static void
gui_single_window_mode_notify (ClayGuiConfig      *gui_config,
                               GParamSpec         *pspec,
                               ClayUIConfigurer   *ui_configurer)
{
  clay_ui_configurer_configure (ui_configurer,
                                gui_config->single_window_mode);
}

static void
gui_clipboard_changed (Clay *clay)
{
  if (clay_get_clipboard_image (clay))
    clay_clipboard_set_image (clay, clay_get_clipboard_image (clay));
  else
    clay_clipboard_set_buffer (clay, clay_get_clipboard_buffer (clay));
}

static void
gui_menu_show_tooltip (ClayUIManager *manager,
                       const gchar   *tooltip,
                       Clay          *clay)
{
  ClayContext *context = clay_get_user_context (clay);
  ClayDisplay *display = clay_context_get_display (context);

  if (display)
    {
      ClayDisplayShell *shell     = clay_display_get_shell (display);
      ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

      clay_statusbar_push (statusbar, "menu-tooltip",
                           NULL, "%s", tooltip);
    }
}

static void
gui_menu_hide_tooltip (ClayUIManager *manager,
                       Clay          *clay)
{
  ClayContext *context = clay_get_user_context (clay);
  ClayDisplay *display = clay_context_get_display (context);

  if (display)
    {
      ClayDisplayShell *shell     = clay_display_get_shell (display);
      ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);

      clay_statusbar_pop (statusbar, "menu-tooltip");
    }
}

static void
gui_display_changed (ClayContext *context,
                     ClayDisplay *display,
                     Clay        *clay)
{
  if (! display)
    {
      ClayImage *image = clay_context_get_image (context);

      if (image)
        {
          GList *list;

          for (list = clay_get_display_iter (clay);
               list;
               list = g_list_next (list))
            {
              ClayDisplay *display2 = list->data;

              if (clay_display_get_image (display2) == image)
                {
                  clay_context_set_display (context, display2);

                  /* stop the emission of the original signal
                   * (the emission of the recursive signal is finished)
                   */
                  g_signal_stop_emission_by_name (context, "display-changed");
                  return;
                }
            }

          clay_context_set_image (context, NULL);
        }
    }

  clay_ui_manager_update (menus_get_image_manager_singleton (clay),
                          display);
}

typedef struct
{
  const gchar     *path;
  guint            key;
  GdkModifierType  mods;
}
accelData;

static void
gui_check_unique_accelerators (Clay *clay)
{
  gchar **actions;

  actions = g_action_group_list_actions (G_ACTION_GROUP (clay->app));

  for (gint i = 0; actions[i] != NULL; i++)
    {
      gchar      **accels;
      gchar       *detailed_name;
      ClayAction  *action;
      gint         value;

      action = (ClayAction *) g_action_map_lookup_action (G_ACTION_MAP (clay->app), actions[i]);

      if (CLAY_IS_RADIO_ACTION (action))
        {
          g_object_get ((GObject *) action,
                        "value", &value,
                        NULL);
          detailed_name = g_strdup_printf ("app.%s(%i)", actions[i],
                                           value);
        }
      else
        {
          detailed_name = g_strdup_printf ("app.%s", actions[i]);
        }

      accels = gtk_application_get_accels_for_action (GTK_APPLICATION (clay->app),
                                                      detailed_name);
      g_free (detailed_name);

      for (gint j = 0; accels[j] != NULL; j++)
        {
          for (gint k = i + 1; actions[k] != NULL; k++)
            {
              gchar      **accels2;
              gchar       *detailed_name2;
              ClayAction  *action2;

              action2 = (ClayAction *) g_action_map_lookup_action (G_ACTION_MAP (clay->app), actions[k]);

              if (CLAY_IS_RADIO_ACTION (action2))
                {
                  g_object_get ((GObject *) action2,
                                "value", &value,
                                NULL);
                  detailed_name2 = g_strdup_printf ("app.%s(%i)", actions[k],
                                                    value);
                }
              else
                {
                  detailed_name2 = g_strdup_printf ("app.%s", actions[k]);
                }

              accels2 = gtk_application_get_accels_for_action (GTK_APPLICATION (clay->app),
                                                               detailed_name2);
              g_free (detailed_name2);

              for (gint l = 0; accels2[l] != NULL; l++)
                {
                  if (g_strcmp0 (accels[j], accels2[l]) == 0)
                    {
                      GAction  *action;
                      gchar    *disabled_action;
                      gchar   **disabled_accels;
                      gint      len;
                      gint      remove;
                      gboolean  print_warning = TRUE;

                      action = g_action_map_lookup_action (G_ACTION_MAP (clay->app),
                                                           actions[i]);
                      /* Just keep the first one (no reason other than we have
                       * to choose), unless it's a secondary shortcut, and the
                       * second is a primary shortcut.
                       */
                      if ((l == 0 && j != 0) ||
                          /* If the first action is one of "view-zoom-1-*" and
                           * the shortcut default, we assume it's because of our
                           * trick to transform `Shift+num` shortcuts based on
                           * layout and we happen to be on a layout where it
                           * clashes with other shortcuts. In this case, we
                           * drop the duplicate shortcut on the zoom action. See
                           * special code in
                           * clay_action_group_add_action_with_accel()
                           */
                          (g_str_has_prefix (actions[i], "view-zoom-1-") &&
                           clay_action_is_default_accel (CLAY_ACTION (action), accels[j])))
                        {
                          disabled_action = actions[i];
                          disabled_accels = accels;
                          remove = j;
                        }
                      else
                        {
                          disabled_action = actions[k];
                          disabled_accels = accels2;
                          remove = l;
                        }

                      action = g_action_map_lookup_action (G_ACTION_MAP (clay->app),
                                                           disabled_action);

                      if (g_str_has_prefix (disabled_action, "view-zoom-1-") &&
                          clay_action_is_default_accel (CLAY_ACTION (action), disabled_accels[remove]))
                        /* We drop the shortcut **silently** because it will be
                         * a case where we have 2 default accelerators clashing
                         * (because of the conversion code) while not being a
                         * real bug. Clashes with custom accelerators are
                         * handled by shortcuts_action_deserialize().
                         */
                        print_warning = FALSE;

                      /* Remove only the duplicate shortcut but keep others. */
                      len = g_strv_length (disabled_accels);
                      g_free (disabled_accels[remove]);
                      memmove (&disabled_accels[remove],
                               &disabled_accels[remove + 1],
                               sizeof (char *) * (len - remove));

                      if (print_warning)
                        g_printerr ("Actions \"%s\" and \"%s\" use the same accelerator.\n"
                                    "  Disabling the accelerator on \"%s\".\n",
                                    actions[i], actions[k], disabled_action);

                      clay_action_set_accels (CLAY_ACTION (action),
                                              (const gchar **) disabled_accels);
                    }
                }

              g_strfreev (accels2);
            }
        }

      g_strfreev (accels);
    }

  g_strfreev (actions);
}
