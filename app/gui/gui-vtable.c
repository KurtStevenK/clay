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
#include <errno.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#ifndef pipe
#define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif
#define read _read
#define close _close
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "gui-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-parallel.h"
#include "core/clay-spawn.h"
#include "core/clay-utils.h"
#include "core/clayasync.h"
#include "core/claybrush.h"
#include "core/claycancelable.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/claydrawable.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/clayimagefile.h"
#include "core/claylist.h"
#include "core/claypalette.h"
#include "core/claypattern.h"
#include "core/clayprogress.h"
#include "core/claywaitable.h"

#include "text/clayfont.h"

#include "pdb/claypdb.h"
#include "pdb/clayprocedure.h"

#include "plug-in/claypluginmanager-file.h"

#include "widgets/clayactiongroup.h"
#include "widgets/claybrushselect.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydocked.h"
#include "widgets/clayfontselect.h"
#include "widgets/claygradientselect.h"
#include "widgets/clayhelp.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayimageselect.h"
#include "widgets/clayimageview.h"
#include "widgets/clayitemselect.h"
#include "widgets/claymenufactory.h"
#include "widgets/claypaletteselect.h"
#include "widgets/claypatternselect.h"
#include "widgets/clayprogressdialog.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplay-foreach.h"
#include "display/claydisplayshell.h"
#include "display/claysinglewindowstrategy.h"
#include "display/claymultiwindowstrategy.h"

#include "actions/plug-in-actions.h"

#include "menus/menus.h"

#include "dialogs/color-profile-import-dialog.h"
#include "dialogs/metadata-rotation-import-dialog.h"

#include "gui.h"
#include "gui-message.h"
#include "gui-vtable.h"
#include "icon-themes.h"
#include "themes.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void           gui_ungrab                    (Clay                *clay);

static void           gui_set_busy                  (Clay                *clay);
static void           gui_unset_busy                (Clay                *clay);

static void           gui_help                      (Clay                *clay,
                                                     ClayProgress        *progress,
                                                     const gchar         *help_domain,
                                                     const gchar         *help_id);
static const gchar  * gui_get_program_class         (Clay                *clay);
static gchar        * gui_get_display_name          (Clay                *clay,
                                                     gint                 display_id,
                                                     GObject            **monitor,
                                                     gint                *monitor_number);
static guint32        gui_get_user_time             (Clay                *clay);
static GFile        * gui_get_theme_dir             (Clay                *clay);
static GFile        * gui_get_icon_theme_dir        (Clay                *clay);
static ClayObject   * gui_get_window_strategy       (Clay                *clay);
static ClayDisplay  * gui_get_empty_display         (Clay                *clay);
static GBytes       * gui_display_get_window_handle (ClayDisplay         *display);
static ClayDisplay  * gui_display_create            (Clay                *clay,
                                                     ClayImage           *image,
                                                     ClayUnit            *unit,
                                                     gdouble              scale,
                                                     GObject             *monitor);
static void           gui_display_delete            (ClayDisplay         *display);
static void           gui_displays_reconnect        (Clay                *clay,
                                                     ClayImage           *old_image,
                                                     ClayImage           *new_image);
static gboolean       gui_wait                      (Clay                *clay,
                                                     ClayWaitable        *waitable,
                                                     const gchar         *message);
static ClayProgress * gui_new_progress              (Clay                *clay,
                                                     ClayDisplay         *display);
static void           gui_free_progress             (Clay                *clay,
                                                     ClayProgress        *progress);
static gboolean       gui_pdb_dialog_new            (Clay                *clay,
                                                     ClayContext         *context,
                                                     ClayProgress        *progress,
                                                     GType                object_type,
                                                     GBytes              *parent_handle,
                                                     const gchar         *title,
                                                     const gchar         *callback_name,
                                                     ClayObject          *object,
                                                     va_list              args);
static gboolean       gui_pdb_dialog_set            (Clay                *clay,
                                                     GType                contents_type,
                                                     const gchar         *callback_name,
                                                     ClayObject          *object,
                                                     va_list              args);
