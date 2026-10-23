/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <gdk/gdkwin32.h>
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <unistd.h>
#include <libproc.h>
#include <Cocoa/Cocoa.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "clay.h"
#include "clayui.h"

#include "libclaymodule/claymodule.h"

#include "libclaywidgets/claywidgets.h"
#include "libclaywidgets/claywidgets-private.h"


/**
 * SECTION: clayui
 * @title: clayui
 * @short_description: Common user interface functions. This header includes
 *                     all other Clay User Interface Library headers.
 * @see_also: gtk_init(), gdk_set_use_xshm(), gtk_widget_set_default_visual().
 *
 * Common user interface functions. This header includes all other
 * Clay User Interface Library headers.
 **/


/*  local function prototypes  */

static void        clay_ui_help_func               (const gchar       *help_id,
                                                    gpointer           help_data);
static void        clay_ui_theme_changed           (GFileMonitor      *monitor,
                                                    GFile             *file,
                                                    GFile             *other_file,
                                                    GFileMonitorEvent  event_type,
                                                    GtkCssProvider    *css_provider);
static void        clay_ensure_modules             (void);

#ifdef GDK_WINDOWING_QUARTZ
static gboolean    clay_osx_focus_window           (gpointer);

static void        clay_osx_display_callback       (CFNotificationCenterRef  center,
                                                    void                    *observer,
                                                    CFNotificationName       name,
                                                    const void              *object,
                                                    CFDictionaryRef          user_info)
{
  /* sent by the Clay main process, see clay_image_window_window_state_event() */
  if (CFStringCompare (name, CFSTR ("Clay-" CLAY_MUTEX_VERSION ".window-miniaturized"), 0) == kCFCompareEqualTo)
    {
      for (NSWindow *win in [NSApp windows])
        [win orderOut:nil];
    }
  else
    {
      for (NSWindow *win in [NSApp windows])
        [win orderFront:nil];
    }
}
#endif

#ifdef GDK_WINDOWING_WIN32
static DWORD plugin_pid      = 0;
static DWORD clay_pid        = 0;
static HWND  clay_hwnd       = NULL;
static HWND  clay_extra_hwnd = NULL;

