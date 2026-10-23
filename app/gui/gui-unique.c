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

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#import <AppKit/AppKit.h>
#endif

#include "gui/gui-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/clayimagewindow.h"

#include "file/file-open.h"

#include "menus/menus.h"

#include "widgets/clayuimanager.h"

#include "claydbusservice.h"
#include "gui-unique.h"
#include "themes.h"


#ifdef G_OS_WIN32

static void  gui_unique_win32_init (Clay *clay);
static void  gui_unique_win32_exit (void);

static Clay *unique_clay  = NULL;
static HWND  proxy_window = NULL;

#elif defined (GDK_WINDOWING_QUARTZ)

static void     gui_unique_quartz_init         (Clay *clay);
static void     gui_unique_quartz_exit         (void);
static gboolean gui_unique_quartz_trigger_quit (gpointer data);

@interface ClayAppleEventHandler : NSObject {}
- (void) handleEvent:(NSAppleEventDescriptor *) inEvent
        andReplyWith:(NSAppleEventDescriptor *) replyEvent;
@end

@interface ClayAppDelegate : NSObject <NSApplicationDelegate>
@end


static Clay                   *unique_clay                            = NULL;
static ClayAppleEventHandler  *event_handler                          = NULL;
static void                  (^themeChangeHandler) (NSNotification *) = NULL;

#else

static void  gui_dbus_service_init (Clay *clay);
static void  gui_dbus_service_exit (void);

static GDBusObjectManagerServer *dbus_manager = NULL;
static guint                     dbus_name_id = 0;

#endif


void
gui_unique_init (Clay *clay)
{
#ifdef G_OS_WIN32
  gui_unique_win32_init (clay);
#elif defined (GDK_WINDOWING_QUARTZ)
  gui_unique_quartz_init (clay);
#else
  gui_dbus_service_init (clay);
#endif
}

void
gui_unique_exit (void)
{
#ifdef G_OS_WIN32
  gui_unique_win32_exit ();
#elif defined (GDK_WINDOWING_QUARTZ)
  gui_unique_quartz_exit ();
#else
  gui_dbus_service_exit ();
#endif
}


#ifdef G_OS_WIN32

typedef struct
{
  GFile    *file;
  gboolean  as_new;
} IdleOpenData;

static IdleOpenData *
idle_open_data_new (GFile    *file,
                    gboolean  as_new)
{
  IdleOpenData *data = g_slice_new0 (IdleOpenData);

  data->file   = g_object_ref (file);
  data->as_new = as_new;

  return data;
}

static void
idle_open_data_free (IdleOpenData *data)
{
  g_object_unref (data->file);
  g_slice_free (IdleOpenData, data);
}

static gboolean
gui_unique_win32_idle_open (IdleOpenData *data)
{
  /*  We want to be called again later in case that Clay is not fully
   *  started yet.
   */
  if (! clay_is_restored (unique_clay))
    return TRUE;

  if (data->file)
    {
      file_open_from_command_line (unique_clay, data->file,
                                   data->as_new, NULL);
    }
  else
    {
      /*  raise the first display  */
      ClayObject *display;

      display = clay_container_get_first_child (unique_clay->displays);

      clay_display_shell_present (clay_display_get_shell (CLAY_DISPLAY (display)));
    }

  return FALSE;
}