static gboolean       gui_pdb_dialog_close          (Clay                *clay,
                                                     GType                contents_type,
                                                     const gchar         *callback_name);
static gboolean       gui_recent_list_add_file      (Clay                *clay,
                                                     GFile               *file,
                                                     const gchar         *mime_type);
static void           gui_recent_list_load          (Clay                *clay);

static GMountOperation
                    * gui_get_mount_operation       (Clay                *clay,
                                                     ClayProgress        *progress);

static ClayColorProfilePolicy
                      gui_query_profile_policy      (Clay                *clay,
                                                     ClayImage           *image,
                                                     ClayContext         *context,
                                                     ClayColorProfile   **dest_profile,
                                                     ClayColorRenderingIntent *intent,
                                                     gboolean            *bpc,
                                                     gboolean            *dont_ask);
static ClayMetadataRotationPolicy
                      gui_query_rotation_policy     (Clay                *clay,
                                                     ClayImage           *image,
                                                     ClayContext         *context,
                                                     gboolean            *dont_ask);

static void           gui_inhibit                   (Clay                *clay);
static void           gui_image_disconnect          (ClayImage           *image,
                                                     Clay                *clay);


/*  public functions  */

void
gui_vtable_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay->gui.ungrab                    = gui_ungrab;
  clay->gui.set_busy                  = gui_set_busy;
  clay->gui.unset_busy                = gui_unset_busy;
  clay->gui.show_message              = gui_message;
  clay->gui.help                      = gui_help;
  clay->gui.get_program_class         = gui_get_program_class;
  clay->gui.get_display_name          = gui_get_display_name;
  clay->gui.get_user_time             = gui_get_user_time;
  clay->gui.get_theme_dir             = gui_get_theme_dir;
  clay->gui.get_icon_theme_dir        = gui_get_icon_theme_dir;
  clay->gui.get_window_strategy       = gui_get_window_strategy;
  clay->gui.get_empty_display         = gui_get_empty_display;
  clay->gui.display_get_window_handle = gui_display_get_window_handle;
  clay->gui.display_create            = gui_display_create;
  clay->gui.display_delete            = gui_display_delete;
  clay->gui.displays_reconnect        = gui_displays_reconnect;
  clay->gui.wait                      = gui_wait;
  clay->gui.progress_new              = gui_new_progress;
  clay->gui.progress_free             = gui_free_progress;
  clay->gui.pdb_dialog_new            = gui_pdb_dialog_new;
  clay->gui.pdb_dialog_set            = gui_pdb_dialog_set;
  clay->gui.pdb_dialog_close          = gui_pdb_dialog_close;
  clay->gui.recent_list_add_file      = gui_recent_list_add_file;
  clay->gui.recent_list_load          = gui_recent_list_load;
  clay->gui.get_mount_operation       = gui_get_mount_operation;
  clay->gui.query_profile_policy      = gui_query_profile_policy;
  clay->gui.query_rotation_policy     = gui_query_rotation_policy;
}


/*  private functions  */

static void
gui_ungrab (Clay *clay)
{
  GdkDisplay *display = gdk_display_get_default ();

  if (display)
    gdk_seat_ungrab (gdk_display_get_default_seat (display));
}

static void
gui_set_busy (Clay *clay)
{
  clay_displays_set_busy (clay);
  clay_dialog_factory_set_busy (clay_dialog_factory_get_singleton ());

  gdk_display_flush (gdk_display_get_default ());
}

static void
gui_unset_busy (Clay *clay)
{
  clay_displays_unset_busy (clay);
  clay_dialog_factory_unset_busy (clay_dialog_factory_get_singleton ());

  gdk_display_flush (gdk_display_get_default ());
}

static void
gui_help (Clay         *clay,
          ClayProgress *progress,
          const gchar  *help_domain,
          const gchar  *help_id)
{
  clay_help_show (clay, progress, help_domain, help_id);
}

static const gchar *
gui_get_program_class (Clay *clay)
{
  return gdk_get_program_class ();
}

