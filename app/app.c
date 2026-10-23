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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <winnls.h>
#endif

#undef CLAY_DISABLE_DEPRECATED /* for compat enums */
#include "libclaybase/claybase.h"
#define CLAY_DISABLE_DEPRECATED
#include "libclaybase/claybase-private.h"
#include "libclayconfig/clayconfig.h"

#include "core/core-types.h"

#include "config/clayrc.h"

#include "gegl/clay-gegl.h"

#include "core/clay.h"
#include "core/clay-batch.h"
#include "core/clay-user-install.h"
#include "core/clayidtable.h"
#include "core/clayimage.h"
#include "core/clayimage-savable.h"

#include "file/file-open.h"

#ifndef CLAY_CONSOLE_COMPILATION
#include <gtk/gtk.h>

#include "dialogs/user-install-dialog.h"

#include "gui/clayapp.h"
#include "gui/gui.h"
#endif

#include "app.h"
#include "errors.h"
#include "clayconsoleapp.h"
#include "claycoreapp.h"
#include "language.h"
#include "sanity.h"
#include "clay-debug.h"

#include "clay-intl.h"
#include "clay-update.h"


/*  local prototypes  */

static void       app_init_update_noop       (const gchar        *text1,
                                              const gchar        *text2,
                                              gdouble             percentage);
static void       app_activate_callback      (ClayCoreApp        *app,
                                              gpointer            user_data);
static gboolean   app_exit_after_callback    (Clay               *clay,
                                              gboolean            kill_it,
                                              GApplication       *app);
#ifdef PLATFORM_OSX
static void       app_language_notify        (GObject            *config,
                                              GParamSpec         *pspec,
                                              gpointer            user_data);
#endif

#ifdef G_OS_WIN32
static BOOL       app_quit_on_ctrl_c         (DWORD               ctrl_type);
#else
static void       app_quit_on_ctrl_c         (gint                sig_num);
#endif

#if 0
/*  left here as documentation how to do compat enums  */
GType clay_convert_dither_type_compat_get_type (void); /* compat cruft */
#endif


/*  public functions  */

void
app_libs_init (GOptionContext *context,
               gboolean        no_interface)
{
#if 0
  GQuark quark;
#endif

  /* disable OpenCL before GEGL is even initialized; this way we only
   * enable if wanted in clayrc, instead of always enabling, and then
   * disabling again if wanted in clayrc
   */
  g_object_set (gegl_config (),
                "use-opencl", FALSE,
                "application-license", "GPL3",
                NULL);

  g_option_context_add_group (context, gegl_get_option_group ());

#ifndef CLAY_CONSOLE_COMPILATION
  if (! no_interface)
    {
      gui_libs_init (context);
    }
#endif

#if 0
  /*  left here as documentation how to do compat enums  */

  /*  keep compat enum code in sync with pdb/enumcode.pl  */
  quark = g_quark_from_static_string ("clay-compat-enum");

  g_type_set_qdata (CLAY_TYPE_CONVERT_DITHER_TYPE, quark,
                    (gpointer) clay_convert_dither_type_compat_get_type ());
#endif
}

void
app_abort (gboolean     no_interface,
           const gchar *abort_message)
{
#ifndef CLAY_CONSOLE_COMPILATION
  if (no_interface)
#endif
    {
      g_print ("%s\n\n", abort_message);
    }
#ifndef CLAY_CONSOLE_COMPILATION
  else
    {
      gui_abort (abort_message);
    }
#endif

  app_exit (EXIT_FAILURE);
}

void
app_exit (gint status)
{
  exit (status);
}