static LRESULT CALLBACK
gui_unique_win32_message_handler (HWND   hWnd,
                                  UINT   uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_COPYDATA:
      if (unique_clay)
        {
          COPYDATASTRUCT *copydata = (COPYDATASTRUCT *) lParam;
          ClayObject     *display;

          if (copydata->cbData > 0)
            {
              GSource        *source;
              GClosure       *closure;
              GFile          *file;
              IdleOpenData   *data;

              file = g_file_new_for_uri (copydata->lpData);

              data = idle_open_data_new (file,
                                         copydata->dwData != 0);

              g_object_unref (file);

              closure = g_cclosure_new (G_CALLBACK (gui_unique_win32_idle_open),
                                        data,
                                        (GClosureNotify) idle_open_data_free);

              g_object_watch_closure (G_OBJECT (unique_clay), closure);

              /* be patient on opening to avoid stuckness */
              source = g_timeout_source_new (50);
              g_source_set_priority (source, G_PRIORITY_LOW);
              g_source_set_closure (source, closure);
              g_source_attach (source, NULL);
              g_source_unref (source);
            }

          /* Deiconify the window if minimized. */
          display = clay_container_get_first_child (unique_clay->displays);
          if (display)
            clay_display_shell_present (clay_display_get_shell (CLAY_DISPLAY (display)));
        }
      return TRUE;

    case WM_SETTINGCHANGE:
      /* This message is not about the unique GUI code, but we reuse the
       * existing top-level (hidden) window used for receiving messages
       * for other purposes too, such as color scheme in this case.
       * See !2308.
       */
      if (lParam != 0 && lstrcmpW((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)
        {
          themes_theme_change_notify (CLAY_GUI_CONFIG (unique_clay->config), NULL, unique_clay);
          return 0;
        }
      break;
    }

    return DefWindowProcW (hWnd, uMsg, wParam, lParam);
}

static void
gui_unique_win32_init (Clay *clay)
{
  WNDCLASSW wc;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (unique_clay == NULL);

  unique_clay = clay;

  /* register window class for proxy window */
  memset (&wc, 0, sizeof (wc));

  wc.hInstance     = GetModuleHandleW (NULL);
  wc.lpfnWndProc   = gui_unique_win32_message_handler;
  wc.lpszClassName = CLAY_UNIQUE_WIN32_WINDOW_CLASS;

  RegisterClassW (&wc);

  proxy_window = CreateWindowExW (0,
                                  CLAY_UNIQUE_WIN32_WINDOW_CLASS,
                                  CLAY_UNIQUE_WIN32_WINDOW_NAME,
                                  WS_POPUP, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);
}

static void
gui_unique_win32_exit (void)
{
  g_return_if_fail (CLAY_IS_CLAY (unique_clay));

  unique_clay = NULL;

  DestroyWindow (proxy_window);
}

#elif defined (GDK_WINDOWING_QUARTZ)

static gboolean
gui_unique_quartz_idle_open (GFile *file)
{
  /*  We want to be called again later in case that Clay is not fully
   *  started yet.
   */
  if (! clay_is_restored (unique_clay))
    return TRUE;

  if (file)
    {
      file_open_from_command_line (unique_clay, file, FALSE, NULL);
    }

  return FALSE;
}

@implementation ClayAppleEventHandler
- (void) handleEvent: (NSAppleEventDescriptor *) inEvent
        andReplyWith: (NSAppleEventDescriptor *) replyEvent
{
  NSAutoreleasePool      *urlpool;
  NSInteger               count;
  NSInteger               i;
  NSAppleEventDescriptor *filesList;

  urlpool = [[NSAutoreleasePool alloc] init];

  filesList = [inEvent paramDescriptorForKeyword:keyDirectObject];
  g_return_if_fail (filesList);

  count = [filesList numberOfItems];

  for (i = 1; i <= count; i++)
    {
      const gchar            *path;
      GSource                *source;
      GClosure               *closure;
      NSAppleEventDescriptor *fileDescriptor;
      NSString               *filePath;
      NSURL                  *fileURL;

      fileDescriptor = [filesList descriptorAtIndex:i];
      filePath = [[fileDescriptor stringValue] stringByResolvingSymlinksInPath];
      if (filePath)
        {
          fileURL = [NSURL URLWithString:filePath];
          if ([fileURL isFileURL])
            {
              path = [[fileURL path] UTF8String];
            }
          else
            {
              g_printerr ("Descriptor is not a valid file URL: %s\n", [filePath UTF8String]);
              continue;
            }
        }
      else
        {
          g_printerr ("Could not resolve file path for descriptor\n");
          continue;
        }

      closure = g_cclosure_new (G_CALLBACK (gui_unique_quartz_idle_open),
                                g_file_new_for_path (path), (GClosureNotify) g_object_unref);

      g_object_watch_closure (G_OBJECT (unique_clay), closure);

      /* be patient on opening to avoid stuckness */
      source = g_timeout_source_new (50);
      g_source_set_priority (source, G_PRIORITY_LOW);
      g_source_set_closure (source, closure);
      g_source_attach (source, NULL);
      g_source_unref (source);
    }

  [urlpool drain];
}
@end

@implementation ClayAppDelegate
- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *) sender
{
    g_idle_add ((GSourceFunc) gui_unique_quartz_trigger_quit, unique_clay);

    return NSTerminateCancel;
}
@end