static gint
get_monitor_number (GdkMonitor *monitor)
{
  GdkDisplay *display    = gdk_monitor_get_display (monitor);
  gint        n_monitors = gdk_display_get_n_monitors (display);
  gint        i;

  for (i = 0; i < n_monitors; i++)
    if (gdk_display_get_monitor (display, i) == monitor)
      return i;

  return 0;
}

static gchar *
gui_get_display_name (Clay     *clay,
                      gint      display_id,
                      GObject **monitor,
                      gint     *monitor_number)
{
  ClayDisplay *display = NULL;
  GdkDisplay  *gdk_display;

  if (display_id > 0)
    display = clay_display_get_by_id (clay, display_id);

  if (display)
    {
      ClayDisplayShell *shell = clay_display_get_shell (display);

      gdk_display = gtk_widget_get_display (GTK_WIDGET (shell));

      *monitor = G_OBJECT (clay_widget_get_monitor (GTK_WIDGET (shell)));
    }
  else
    {
      *monitor = G_OBJECT (gui_get_initial_monitor (clay));

      if (! *monitor)
        *monitor = G_OBJECT (clay_get_monitor_at_pointer ());

      gdk_display = gdk_monitor_get_display (GDK_MONITOR (*monitor));
    }

  *monitor_number = get_monitor_number (GDK_MONITOR (*monitor));

  return g_strdup (gdk_display_get_name (gdk_display));
}

static guint32
gui_get_user_time (Clay *clay)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_display_get_user_time (gdk_display_get_default ());
#endif

  return gtk_get_current_event_time ();
}

static GFile *
gui_get_theme_dir (Clay *clay)
{
  return themes_get_theme_dir (clay, CLAY_GUI_CONFIG (clay->config)->theme);
}

static GFile *
gui_get_icon_theme_dir (Clay *clay)
{
  return icon_themes_get_theme_dir (clay, CLAY_GUI_CONFIG (clay->config)->icon_theme);
}

static ClayObject *
gui_get_window_strategy (Clay *clay)
{
  if (CLAY_GUI_CONFIG (clay->config)->single_window_mode)
    return clay_single_window_strategy_get_singleton ();
  else
    return clay_multi_window_strategy_get_singleton ();
}

static ClayDisplay *
gui_get_empty_display (Clay *clay)
{
  ClayDisplay *display = NULL;

  if (clay_container_get_n_children (clay->displays) == 1)
    {
      display = (ClayDisplay *) clay_container_get_first_child (clay->displays);

      if (clay_display_get_image (display))
        {
          /* The display was not empty */
          display = NULL;
        }
    }

  return display;
}

static GBytes *
gui_display_get_window_handle (ClayDisplay *display)
{
  ClayDisplay      *disp  = CLAY_DISPLAY (display);
  ClayDisplayShell *shell = clay_display_get_shell (disp);

  if (shell)
    return clay_progress_get_window_handle (CLAY_PROGRESS (shell));

  return NULL;
}

static ClayDisplay *
gui_display_create (Clay      *clay,
                    ClayImage *image,
                    ClayUnit  *unit,
                    gdouble    scale,
                    GObject   *monitor)
{
  ClayContext *context = clay_get_user_context (clay);
  ClayDisplay *display = CLAY_DISPLAY (gui_get_empty_display (clay));

  if (! monitor)
    monitor = G_OBJECT (clay_get_monitor_at_pointer ());

  if (display)
    {
      clay_display_fill (display, image, unit, scale);
    }
  else
    {
      GList *image_managers = clay_ui_managers_from_name ("<Image>");

      g_return_val_if_fail (image_managers != NULL, NULL);

      display = clay_display_new (clay, image, unit, scale,
                                  image_managers->data,
                                  clay_dialog_factory_get_singleton (),
                                  GDK_MONITOR (monitor));
   }

  if (clay_context_get_display (context) == display)
    {
      clay_context_set_image (context, image);
      clay_context_display_changed (context);
    }
  else
    {
      clay_context_set_display (context, display);
    }

  if (image)
    {
      g_signal_handlers_disconnect_by_func (image,
                                            G_CALLBACK (gui_inhibit),
                                            clay);
      g_signal_handlers_disconnect_by_func (image,
                                            G_CALLBACK (gui_image_disconnect),
                                            clay);
      g_signal_connect_swapped (image, "dirty",
                                G_CALLBACK (gui_inhibit),
                                clay);
      g_signal_connect_swapped (image, "clean",
                                G_CALLBACK (gui_inhibit),
                                clay);
      g_signal_connect_after (image, "disconnect",
                              G_CALLBACK (gui_image_disconnect),
                              clay);
    }

  return display;
}