gint
app_run (const gchar         *full_prog_name,
         const gchar        **filenames,
         GFile               *alternate_system_clayrc,
         GFile               *alternate_clayrc,
         const gchar         *session_name,
         const gchar         *batch_interpreter,
         const gchar        **batch_commands,
         gboolean             quit,
         gboolean             as_new,
         gboolean             no_interface,
         gboolean             no_data,
         gboolean             no_fonts,
         gboolean             no_splash,
         gboolean             be_verbose,
         gboolean             use_shm,
         gboolean             use_cpu_accel,
         gboolean             console_messages,
         gboolean             use_debug_handler,
         gboolean             show_playground,
         gboolean             show_debug_menu,
         ClayStackTraceMode   stack_trace_mode,
         ClayPDBCompatMode    pdb_compat_mode,
         const gchar         *backtrace_file)
{
  Clay               *clay           = NULL;
  GApplication       *app            = NULL;
  GFile              *default_folder = NULL;
  GFile              *claydir        = NULL;
  const gchar        *abort_message  = NULL;
  gint                retval         = EXIT_SUCCESS;

  if (filenames && filenames[0] && ! filenames[1] &&
      g_file_test (filenames[0], G_FILE_TEST_IS_DIR))
    {
      if (g_path_is_absolute (filenames[0]))
        {
          default_folder = g_file_new_for_path (filenames[0]);
        }
      else
        {
          gchar *absolute = g_build_path (G_DIR_SEPARATOR_S,
                                          g_get_current_dir (),
                                          filenames[0],
                                          NULL);
          default_folder = g_file_new_for_path (absolute);
          g_free (absolute);
        }

      filenames = NULL;
    }

  /*  Create an instance of the "Clay" object which is the root of the
   *  core object system
   */
  clay = clay_new (full_prog_name,
                   session_name,
                   default_folder,
                   be_verbose,
                   no_data,
                   no_fonts,
                   no_interface,
                   use_shm,
                   use_cpu_accel,
                   console_messages,
                   show_playground,
                   show_debug_menu,
                   stack_trace_mode,
                   pdb_compat_mode);

  g_clear_object (&default_folder);

#ifndef CLAY_CONSOLE_COMPILATION
  if (no_interface)
    {
      app = clay_console_app_new (clay, quit, as_new, filenames, batch_interpreter, batch_commands);
    }
  else
    {
      app = clay_app_new (clay, no_splash, quit, as_new, filenames, batch_interpreter, batch_commands);
    }
#else
  app = clay_console_app_new (clay, quit, as_new, filenames, batch_interpreter, batch_commands);
#endif

  clay->app = app;

  clay_cpu_accel_set_use (use_cpu_accel);

  /*  Check if the user's clay_directory exists */
  claydir = clay_directory_file (NULL);

  if (g_file_query_file_type (claydir, G_FILE_QUERY_INFO_NONE, NULL) !=
      G_FILE_TYPE_DIRECTORY)
    {
      ClayUserInstall *install = clay_user_install_new (G_OBJECT (clay),
                                                        be_verbose);

#ifdef CLAY_CONSOLE_COMPILATION
      clay_user_install_run (install, 1);
#else
      if (! (no_interface ?
             clay_user_install_run (install, 1) :
             user_install_dialog_run (install)))
        exit (EXIT_FAILURE);
#endif

      clay_user_install_free (install);
    }

  g_object_unref (claydir);

  clay_load_config (clay, alternate_system_clayrc, alternate_clayrc);

  /* Initialize the error handling after creating/migrating the config
   * directory because it will create some folders for backup and crash
   * logs in advance. Therefore running this before
   * clay_user_install_new() would break migration as well as initial
   * folder creations.
   *
   * It also needs to be run after clay_load_config() because error
   * handling is based on Preferences. It means that early loading code
   * is not handled by our debug code, but that's not a big deal.
   */
  errors_init (clay, full_prog_name, use_debug_handler,
               stack_trace_mode, backtrace_file);

  /*  run the late-stage sanity check.  it's important that this check is run
   *  after the call to language_init() (see comment in sanity_check_late().)
   */
  abort_message = sanity_check_late ();
  if (abort_message)
    app_abort (no_interface, abort_message);

  /*  initialize lowlevel stuff  */
  clay_gegl_init (clay);

  g_signal_connect_after (clay, "exit",
                          G_CALLBACK (app_exit_after_callback),
                          app);

  g_signal_connect (app, "activate",
                    G_CALLBACK (app_activate_callback),
                    NULL);
  retval = g_application_run (app, 0, NULL);

  if (! retval)
    retval = clay_core_app_get_exit_status (CLAY_CORE_APP (app));

  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, TRUE);

  g_clear_object (&app);

  clay_gegl_exit (clay);

  errors_exit ();

  g_object_unref (clay);

  clay_debug_instances ();

  gegl_exit ();

  return retval;
}