static void
gui_unique_quartz_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (unique_clay == NULL);

  unique_clay = clay;

  themeChangeHandler = ^(NSNotification *notification)
  {
    themes_theme_change_notify (CLAY_GUI_CONFIG (unique_clay->config), NULL, unique_clay);
  };

  /* Observe macOS theme changes (Light, Dark) via distributed notifications.
   * When the "AppleInterfaceThemeChangedNotification" fires, the block runs
   * on the main thread (via mainQueue), calling themes_theme_change_notify()
   * to update Clay's UI theme accordingly.
   */
  [[NSDistributedNotificationCenter defaultCenter]
    addObserverForName: @"AppleInterfaceThemeChangedNotification"
                object: nil
                 queue: [NSOperationQueue mainQueue]
            usingBlock: themeChangeHandler];

  /* same as above but for the system accent color */
  [[NSNotificationCenter defaultCenter]
    addObserverForName: NSSystemColorsDidChangeNotification
                object: nil
                 queue: [NSOperationQueue mainQueue]
            usingBlock: themeChangeHandler];

  /* Using the event handler is a hack, it is necessary because
   * gtkosx_application will drop the file open events if any
   * event processing is done before gtkosx_application_ready is
   * called, which we unfortuantly can't avoid doing right now.
   */
  event_handler = [[ClayAppleEventHandler alloc] init];

  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler: event_handler
          andSelector: @selector (handleEvent: andReplyWith:)
        forEventClass: kCoreEventClass
           andEventID: kAEOpenDocuments];

  /* When quitting the application using "Quit" from the dock's right-click menu,
   * Clay does not follow our standard quit procedure. Instead, macOS forces the
   * application to close, which may result in losing unsaved changes.
   * This delegate intercepts the applicationShouldTerminate call and uses our
   * existing quit code, preventing macOS from handling the shutdown directly. */
  [NSApp setDelegate:[[ClayAppDelegate alloc] init]];
}

static void
gui_unique_quartz_exit (void)
{
  g_return_if_fail (CLAY_IS_CLAY (unique_clay));

  unique_clay = NULL;

  [[NSAppleEventManager sharedAppleEventManager]
      removeEventHandlerForEventClass: kCoreEventClass
                           andEventID: kAEOpenDocuments];

  [[NSDistributedNotificationCenter defaultCenter]
    removeObserver: themeChangeHandler
              name: @"AppleInterfaceThemeChangedNotification"
            object: nil];

  [[NSNotificationCenter defaultCenter]
    removeObserver: themeChangeHandler
              name: NSSystemColorsDidChangeNotification
            object: nil];

  themeChangeHandler = NULL;
  [event_handler release];

  event_handler = NULL;
}

static gboolean
gui_unique_quartz_trigger_quit (gpointer data)
{
  Clay          *clay       = (Clay *)data;
  ClayUIManager *ui_manager = menus_get_image_manager_singleton (clay);

  clay_ui_manager_activate_action (ui_manager, "file", "file-quit");

  return FALSE;
}

#else

static void
gui_dbus_bus_acquired (GDBusConnection *connection,
                       const gchar     *name,
                       Clay            *clay)
{
  GDBusObjectSkeleton *object;
  GObject             *service;

  /* this should use CLAY_DBUS_SERVICE_PATH, but that's historically wrong */
  dbus_manager = g_dbus_object_manager_server_new ("/org/clay/Clay");

  object = g_dbus_object_skeleton_new (CLAY_DBUS_INTERFACE_PATH);

  service = clay_dbus_service_new (clay);
  g_dbus_object_skeleton_add_interface (object,
                                        G_DBUS_INTERFACE_SKELETON (service));
  g_object_unref (service);

  g_dbus_object_manager_server_export (dbus_manager, object);
  g_object_unref (object);

  g_dbus_object_manager_server_set_connection (dbus_manager, connection);
}

static void
gui_dbus_name_acquired (GDBusConnection *connection,
                        const gchar     *name,
                        Clay            *clay)
{
}

static void
gui_dbus_name_lost (GDBusConnection *connection,
                    const gchar     *name,
                    Clay            *clay)
{
  if (connection == NULL)
    g_printerr ("%s: connection to the bus cannot be established.\n",
                G_STRFUNC);
  else
    g_printerr ("%s: the name \"%s\" could not be acquired on the bus.\n",
                G_STRFUNC, name);
}

static void
gui_dbus_service_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (dbus_name_id == 0);

  dbus_name_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                 CLAY_DBUS_SERVICE_NAME,
                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                 (GBusAcquiredCallback) gui_dbus_bus_acquired,
                                 (GBusNameAcquiredCallback) gui_dbus_name_acquired,
                                 (GBusNameLostCallback) gui_dbus_name_lost,
                                 clay, NULL);
}

static void
gui_dbus_service_exit (void)
{
  g_bus_unown_name (dbus_name_id);
  g_clear_object (&dbus_manager);
}

#endif