static void
gui_display_delete (ClayDisplay *display)
{
  clay_display_close (display);
}

static void
gui_displays_reconnect (Clay      *clay,
                        ClayImage *old_image,
                        ClayImage *new_image)
{
  clay_displays_reconnect (clay, old_image, new_image);
}

static void
gui_wait_input_async (ClayAsync  *async,
                      const gint  input_pipe[2])
{
  guint8 buffer[1];

  while (read (input_pipe[0], buffer, sizeof (buffer)) == -1 &&
         errno == EINTR);

  clay_async_finish (async, NULL);
}

static gboolean
gui_wait (Clay         *clay,
          ClayWaitable *waitable,
          const gchar  *message)
{
  ClayProcedure  *procedure;
  ClayValueArray *args;
  ClayAsync      *input_async = NULL;
  GError         *error       = NULL;
  gint            input_pipe[2];
  gint            output_pipe[2];

  procedure = clay_pdb_lookup_procedure (clay->pdb, "plug-in-busy-dialog");

  if (! procedure)
    return FALSE;

  if (pipe (input_pipe))
    return FALSE;

  if (pipe (output_pipe))
    {
      close (input_pipe[0]);
      close (input_pipe[1]);

      return FALSE;
    }

  clay_spawn_set_cloexec (input_pipe[0]);
  clay_spawn_set_cloexec (output_pipe[1]);

  args = clay_procedure_get_arguments (procedure);
  clay_value_array_truncate (args, 5);

  g_value_set_enum    (clay_value_array_index (args, 0),
                       CLAY_RUN_INTERACTIVE);
  g_value_set_int     (clay_value_array_index (args, 1),
                       output_pipe[0]);
  g_value_set_int     (clay_value_array_index (args, 2),
                       input_pipe[1]);
  g_value_set_string  (clay_value_array_index (args, 3),
                       message);
  g_value_set_boolean (clay_value_array_index (args, 4),
                       CLAY_IS_CANCELABLE (waitable));

  clay_procedure_execute_async (procedure, clay,
                                clay_get_user_context (clay),
                                NULL, args, NULL, &error);

  clay_value_array_unref (args);

  close (input_pipe[1]);
  close (output_pipe[0]);

  if (error)
    {
      g_clear_error (&error);

      close (input_pipe[0]);
      close (output_pipe[1]);

      return FALSE;
    }

  if (CLAY_IS_CANCELABLE (waitable))
    /* listens for a cancellation request */
    input_async = clay_parallel_run_async_independent ((ClayRunAsyncFunc) gui_wait_input_async,
                                                       input_pipe);

  while (! clay_waitable_wait_for (waitable, 0.1 * G_TIME_SPAN_SECOND))
    {
      /* Making sure main GUI is not frozen while waiting. */
      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, FALSE);

      /* check for a cancellation request */
      if (input_async && clay_waitable_try_wait (CLAY_WAITABLE (input_async)))
        {
          clay_cancelable_cancel (CLAY_CANCELABLE (waitable));

          break;
        }
    }

  clay_waitable_wait (waitable);

  /* signal completion to the plug-in */
  close (output_pipe[1]);

  if (input_async)
    {
      clay_waitable_wait (CLAY_WAITABLE (input_async));

      g_object_unref (input_async);
    }

  close (input_pipe[0]);

  return TRUE;
}

static ClayProgress *
gui_new_progress (Clay        *clay,
                  ClayDisplay *display)
{
  g_return_val_if_fail (display == NULL || CLAY_IS_DISPLAY (display), NULL);

  if (display)
    return CLAY_PROGRESS (display);

  return CLAY_PROGRESS (clay_progress_dialog_new ());
}