/*  private functions  */

static void
app_init_update_noop (const gchar *text1,
                      const gchar *text2,
                      gdouble      percentage)
{
  /*  deliberately do nothing  */
}

#ifdef PLATFORM_OSX
static void
app_language_notify (GObject    *config,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
  gchar *language = NULL;

  g_object_get (config, "language", &language, NULL);
  language_set_macos_menu_bar_lang (language);
  g_free (language);
}
#endif

static void
app_activate_callback (ClayCoreApp *app,
                       gpointer     user_data)
{
  Clay               *clay               = NULL;
  ClayInitStatusFunc  update_status_func = NULL;
  const gchar       **filenames;
  const gchar        *current_language;
  const gchar        *system_lang_l10n   = NULL;
  gchar              *prev_language      = NULL;
  GError             *font_error         = NULL;
  gint                batch_retval;

  g_return_if_fail (CLAY_IS_CORE_APP (app));

  clay = clay_core_app_get_clay (app);

  clay_core_app_set_exit_status (app, EXIT_SUCCESS);

  /* Language was already initialized. I call this again only to get the
   * actual language information and the "System Language" string
   * localized in the actual system language.
   */
  current_language = language_init (NULL, &system_lang_l10n);
#ifndef CLAY_CONSOLE_COMPILATION
  if (! clay->no_interface)
    update_status_func = gui_init (clay, clay_app_get_no_splash (CLAY_APP (app)),
                                   CLAY_APP (app), NULL, system_lang_l10n);
#endif

  if (! update_status_func)
    update_status_func = app_init_update_noop;

  /*  Create all members of the global Clay instance which need an already
   *  parsed clayrc, e.g. the data factories
   */
  clay_initialize (clay, update_status_func);

#ifdef PLATFORM_OSX
  /* macOS freezes [[NSBundle mainBundle] preferredLocalizations] before main(),
     so a language change only reaches those menus on the next launch. That
     is why we need to call it here, not only on language.c. */
  g_signal_connect (clay->edit_config, "notify::language",
                    G_CALLBACK (app_language_notify), NULL);
#endif

  g_object_get (clay->edit_config,
                "prev-language", &prev_language,
                NULL);

  clay->query_all = (prev_language == NULL ||
                     g_strcmp0 (prev_language, current_language) != 0);
  g_free (prev_language);

  /*  Load all data files */
  clay_restore (clay, update_status_func, &font_error);

  /*  enable autosave late so we don't autosave when the
   *  monitor resolution is set in gui_init()
   */
  clay_rc_set_autosave (CLAY_RC (clay->edit_config), TRUE);

#ifndef CLAY_CONSOLE_COMPILATION
  if (! clay->no_interface)
    {
      /* Before opening images from command line, check for salvaged images
       * and query interactively to know if we should recover or discard
       * them.
       */
      GHashTable *recovering;

      recovering = errors_recovered (clay);
      if (g_hash_table_size (recovering))
        {
          if (gui_recover ((g_hash_table_size (recovering))))
            {
              GList *IDs = g_hash_table_get_keys (recovering);

              for (GList *iter = IDs; iter; iter = iter->next)
                {
                  ClayImage *image;
                  gint       id     = GPOINTER_TO_INT (iter->data);
                  GFile     *subdir = g_hash_table_lookup (recovering, GINT_TO_POINTER (id));

                  image = clay_image_load_from_cache (clay, subdir, id);
                  if (image)
                    {
                      /* TODO: we should be able to distinguish
                       * recovered images which were clean when Clay
                       * crashed.
                       */
                      clay_image_dirty (image, CLAY_DIRTY_IMAGE);
                      clay_image_flush (image);
                      clay_create_display (clay, image, clay_unit_pixel (), 1.0, NULL);
                      g_object_unref (image);
                    }
                  else
                    {
                      clay_id_table_unreserve (clay->image_table, id);
                      /* TODO: should we delete the cache folder when
                       * recovering failed?
                       */
                    }
                }

              g_list_free (IDs);
            }
          /* TODO: when ! gui_recover(), delete cache. */
        }

      g_hash_table_unref (recovering);
    }
#endif

  /* Get possible files first, so that we can check if the
   * welcome dialog should be displayed
   */
  filenames = clay_core_app_get_filenames (app);

  /*  check for updates *after* enabling config autosave, so that the timestamp
   *  is saved
   */
  clay_update_auto_check (clay->edit_config, clay, (filenames != NULL));

  /* Setting properties to be used for the next run.  */
  g_object_set (clay->edit_config,
                /* Set this after clay_update_auto_check(). */
                "config-version", CLAY_VERSION,
                /* Set this after clay_restore(). */
                "prev-language",  current_language,
                NULL);

  /*  Load the images given on the command-line. */
  if (filenames != NULL)
    {
      gint i;

      for (i = 0; filenames[i] != NULL; i++)
        {
          GFile *file = g_file_new_for_commandline_arg (filenames[i]);

          file_open_from_command_line (clay, file,
                                       clay_core_app_get_as_new (app),
                                       NULL);

          g_object_unref (file);
        }
    }

  /* The software is now fully loaded and ready to be used and get
   * external input.
   */
  clay->initialized = TRUE;

  if (font_error)
    {
      clay_message_literal (clay, NULL,
                            CLAY_MESSAGE_INFO,
                            font_error->message);
      g_error_free (font_error);
    }

  batch_retval = clay_batch_run (clay,
                                 clay_core_app_get_batch_interpreter (app),
                                 clay_core_app_get_batch_commands (app));

  if (clay_core_app_get_quit (app))
    {
      /*  Only if we are in batch mode, we want to exit with the
       *  return value of the batch command.
       */
      clay_core_app_set_exit_status (app, batch_retval);

      /* Emit the "exit" signal, but also properly free all images still
      * opened.
      */
      clay_exit (clay, TRUE);
    }
  else
#ifndef CLAY_CONSOLE_COMPILATION
    if (clay->no_interface)
#endif
    {
      /* In console version or GUI version with no interface, we keep
       * running when --quit was not set. For instance, there could be
       * an always-ON plug-in (CLAY_PDB_PROC_TYPE_PERSISTENT) which is
       * set up to receive commands for Clay.
       */
#ifdef G_OS_WIN32
      SetConsoleCtrlHandler ((PHANDLER_ROUTINE) app_quit_on_ctrl_c, TRUE);
#else
      clay_signal_private (SIGINT, app_quit_on_ctrl_c, 0);
#endif
      g_printf ("\n== %s ==\n%s\n\n%s\n",
                /* TRANSLATORS: title for info message in terminal window */
                _("INFO"),
                /* TRANSLATORS: info message displayed in terminal window. */
                _("Clay is now running as a background process. "
                  "You can quit anytime with Ctrl-C (SIGINT)."),
                /* TRANSLATORS: info message displayed in terminal window. */
                _("If you wanted to quit immediately instead, call Clay with --quit."));
      g_application_hold (G_APPLICATION (app));
    }
}

static gboolean
app_exit_after_callback (Clay         *clay,
                         gboolean      kill_it,
                         GApplication *app)
{
  if (clay->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_application_quit (G_APPLICATION (app));

  return FALSE;
}

#ifdef G_OS_WIN32
static BOOL
app_quit_on_ctrl_c (DWORD ctrl_type)
{
  gboolean handled = FALSE;

  if (ctrl_type == CTRL_C_EVENT)
    {
      GApplication *app = g_application_get_default ();
      Clay         *clay;

      g_application_release (app);
      clay = clay_core_app_get_clay (CLAY_CORE_APP (app));
      clay_exit (clay, TRUE);

      handled = TRUE;
    }

  return handled;
}
#else
static void
app_quit_on_ctrl_c (gint sig_num)
{
  GApplication *app = g_application_get_default ();
  Clay         *clay;

  g_application_release (app);
  clay = clay_core_app_get_clay (CLAY_CORE_APP (app));
  clay_exit (clay, TRUE);
}
#endif