static void
clay_win32_set_visible (gboolean visible)
{
  GList *plugin_win = gtk_window_list_toplevels ();
  GList *list;

  for (list = plugin_win; list; list = g_list_next (list))
    {
      GtkWidget *win = list->data;
      HWND       hwnd_window;

      if (! gtk_widget_get_mapped (win))
        continue;

      hwnd_window = (HWND) gdk_win32_window_get_handle (gtk_widget_get_window (GTK_WIDGET (win)));

      if (visible)
        {
          /* clay main window is active, show dialog on top always */
          ShowWindow (hwnd_window, SW_SHOWNOACTIVATE);
          SetWindowPos (hwnd_window, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
      else
        {
          /* clay main window is not active, hide dialog */
          ShowWindow (hwnd_window, SW_HIDE);
        }
    }

  g_list_free (plugin_win);
}

static gboolean
clay_win32_is_plugin_from_plugin (DWORD pid)
{
  /* a plug-in cannot spawn another plug-in directly: every plug-in process,
    no matter where the dialog was launched from, is a direct child of clay core.
    e.g. a metadata editor opened from an export dialog. See: #6223 */
  HANDLE         process_list;
  PROCESSENTRY32 process;
  gboolean       is_child = FALSE;

  process_list = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
  if (process_list == INVALID_HANDLE_VALUE)
    return FALSE;

  process.dwSize = sizeof (PROCESSENTRY32);

  if (Process32First (process_list, &process))
    {
      do
        {
          if (process.th32ProcessID == pid)
            {
              is_child = (process.th32ParentProcessID == clay_pid);
              break;
            }
        }
      while (Process32Next (process_list, &process));
    }

  CloseHandle (process_list);

  return is_child;
}

static gboolean
clay_win32_reorder_core_dialog (gpointer hwnd)
{
  /* on its first appearance, a clay core window (help dialog, resource
     chooser, etc.) can still be mid-setup. Try to put it on top twice. */
  if ((HWND) hwnd == clay_extra_hwnd)
    {
      SetWindowPos ((HWND) hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      SetForegroundWindow ((HWND) hwnd);
    }

  return G_SOURCE_REMOVE;
}

static void
clay_win32_display_callback (HWINEVENTHOOK hook,
                             DWORD         event,
                             HWND          hwnd,
                             LONG          id_object,
                             LONG          id_child,
                             DWORD         event_thread,
                             DWORD         event_time)
{
  /* SetWinEventHook() gives no per-window refcon like AXObserverAddNotification() does */
  DWORD pid = 0;

  if (id_object != OBJID_WINDOW || hwnd == NULL)
    return;

  GetWindowThreadProcessId (hwnd, &pid);

  switch (event)
    {
    case EVENT_SYSTEM_MINIMIZESTART:
      if (pid == clay_pid)
        clay_win32_set_visible (FALSE);
      break;
    case EVENT_SYSTEM_MINIMIZEEND:
      if (pid == clay_pid)
        clay_win32_set_visible (TRUE);
      break;
    case EVENT_SYSTEM_FOREGROUND:
      /* show when clay or one of our own plug-in windows is activated;
         hide otherwise, or we would stay on top of other apps */
      if (pid != clay_pid && pid != plugin_pid &&
          ! clay_win32_is_plugin_from_plugin (pid))
        {
          clay_win32_set_visible (FALSE);

          if (clay_extra_hwnd)
            {
              /* hide clay help or clay resource dialog */
              SetWindowPos (clay_extra_hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
              clay_extra_hwnd = NULL;
            }
        }
      else if (hwnd == clay_hwnd || pid == plugin_pid)
        {
          clay_win32_set_visible (TRUE);
        }
      else
        {
          /* show clay help or clay resource dialog */
          clay_extra_hwnd = hwnd;
          SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
          SetForegroundWindow (hwnd);
          g_timeout_add (200, clay_win32_reorder_core_dialog, hwnd);
        }
      break;
    default:
      break;
    }
}
#endif

#if !defined(GDK_WINDOWING_QUARTZ)
static GdkWindow * clay_ui_get_foreign_window      (gpointer           window);
#endif
static gboolean    clay_window_transient_on_mapped (GtkWidget         *window,
                                                    GdkEventAny       *event,
                                                    GBytes            *handle);


static gboolean clay_ui_initialized = FALSE;


/*  public functions  */

/**
 * clay_ui_init:
 * @prog_name: The name of the plug-in which will be passed as argv[0] to
 *             `gtk_init()`. It's a convention to use the name of the
 *             executable and _not_ the PDB procedure name.
 *
 * This function initializes GTK with [func@Gtk.init], as well as GEGL and
 * babl (non-interactive plug-ins should use only [func@Gegl.init] instead).
 *
 * It also sets up various other things so that the plug-in user looks
 * and behaves like the Clay core. This includes selecting the GTK theme
 * and setting up the help system as chosen in Clay preferences. Any
 * plug-in that provides a user interface should call this function.
 *
 * It can safely be called more than once. Calls after the first return
 * quickly with no effect.
 **/
void
clay_ui_init (const gchar *prog_name)
{
  const gchar    *display_name;
  GtkCssProvider *css_provider;
  GFileMonitor   *css_monitor;
  GFile          *file;

  g_return_if_fail (prog_name != NULL);

  if (clay_ui_initialized)
    return;

  g_set_prgname (prog_name);

  display_name = clay_display_name ();

  if (display_name)
    {
#if defined (GDK_WINDOWING_X11)
      g_setenv ("DISPLAY", display_name, TRUE);
#else
      g_setenv ("GDK_DISPLAY", display_name, TRUE);
#endif
    }

  if (clay_user_time ())
    {
      /* Construct a fake startup ID as we only want to pass the
       * interaction timestamp, see _gdk_windowing_set_default_display().
       */
      gchar *startup_id = g_strdup_printf ("_TIME%u", clay_user_time ());

      g_setenv ("DESKTOP_STARTUP_ID", startup_id, TRUE);
      g_free (startup_id);
    }

  gtk_init (NULL, NULL);

#ifdef GDK_WINDOWING_QUARTZ
  /* THIS CODE SHOULD BE POSITIONED ALWAYS AFTER gtk_init() !!!
   *
   * Sets activation policy to prevent plugins from appearing as separate apps
   * in Dock. See: #12150
   * Makes plugins behave as helper processes of Clay on macOS.
   */
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
#endif

  css_provider = gtk_css_provider_new ();
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  file = clay_directory_file ("theme.css", NULL);
  css_monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_object_unref (file);

  clay_ui_theme_changed (css_monitor, NULL, NULL, G_FILE_MONITOR_EVENT_CHANGED,
                         css_provider);

  g_signal_connect (css_monitor, "changed",
                    G_CALLBACK (clay_ui_theme_changed),
                    css_provider);

  g_object_unref (css_provider);

  gdk_set_program_class (clay_wm_class ());

  if (clay_icon_theme_dir ())
    {
      file = g_file_new_for_path (clay_icon_theme_dir ());
      clay_icons_set_icon_theme (file);
      g_object_unref (file);
    }

  clay_widgets_init (clay_ui_help_func,
                     clay_context_get_foreground,
                     clay_context_get_background,
                     clay_ensure_modules, NULL);

  clay_dialogs_show_help_button (clay_show_help_button ());

#ifdef GDK_WINDOWING_QUARTZ
  g_idle_add (clay_osx_focus_window, NULL);
#endif

  /* Some widgets use GEGL buffers for thumbnails, previews, etc. */
  gegl_init (NULL, NULL);

  clay_ui_initialized = TRUE;
}

#ifdef GDK_WINDOWING_QUARTZ
static void
clay_window_transient_show (GtkWidget *window)
{
  g_signal_handlers_disconnect_by_func (window,
                                        clay_window_transient_show,
                                        NULL);
  [NSApp arrangeInFront: nil];
}
#endif

/**
 * clay_window_set_transient_for:
 * @window: the #GtkWindow that should become transient
 * @handle: handle of the window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * to the window identified by @handle.
 *
 * Note that @handle is an opaque data, which you should not try to
 * construct yourself or make sense of. It may be different things
 * depending on the OS or even the display server. You should only use
 * a handle returned by [func@Clay.progress_get_window_handle],
 * [method@Clay.Display.get_window_handle] or
 * [method@ClayUi.Dialog.get_native_handle].
 *
 * Most of the time you will want to use the convenience function
 * [func@ClayUi.window_set_transient].
 *
 * Since: 3.0
 */
void
clay_window_set_transient_for (GtkWindow *window,
                               GBytes    *handle)
{
  g_return_if_fail (clay_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (handle != NULL);

#if defined (GDK_WINDOWING_WIN32) || defined (GDK_WINDOWING_QUARTZ)
  /* Windows and macOS don't use real cross-process transiency, so the
     plug-in won't be on the right monitor. Center it before it is mapped
     (on macOS this also avoids a top-left render glitch on plug-ins). */
  gtk_window_set_position (window, GTK_WIN_POS_CENTER);
#endif

  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        clay_window_transient_on_mapped,
                                        NULL);

  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (clay_window_transient_on_mapped),
                         g_bytes_ref (handle),
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    clay_window_transient_on_mapped (GTK_WIDGET (window), NULL, handle);
}

/**
 * clay_window_set_transient_for_display:
 * @window:  the #GtkWindow that should become transient
 * @display: display of the image window that should become the parent
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the Clay image window that is identified by its
 * display. See [method@Gdk.Window.set_transient_for] for more information.
 *
 * Most of the time you will want to use the convenience function
 * [func@ClayUi.window_set_transient].
 *
 * Since: 2.4
 */
void
clay_window_set_transient_for_display (GtkWindow   *window,
                                       ClayDisplay *display)
{
  GBytes *handle;

  g_return_if_fail (clay_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (CLAY_IS_DISPLAY (display));

#if defined (GDK_WINDOWING_WIN32) || defined (GDK_WINDOWING_QUARTZ)
  /* Windows and macOS don't use real cross-process transiency, so the
     plug-in won't be on the right monitor. Center it before it is mapped
     (on macOS this also avoids a top-left render glitch on plug-ins). */
  gtk_window_set_position (window, GTK_WIN_POS_CENTER);
#endif

  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        clay_window_transient_on_mapped,
                                        NULL);

  handle = clay_display_get_window_handle (display);
  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (clay_window_transient_on_mapped),
                         handle,
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    clay_window_transient_on_mapped (GTK_WIDGET (window), NULL, handle);
}

/**
 * clay_window_set_transient:
 * @window: the #GtkWindow that should become transient
 *
 * Indicates to the window manager that @window is a transient dialog
 * associated with the Clay window that the plug-in has been
 * started from. See also [func@ClayUi.window_set_transient_for_display].
 *
 * Since: 2.4
 */
void
clay_window_set_transient (GtkWindow *window)
{
  GBytes *handle;

  g_return_if_fail (clay_ui_initialized);
  g_return_if_fail (GTK_IS_WINDOW (window));

#if defined (GDK_WINDOWING_WIN32) || defined (GDK_WINDOWING_QUARTZ)
  /* Windows and macOS don't use real cross-process transiency, so the
     plug-in won't be on the right monitor. Center it before it is mapped
     (on macOS this also avoids a top-left render glitch on plug-ins). */
  gtk_window_set_position (window, GTK_WIN_POS_CENTER);
#endif

  g_signal_handlers_disconnect_matched (window, G_SIGNAL_MATCH_FUNC,
                                        0, 0, NULL,
                                        clay_window_transient_on_mapped,
                                        NULL);

  handle = clay_progress_get_window_handle ();
  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (clay_window_transient_on_mapped),
                         handle,
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    clay_window_transient_on_mapped (GTK_WIDGET (window), NULL, handle);
}


/*  private functions  */

static void
clay_ui_help_func (const gchar *help_id,
                   gpointer     help_data)
{
#ifdef GDK_WINDOWING_QUARTZ
  NSRunningApplication *clay_app;
#endif

  clay_help (NULL, help_id);

#ifdef GDK_WINDOWING_QUARTZ
  /* activate the parent Clay process so the app/widgets/clayhelp.c dialog appears on top */
  clay_app = [NSRunningApplication runningApplicationWithProcessIdentifier:getppid()];
  if (clay_app)
    {
      if (@available(macOS 14.0, *))
        {
          [[NSApplication sharedApplication] yieldActivationToApplication:clay_app];
          [clay_app activateFromApplication:[NSRunningApplication currentApplication] options:0];
        }
      else
        {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
          [clay_app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
#pragma clang diagnostic pop
        }
    }
#endif
}

static void
clay_ui_theme_changed (GFileMonitor      *monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       GtkCssProvider    *css_provider)
{
  GError *error = NULL;
  gchar  *contents;

  file = clay_directory_file ("theme.css", NULL);

  if (g_file_load_contents (file, NULL, &contents, NULL, NULL, &error))
    {
      gboolean prefer_dark_theme;

      prefer_dark_theme = strstr (contents, "/* prefer-dark-theme */") != NULL;

      g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                    "gtk-application-prefer-dark-theme", prefer_dark_theme,
                    NULL);

      g_free (contents);
    }
  else
    {
      g_printerr ("%s: error loading %s: %s\n", G_STRFUNC,
                  clay_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  if (! gtk_css_provider_load_from_file (css_provider, file, &error))
    {
      g_printerr ("%s: error parsing %s: %s\n", G_STRFUNC,
                  clay_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

static void
clay_ensure_modules (void)
{
  static ClayModuleDB *module_db = NULL;

  if (! module_db)
    {
      gchar *load_inhibit = clay_get_module_load_inhibit ();
      gchar *module_path  = clay_clayrc_query ("module-path");

      module_db = clay_module_db_new (FALSE);

      clay_module_db_set_load_inhibit (module_db, load_inhibit);
      clay_module_db_load (module_db, module_path);

      g_free (module_path);
      g_free (load_inhibit);
    }
}

#ifdef GDK_WINDOWING_QUARTZ
static gboolean
clay_osx_focus_window (gpointer user_data)
{
  [NSApp activateIgnoringOtherApps:YES];
  return FALSE;
}
#endif

#if !defined(GDK_WINDOWING_QUARTZ)
static GdkWindow *
clay_ui_get_foreign_window (gpointer window)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (Window) window);
#endif

#ifdef GDK_WINDOWING_WIN32
  return gdk_win32_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (HWND) window);
#endif

  return NULL;
}
#endif

static gboolean
clay_window_transient_on_mapped (GtkWidget   *window,
                                 GdkEventAny *event,
                                 GBytes      *handle)
{
  gboolean transient_set = FALSE;
#ifdef GDK_WINDOWING_WIN32
  static HWINEVENTHOOK clay_app_min;
  static HWINEVENTHOOK clay_app_res;
#endif
#ifdef GDK_WINDOWING_QUARTZ
  NSArray        *plugin_win        = [NSApp windows];
  pid_t           plugin_pid        = getpid();
  pid_t           clay_pid          = getppid();
  static gboolean clay_app_observer;
  static gboolean not_clay_observer_added;
#endif

  if (handle == NULL)
    return FALSE;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    {
      char *wayland_handle;

      wayland_handle = (char *) g_bytes_get_data (handle, NULL);
      gdk_wayland_window_set_transient_for_exported (gtk_widget_get_window (window),
                                                     wayland_handle);
      transient_set = TRUE;
    }
#endif

#ifdef GDK_WINDOWING_X11
  if (! transient_set && GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    {
      GdkWindow *parent;
      Window    *handle_data;
      Window     parent_ID;
      gsize      handle_size;

      handle_data = (Window *) g_bytes_get_data (handle, &handle_size);
      g_return_val_if_fail (handle_size == sizeof (Window), FALSE);
      parent_ID = *handle_data;

      parent = clay_ui_get_foreign_window ((gpointer) parent_ID);

      if (parent)
        gdk_window_set_transient_for (gtk_widget_get_window (window), parent);

      transient_set = TRUE;
    }
#endif
  /* Windows does support cross-process transiency, but calling
   * gdk_window_set_transient_for() (aka SetWindowLongPtr (GWLP_HWNDPARENT))
   * hangs Clay and the plug-in, see clay_window_set_transient_cb() in
   * app/widgets/claywidgets-utils.c. So we emulate it with WinEvent API.
   */
#ifdef GDK_WINDOWING_WIN32
  if (! transient_set)
    {
      GdkWindow *parent;
      HANDLE    *handle_data;
      HANDLE     parent_ID;
      gsize      handle_size;

      handle_data = (HANDLE *) g_bytes_get_data (handle, &handle_size);
      g_return_val_if_fail (handle_size == sizeof (HANDLE), FALSE);
      parent_ID = *handle_data;

      parent = clay_ui_get_foreign_window ((gpointer) parent_ID);

      if (parent)
        {
          /* see: #10229 */
          /* gdk_window_set_transient_for (gtk_widget_get_window (window), parent); */

          plugin_pid = GetCurrentProcessId ();
          clay_hwnd  = (HWND) parent_ID;
          GetWindowThreadProcessId (clay_hwnd, &clay_pid);

          /* first, set all plug-in windows as always visible */
          clay_win32_set_visible (TRUE);

          /* if clay app is activated or deactivated, or minimized or
             restored, do the same on the plug-in windows */
          if (! clay_app_min && ! clay_app_res)
            {
              clay_app_min =
                SetWinEventHook (EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND,
                                 NULL, clay_win32_display_callback,
                                 0, 0, WINEVENT_OUTOFCONTEXT);
              clay_app_res =
                SetWinEventHook (EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                                 NULL, clay_win32_display_callback,
                                 0, 0, WINEVENT_OUTOFCONTEXT);
            }
        }
    }
#endif

  if (! transient_set)
    {
      /*  if setting the window transient failed, at least set
       *  WIN_POS_CENTER, which will center the window on the screen
       *  where the mouse is (see bug #684003).
       */
      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

#ifdef GDK_WINDOWING_QUARTZ
      /*  macOS does not support cross-process trasiency so we use NotificationCenter API*/
      for (NSWindow *win in plugin_win)
        {
          /* first, set all plug-in windows as always visible */
          [win setLevel:NSFloatingWindowLevel];
        }

      /* if clay app is minimzed or restored, do the same on the plug-in windows
         otherwise the windows would stay always visible over other apps */
      if (! clay_app_observer)
        {
          CFStringRef clay_pid_str = CFStringCreateWithFormat (NULL, NULL, CFSTR ("%d"), clay_pid);
          CFNotificationCenterAddObserver (CFNotificationCenterGetDistributedCenter (),
                                           &clay_app_observer,
                                           clay_osx_display_callback,
                                           CFSTR ("Clay-" CLAY_MUTEX_VERSION ".window-miniaturized"),
                                           clay_pid_str,
                                           CFNotificationSuspensionBehaviorDeliverImmediately);
          CFNotificationCenterAddObserver (CFNotificationCenterGetDistributedCenter (),
                                           &clay_app_observer,
                                           clay_osx_display_callback,
                                           CFSTR ("Clay-" CLAY_MUTEX_VERSION ".window-deminiaturized"),
                                           clay_pid_str,
                                           CFNotificationSuspensionBehaviorDeliverImmediately);
          CFRelease (clay_pid_str);
          clay_app_observer = TRUE;
        }

      /* if clay app is hidden or show, do the same on the plug-in windows
         (this is NOT the same as minimization/restoration of clay app) */
      if (! not_clay_observer_added)
        {
          /* clay main window is not active, save last focused window/dialog */
          static BOOL focus_saved = NO;
          [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceDidDeactivateApplicationNotification
                                                              object:nil
                                                              queue:[NSOperationQueue mainQueue]
                                                              usingBlock:^(NSNotification *note) {
            NSRunningApplication *deactivated_app = [[note userInfo] objectForKey:NSWorkspaceApplicationKey];
            pid_t                 deactivated_pid = 0;
            struct proc_bsdinfo   deactivated_info;
            if (!deactivated_app)
              return;
            deactivated_pid = [deactivated_app processIdentifier];
            if (deactivated_pid == plugin_pid)
              focus_saved = YES;
            /* take into account other plug-ins too */
            else if (deactivated_pid == clay_pid ||
                     (proc_pidinfo (deactivated_pid, PROC_PIDTBSDINFO, 0, &deactivated_info, sizeof (deactivated_info)) == sizeof (deactivated_info) &&
                      deactivated_info.pbi_ppid == clay_pid))
              focus_saved = NO;
          }];

          /* clay main window is active, show dialog focused or not */
          [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceDidActivateApplicationNotification
                                                              object:nil
                                                              queue:[NSOperationQueue mainQueue]
                                                              usingBlock:^(NSNotification *note) {

            NSRunningApplication *not_clay_app       = [[note userInfo] objectForKey:NSWorkspaceApplicationKey];
            pid_t                 not_clay_pid       = 0;
            struct proc_bsdinfo   not_clay_info;
            BOOL                  not_clay           = NO;
            static BOOL           not_clay_activated = NO;
            BOOL                  focus_restore      = NO;
            if (!not_clay_app)
              return;
            not_clay_pid = [not_clay_app processIdentifier];
            /* take into account other plug-ins too */
            if (proc_pidinfo (not_clay_pid, PROC_PIDTBSDINFO, 0, &not_clay_info, sizeof (not_clay_info)) == sizeof (not_clay_info) &&
                not_clay_info.pbi_ppid == clay_pid)
              return;
            not_clay = (not_clay_pid != clay_pid && not_clay_pid != plugin_pid);
            if (not_clay)
              {
                not_clay_activated = YES;
              }
            else
              {
                /* only give focus back to the plug-in when coming back from
                 * another app, not when the user clicked the main window */
                focus_restore = focus_saved && not_clay_activated;
                not_clay_activated = NO;
              }
            /* we fetch win again rather than using stale 'plugin_win' due to
             * extension/persistent plug-in (e.g. old-style script-fu) */
            for (NSWindow *win in [NSApp windows])
              {
                if (not_clay)
                  {
                    /* clay main window is not active, hide dialog */
                    [win orderOut:nil];
                  }
                else
                  {
                    /* clay main window is active, show dialog on top always */
                    if (!focus_restore)
                      {
                        [win orderFront:nil];
                      }
                    else
                      {
                        /* we use 0.1 NSEC_PER_SEC to avoid interleaving windows from other apps */
                        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                          [NSApp activateIgnoringOtherApps:YES];
                          [win makeKeyAndOrderFront:nil];
                        });
                      }
                  }
              }
          }];
          not_clay_observer_added = TRUE;
        }

      g_signal_connect (window, "show",
                        G_CALLBACK (clay_window_transient_show),
                        NULL);
#endif
    }

  return FALSE;
}