static void
gui_free_progress (Clay          *clay,
                   ClayProgress  *progress)
{
  g_return_if_fail (CLAY_IS_PROGRESS (progress));

  if (CLAY_IS_PROGRESS_DIALOG (progress))
    gtk_widget_destroy (GTK_WIDGET (progress));
}

static gboolean
gui_pdb_dialog_present (GtkWindow *window)
{
  gtk_window_present (window);

  return FALSE;
}

static gboolean
gui_pdb_dialog_new (Clay          *clay,
                    ClayContext   *context,
                    ClayProgress  *progress,
                    GType          contents_type,
                    GBytes        *parent_handle,
                    const gchar   *title,
                    const gchar   *callback_name,
                    ClayObject    *object,
                    va_list        args)
{
  GType        dialog_type = G_TYPE_NONE;
  const gchar *dialog_role = NULL;
  const gchar *help_id     = NULL;

  if (contents_type == CLAY_TYPE_BRUSH)
    {
      dialog_type = CLAY_TYPE_BRUSH_SELECT;
      dialog_role = "clay-brush-selection";
      help_id     = CLAY_HELP_BRUSH_DIALOG;
    }
  else if (contents_type == CLAY_TYPE_FONT)
    {
      dialog_type = CLAY_TYPE_FONT_SELECT;
      dialog_role = "clay-font-selection";
      help_id     = CLAY_HELP_FONT_DIALOG;
    }
  else if (contents_type == CLAY_TYPE_GRADIENT)
    {
      dialog_type = CLAY_TYPE_GRADIENT_SELECT;
      dialog_role = "clay-gradient-selection";
      help_id     = CLAY_HELP_GRADIENT_DIALOG;
    }
  else if (contents_type == CLAY_TYPE_IMAGE)
    {
      dialog_type = CLAY_TYPE_IMAGE_SELECT;
      dialog_role = "clay-image-selection";
      help_id     = CLAY_HELP_IMAGE_DIALOG;
    }
  else if (contents_type == CLAY_TYPE_PALETTE)
    {
      dialog_type = CLAY_TYPE_PALETTE_SELECT;
      dialog_role = "clay-palette-selection";
      help_id     = CLAY_HELP_PALETTE_DIALOG;
    }
  else if (contents_type == CLAY_TYPE_PATTERN)
    {
      dialog_type = CLAY_TYPE_PATTERN_SELECT;
      dialog_role = "clay-pattern-selection";
      help_id     = CLAY_HELP_PATTERN_DIALOG;
    }
  else if (g_type_is_a (contents_type, CLAY_TYPE_ITEM))
    {
      dialog_type = CLAY_TYPE_ITEM_SELECT;
      dialog_role = "clay-item-selection";
    }
  else
    {
      g_return_val_if_reached (FALSE);
    }

  if (dialog_type != G_TYPE_NONE)
    {
      if (! object && ! g_type_is_a (contents_type, CLAY_TYPE_ITEM))
        object = clay_context_get_by_type (context, contents_type);

      if (object || g_type_is_a (contents_type, CLAY_TYPE_ITEM))
        {
          gint        n_properties = 0;
          gchar     **names        = NULL;
          GValue     *values       = NULL;
          GtkWidget  *dialog;
          GtkWidget  *view;
          gboolean    use_header_bar;

          g_object_get (gtk_settings_get_default (),
                        "gtk-dialogs-use-header", &use_header_bar,
                        NULL);

          names = clay_properties_append (dialog_type,
                                          &n_properties, names, &values,
                                          "title",          title,
                                          "role",           dialog_role,
                                          "help-func",      clay_standard_help_func,
                                          "help-id",        help_id,
                                          "pdb",            clay->pdb,
                                          "context",        context,
                                          "select-type",    contents_type,
                                          "initial-object", object,
                                          "callback-name",  callback_name,
                                          "menu-factory",   menus_get_global_menu_factory (clay),
                                          "use-header-bar", use_header_bar,
                                          NULL);

          names = clay_properties_append_valist (dialog_type,
                                                 &n_properties, names, &values,
                                                 args);

          dialog = (GtkWidget *)
            g_object_new_with_properties (dialog_type,
                                          n_properties,
                                          (const gchar **) names,
                                          (const GValue *) values);

          clay_properties_free (n_properties, names, values);

          view = CLAY_PDB_DIALOG (dialog)->view;
          if (view)
            clay_docked_set_show_button_bar (CLAY_DOCKED (view), FALSE);

          if (progress)
            clay_window_set_transient_for (GTK_WINDOW (dialog), progress);

          gtk_widget_set_visible (dialog, TRUE);

          /*  workaround for bug #360106  */
            {
              GSource  *source = g_timeout_source_new (100);
              GClosure *closure;

              closure = g_cclosure_new_object (G_CALLBACK (gui_pdb_dialog_present),
                                               G_OBJECT (dialog));

              g_source_set_closure (source, closure);
              g_source_attach (source, NULL);
              g_source_unref (source);
            }

          if (parent_handle != NULL)
            clay_window_set_transient_for_handle (GTK_WINDOW (dialog), parent_handle);

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gui_pdb_dialog_set (Clay          *clay,
                    GType          contents_type,
                    const gchar   *callback_name,
                    ClayObject    *object,
                    va_list        args)
{
  ClayPdbDialogClass *klass     = NULL;
  ClayContainer      *container = NULL;
  ClayPdbDialog      *dialog;

  if (contents_type == CLAY_TYPE_BRUSH)
    {
      klass = g_type_class_ref (CLAY_TYPE_BRUSH_SELECT);
      container = clay_data_factory_get_container (clay->brush_factory);
    }
  else if (contents_type == CLAY_TYPE_FONT)
    {
      klass = g_type_class_ref (CLAY_TYPE_FONT_SELECT);
      container = clay_data_factory_get_container (clay->font_factory);
    }
  else if (contents_type == CLAY_TYPE_GRADIENT)
    {
      klass = g_type_class_ref (CLAY_TYPE_GRADIENT_SELECT);
      container = clay_data_factory_get_container (clay->gradient_factory);
    }
  else if (contents_type == CLAY_TYPE_IMAGE)
    {
      klass = g_type_class_ref (CLAY_TYPE_IMAGE_SELECT);
    }
  else if (contents_type == CLAY_TYPE_PALETTE)
    {
      klass = g_type_class_ref (CLAY_TYPE_PALETTE_SELECT);
      container = clay_data_factory_get_container (clay->palette_factory);
    }
  else if (contents_type == CLAY_TYPE_PATTERN)
    {
      klass = g_type_class_ref (CLAY_TYPE_PATTERN_SELECT);
      container = clay_data_factory_get_container (clay->pattern_factory);
    }
  else if (g_type_is_a (contents_type, CLAY_TYPE_ITEM))
    {
      klass = g_type_class_ref (CLAY_TYPE_ITEM_SELECT);
    }

  g_return_val_if_fail (klass != NULL, FALSE);

  dialog = clay_pdb_dialog_get_by_callback (klass, callback_name);

  if (dialog != NULL                       &&
      dialog->select_type == contents_type &&
      (container == NULL || clay_container_get_child_index (container, object) != -1))
    {
      const gchar *prop_name = va_arg (args, const gchar *);

      if (g_type_is_a (contents_type, CLAY_TYPE_RESOURCE))
        {
          g_return_val_if_fail (container != NULL, FALSE);
          clay_context_set_by_type (dialog->context, dialog->select_type, object);
        }
      else
        {
          g_return_val_if_fail (klass->set_object != NULL, FALSE);
          klass->set_object (dialog, object);
        }

      if (prop_name)
        g_object_set_valist (G_OBJECT (dialog), prop_name, args);

      gtk_window_present (GTK_WINDOW (dialog));
      g_type_class_unref (klass);

      return TRUE;
    }

  g_type_class_unref (klass);

  return FALSE;
}

static gboolean
gui_pdb_dialog_close (Clay        *clay,
                      GType        contents_type,
                      const gchar *callback_name)
{
  ClayPdbDialogClass *klass = NULL;

  if (contents_type == CLAY_TYPE_BRUSH)
    klass = g_type_class_peek (CLAY_TYPE_BRUSH_SELECT);
  else if (contents_type == CLAY_TYPE_FONT)
    klass = g_type_class_peek (CLAY_TYPE_FONT_SELECT);
  else if (contents_type == CLAY_TYPE_GRADIENT)
    klass = g_type_class_peek (CLAY_TYPE_GRADIENT_SELECT);
  else if (contents_type == CLAY_TYPE_IMAGE)
    klass = g_type_class_peek (CLAY_TYPE_IMAGE_SELECT);
  else if (contents_type == CLAY_TYPE_PALETTE)
    klass = g_type_class_peek (CLAY_TYPE_PALETTE_SELECT);
  else if (contents_type == CLAY_TYPE_PATTERN)
    klass = g_type_class_peek (CLAY_TYPE_PATTERN_SELECT);
  else if (g_type_is_a (contents_type, CLAY_TYPE_ITEM))
    klass = g_type_class_peek (CLAY_TYPE_ITEM_SELECT);

  if (klass)
    {
      ClayPdbDialog *dialog;

      dialog = clay_pdb_dialog_get_by_callback (klass, callback_name);

      if (dialog && dialog->select_type == contents_type)
        {
          gtk_widget_destroy (GTK_WIDGET (dialog));
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gui_recent_list_add_file (Clay        *clay,
                          GFile       *file,
                          const gchar *mime_type)
{
  GtkRecentData  recent;
  const gchar   *groups[2] = { "Graphics", NULL };
  gchar         *uri;
  gboolean       success;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  /* use last part of the URI */
  recent.display_name = NULL;

  /* no special description */
  recent.description  = NULL;
  recent.mime_type    = (mime_type ?
                         (gchar *) mime_type : "application/octet-stream");
  recent.app_name     = "Clay Image Beautification Program";
  recent.app_exec     = CLAY_COMMAND " %u";
  recent.groups       = (gchar **) groups;
  recent.is_private   = FALSE;

  uri = g_file_get_uri (file);

  success = gtk_recent_manager_add_full (gtk_recent_manager_get_default (),
                                         uri, &recent);

  g_free (uri);

  return success;
}

static gint
gui_recent_list_compare (gconstpointer a,
                         gconstpointer b)
{
  return (gtk_recent_info_get_modified ((GtkRecentInfo *) a) -
          gtk_recent_info_get_modified ((GtkRecentInfo *) b));
}

static void
gui_recent_list_load (Clay *clay)
{
  GList *items;
  GList *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_container_freeze (clay->documents);
  clay_container_clear (clay->documents);

  items = gtk_recent_manager_get_items (gtk_recent_manager_get_default ());

  items = g_list_sort (items, gui_recent_list_compare);

  for (list = items; list; list = list->next)
    {
      GtkRecentInfo *info = list->data;

      if (gtk_recent_info_has_application (info,
                                           "Clay Image Beautification Program"))
        {
          const gchar *mime_type = gtk_recent_info_get_mime_type (info);

          if (mime_type &&
              clay_plug_in_manager_file_procedure_find_by_mime_type (clay->plug_in_manager,
                                                                     CLAY_FILE_PROCEDURE_GROUP_OPEN,
                                                                     mime_type))
            {
              ClayImagefile *imagefile;
              GFile         *file;

              file = g_file_new_for_uri (gtk_recent_info_get_uri (info));
              imagefile = clay_imagefile_new (clay, file);
              g_object_unref (file);

              clay_imagefile_set_mime_type (imagefile, mime_type);

              clay_container_add (clay->documents, CLAY_OBJECT (imagefile));
              g_object_unref (imagefile);
            }
        }

      gtk_recent_info_unref (info);
    }

  g_list_free (items);

  clay_container_thaw (clay->documents);
}

static GMountOperation *
gui_get_mount_operation (Clay         *clay,
                         ClayProgress *progress)
{
  GtkWidget *toplevel = NULL;

  if (GTK_IS_WIDGET (progress))
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

  return gtk_mount_operation_new (GTK_WINDOW (toplevel));
}

static ClayColorProfilePolicy
gui_query_profile_policy (Clay                      *clay,
                          ClayImage                 *image,
                          ClayContext               *context,
                          ClayColorProfile         **dest_profile,
                          ClayColorRenderingIntent  *intent,
                          gboolean                  *bpc,
                          gboolean                  *dont_ask)
{
  return color_profile_import_dialog_run (image, context, NULL,
                                          dest_profile,
                                          intent, bpc,
                                          dont_ask);
}

static ClayMetadataRotationPolicy
gui_query_rotation_policy (Clay        *clay,
                           ClayImage   *image,
                           ClayContext *context,
                           gboolean    *dont_ask)
{
  return metadata_rotation_import_dialog_run (image, context,
                                              NULL, dont_ask);
}

static void
gui_inhibit (Clay *clay)
{
  static gboolean  in_test      = TRUE;
  static gint      cookie       = 0;
  static gint      n_images     = 0;
  static gint64    last_failure = 0;
  GtkApplication  *app;
  ClayContainer   *images  = NULL;

  if (in_test)
    {
      /* Do not call inhibit code while unit-testing the UI code. */
      in_test = (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR") != NULL);
      if (in_test)
        return;
    }

  app = GTK_APPLICATION (g_application_get_default ());
  if (app == NULL)
    /* This may happen when quitting. The GtkApplication is finalized
     * faster than the images without display.
     */
    return;

  if (clay_displays_dirty (clay))
    {
      gint n_dirty_images;

      images         = clay_displays_get_dirty_images (clay);
      n_dirty_images = clay_container_get_n_children (images);

      if (cookie && n_images == n_dirty_images)
        {
          g_object_unref (images);
          return;
        }

      n_images = n_dirty_images;
    }

  if (cookie != 0)
    {
      gtk_application_uninhibit (app, cookie);
      cookie = 0;
    }

  if (last_failure != 0 && g_get_monotonic_time () - last_failure < G_TIME_SPAN_MINUTE * 10)
    {
      /* Don't repeatedly try to inhibit when we are constantly failing.
       * Especially as in some case, it may lock the thread.
       */
      g_clear_object (&images);
      return;
    }
  last_failure = 0;

  if (clay_displays_dirty (clay))
    {
      ClayImage *image;
      GList     *list;
      GtkWindow *window = NULL;
      gchar     *reason;

      image = (ClayImage *) clay_container_get_first_child (images);
      g_object_unref (images);

      for (list = clay_get_display_iter (clay); list; list = list->next)
        {
          ClayDisplay *display = list->data;

          if (clay_display_get_image (display) == image)
            {
              ClayDisplayShell *shell;
              GtkWidget        *toplevel;

              shell    = clay_display_get_shell (display);
              toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

              if (GTK_IS_WINDOW (toplevel))
                {
                  window = GTK_WINDOW (toplevel);
                  break;
                }
            }
        }

      /* TRANSLATORS: unless your language msgstr[0] applies to 1 only (as in
       * English), replace "one" with %d.
       */
      reason = g_strdup_printf (ngettext ("There is one image with unsaved changes!",
                                          "There are %d images with unsaved changes!",
                                          n_images),
                                n_images);
      cookie = gtk_application_inhibit (app, window,
                                        GTK_APPLICATION_INHIBIT_LOGOUT,
                                        reason);
      g_free (reason);

      if (cookie == 0)
        last_failure = g_get_monotonic_time ();
    }

  if (cookie == 0)
    n_images = 0;
}

static void
gui_image_disconnect (ClayImage *image,
                      Clay      *clay)
{
  gui_inhibit (clay);

  g_signal_handlers_disconnect_by_func (image,
                                        G_CALLBACK (gui_inhibit),
                                        clay);
  g_signal_handlers_disconnect_by_func (image,
                                        G_CALLBACK (gui_image_disconnect),
                                        clay);
}
