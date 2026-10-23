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

#define _GNU_SOURCE  /* need the POSIX signal API */

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#ifdef G_OS_WIN32
#include "libclaybase/claywin32-io.h"
#endif

#include "core/core-types.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/clayidtable.h"
#include "core/clayitem.h"
#include "core/clayparamspecs.h"

#include "config/claycoreconfig.h"

#include "pdb/claypdb.h"

#include "errors.h"
#include "clay-log.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

/*  private variables  */

static Clay                *the_errors_clay    = NULL;
static gboolean             use_debug_handler  = FALSE;
static ClayStackTraceMode   stack_trace_mode   = CLAY_STACK_TRACE_QUERY;
static gchar               *full_prog_name     = NULL;
static gchar               *backtrace_file     = NULL;
static ClayLogHandler       log_domain_handler = 0;
static guint                global_handler_id  = 0;


/*  local function prototypes  */

static void    clay_message_log_func             (const gchar        *log_domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data);
static void    clay_error_log_func               (const gchar        *domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data) G_GNUC_NORETURN;

static G_GNUC_NORETURN void  clay_eek            (const gchar        *reason,
                                                  const gchar        *message,
                                                  gboolean            use_handler);


/*  public functions  */

void
errors_init (Clay               *clay,
             const gchar        *_full_prog_name,
             gboolean            _use_debug_handler,
             ClayStackTraceMode  _stack_trace_mode,
             const gchar        *_backtrace_file)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (_full_prog_name != NULL);
  g_return_if_fail (full_prog_name == NULL);

#ifdef CLAY_UNSTABLE
  g_printerr ("This is a development version of Clay.  "
              "Debug messages may appear here.\n\n");
#endif /* CLAY_UNSTABLE */

  the_errors_clay   = clay;
  use_debug_handler = _use_debug_handler ? TRUE : FALSE;
  stack_trace_mode  = _stack_trace_mode;
  full_prog_name    = g_strdup (_full_prog_name);

  /* Create parent directories for the crash files. */
  backtrace_file    = g_path_get_dirname (_backtrace_file);

  g_mkdir_with_parents (backtrace_file, S_IRUSR | S_IWUSR | S_IXUSR);
  g_free (backtrace_file);
  backtrace_file = g_strdup (_backtrace_file);

  log_domain_handler = clay_log_set_handler (FALSE,
                                             G_LOG_LEVEL_WARNING |
                                             G_LOG_LEVEL_MESSAGE |
                                             G_LOG_LEVEL_CRITICAL,
                                             clay_message_log_func, clay);

  global_handler_id = g_log_set_handler (NULL,
                                         G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                                         clay_error_log_func, clay);
}

void
errors_exit (void)
{
  if (log_domain_handler)
    {
      clay_log_remove_handler (log_domain_handler);

      log_domain_handler = 0;
    }

  if (global_handler_id)
    {
      g_log_remove_handler (NULL, global_handler_id);

      global_handler_id = 0;
    }

  the_errors_clay = NULL;

  if (backtrace_file)
    g_free (backtrace_file);
  if (full_prog_name)
    g_free (full_prog_name);
}

GHashTable *
errors_recovered (Clay *clay)
{
  GHashTable      *recovering;
  gchar           *backup_path = g_build_filename (clay_cache_directory (), "images", NULL);
  GFile           *backups_dir = g_file_new_for_path (backup_path);
  GFileEnumerator *enumerator;

  recovering = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                      NULL, g_object_unref);
  enumerator = g_file_enumerate_children (backups_dir,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED ","
                                          G_FILE_ATTRIBUTE_TIME_CHANGED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);
  if (enumerator)
    {
      GFileInfo  *info;
      GRegex     *basename_regex = g_regex_new ("^image-([1-9][0-9]*)$", G_REGEX_DEFAULT,
                                               G_REGEX_MATCH_DEFAULT, NULL);

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFile      *subdir;
          GFile      *xml;
          gchar      *basename;
          GMatchInfo *match_info = NULL;

          subdir   = g_file_enumerator_get_child (enumerator, info);
          basename = g_file_get_basename (subdir);
          xml      = g_file_get_child (subdir, "wlbr-project.xml");

          if (! g_file_info_get_attribute_boolean (info,
                                                   G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN) &&
              g_file_query_file_type (subdir,
                                      G_FILE_QUERY_INFO_NONE,
                                      NULL) == G_FILE_TYPE_DIRECTORY                    &&
              g_file_query_exists (xml, NULL)                                           &&
              g_regex_match (basename_regex, basename, G_REGEX_MATCH_DEFAULT, &match_info))
            {
              gchar  *id_str;
              gint64  id;

              id_str = g_match_info_fetch (match_info, 1);
              id     = g_ascii_strtoll (id_str, NULL, 10);
              if (errno != ERANGE && (gint64) (gint) id == id)
                {
                  /* The reason why we need to reserve the image IDs is
                   * that while recovering an image, it may trigger
                   * further image creation (with link layers), which
                   * could therefore take over the ID of an image to
                   * recover next. So we make sure all our needed IDs
                   * stay available.
                   */
                  clay_id_table_reserve (clay->image_table, (gint) id);
                  g_hash_table_insert (recovering, GINT_TO_POINTER ((gint) id), g_object_ref (subdir));
                }

              g_free (id_str);
            }
          /* TODO: should we delete invalid cache folders or other broken data? */

          g_free (basename);
          g_clear_object (&xml);
          g_clear_object (&subdir);
          g_clear_object (&info);
          g_clear_pointer (&match_info, g_match_info_free);
        }

      g_regex_unref (basename_regex);
      g_object_unref (enumerator);
    }
  g_free (backup_path);
  g_object_unref (backups_dir);

  return recovering;
}

void
clay_fatal_error (const gchar *message)
{
  clay_eek ("fatal error", message, TRUE);
}

void
clay_terminate (const gchar *message)
{
  clay_eek ("terminated", message, use_debug_handler);
}


/*  private functions  */

static void
clay_message_log_func (const gchar    *log_domain,
                       GLogLevelFlags  flags,
                       const gchar    *message,
                       gpointer        data)
{
  Clay                *clay        = data;
  ClayCoreConfig      *config      = clay->config;
  const gchar         *msg_domain  = NULL;
  ClayMessageSeverity  severity    = CLAY_MESSAGE_WARNING;
  gboolean             gui_message = TRUE;
  ClayDebugPolicy      debug_policy;

  /* All Clay messages are processed under the same domain, but
   * we need to keep the log domain information for third party
   * messages.
   */
  if (! log_domain ||
      (! g_str_has_prefix (log_domain, "Clay") &&
       ! g_str_has_prefix (log_domain, "LibClay")))
    msg_domain = log_domain;

  /* If debug policy requires it, WARNING and CRITICAL errors must be
   * routed for appropriate debugging.
   */
  g_object_get (G_OBJECT (config),
                "debug-policy", &debug_policy,
                NULL);

  switch (flags & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_MESSAGE:
      severity = CLAY_MESSAGE_INFO;
      break;
    case G_LOG_LEVEL_WARNING:
      severity = CLAY_MESSAGE_BUG_WARNING;
      if (debug_policy > CLAY_DEBUG_POLICY_WARNING)
        gui_message = FALSE;
      break;
    case G_LOG_LEVEL_CRITICAL:
      severity = CLAY_MESSAGE_BUG_CRITICAL;
      if (debug_policy > CLAY_DEBUG_POLICY_CRITICAL)
        gui_message = FALSE;
      break;
    }

  if (clay && gui_message)
    {
      clay_show_message (clay, NULL, severity, msg_domain, message);
    }
  else
    {
      const gchar *reason = "Message";

      clay_enum_get_value (CLAY_TYPE_MESSAGE_SEVERITY, severity,
                           NULL, NULL, &reason, NULL);

      g_printerr ("%s: %s-%s: %s\n",
                  clay_filename_to_utf8 (full_prog_name),
                  log_domain, reason, message);
    }
}

static void
clay_error_log_func (const gchar    *domain,
                     GLogLevelFlags  flags,
                     const gchar    *message,
                     gpointer        data)
{
  clay_fatal_error (message);
}

static void
clay_eek (const gchar *reason,
          const gchar *message,
          gboolean     use_handler)
{
  ClayCoreConfig  *config        = the_errors_clay->config;
#if !defined(G_OS_WIN32) || !defined(CLAY_CONSOLE_COMPILATION)
  gboolean         eek_handled   = FALSE;
#endif
  ClayDebugPolicy  debug_policy;

  /* Clay has 2 ways to handle termination signals and fatal errors: one
   * is the stack trace mode which is set at start as command line
   * option --stack-trace-mode, this won't change for the length of the
   * session and outputs a trace in terminal; the other is set in
   * preferences, outputs a trace in a GUI and can change anytime during
   * the session.
   * The GUI backtrace has priority if it is set.
   */
  g_object_get (G_OBJECT (config),
                "debug-policy", &debug_policy,
                NULL);

  /* Let's just always output on stdout at least so that there is a
   * trace if the rest fails. */
  g_printerr ("%s: %s: %s\n", full_prog_name, reason, message);

#if ! defined (G_OS_WIN32) || defined (HAVE_EXCHNDL)

  if (use_handler)
    {
#ifndef CLAY_CONSOLE_COMPILATION
      if (debug_policy != CLAY_DEBUG_POLICY_NEVER &&
          ! the_errors_clay->no_interface         &&
          backtrace_file)
        {
#ifndef G_OS_WIN32
          FILE     *fd;
#endif
          gboolean  has_backtrace = TRUE;

          /* If GUI backtrace enabled (it is disabled by default), it
           * takes precedence over the command line argument.
           */
#ifdef G_OS_WIN32
#ifdef ENABLE_RELOCATABLE_RESOURCES
          const gchar *claydebug = g_build_filename (clay_installation_directory (), "bin",
                                                     "clay-debug-tool-" CLAY_TOOL_VERSION ".exe", NULL);
#else
          const gchar *claydebug = BINDIR "/clay-debug-tool-" CLAY_TOOL_VERSION ".exe";
#endif
#elif defined (PLATFORM_OSX)
          const gchar *claydebug = "clay-debug-tool-" CLAY_TOOL_VERSION;
#elif !defined (G_OS_WIN32) && !defined (PLATFORM_OSX) && defined ENABLE_RELOCATABLE_RESOURCES
          const gchar *claydebug = g_build_filename (clay_installation_directory (),
                                                     "libexec", "clay-debug-tool-" CLAY_TOOL_VERSION, NULL);
#else
          const gchar *claydebug = LIBEXECDIR "/clay-debug-tool-" CLAY_TOOL_VERSION;
#endif
          gchar *args[9] = { (gchar *) claydebug, full_prog_name, NULL,
                             (gchar *) reason, (gchar *) message,
                             backtrace_file, the_errors_clay->config->last_known_release,
                             NULL, NULL };
          gchar  pid[16];
          gchar  timestamp[16];

          g_snprintf (pid, 16, "%u", (guint) getpid ());
          args[2] = pid;

          g_snprintf (timestamp, 16, "%"G_GINT64_FORMAT, the_errors_clay->config->last_release_timestamp);
          args[7] = timestamp;

#ifndef G_OS_WIN32
          /* On Win32, the trace has already been processed by ExcHnl
           * and is waiting for us in a text file.
           */
          fd = g_fopen (backtrace_file, "w");
          has_backtrace = clay_stack_trace_print ((const gchar *) full_prog_name,
                                                  fd, NULL);
          fclose (fd);
#endif

          /* We don't care about any return value. If it fails, too
           * bad, we just won't have any stack trace.
           * We still need to use the sync() variant because we have
           * to keep Clay up long enough for the debugger to get its
           * trace.
           */
          if (has_backtrace &&
              g_file_test (backtrace_file, G_FILE_TEST_IS_REGULAR) &&
              g_spawn_async (NULL, args, NULL,
                             G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL,
                             NULL, NULL, NULL, NULL))
            eek_handled = TRUE;
        }
#endif /* !CLAY_CONSOLE_COMPILATION */

#ifndef G_OS_WIN32
      if (! eek_handled)
        {
          switch (stack_trace_mode)
            {
            case CLAY_STACK_TRACE_NEVER:
              break;

            case CLAY_STACK_TRACE_QUERY:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  if (the_errors_clay)
                    clay_gui_ungrab (the_errors_clay);

                  clay_stack_trace_query ((const gchar *) full_prog_name);
                }
              break;

            case CLAY_STACK_TRACE_ALWAYS:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  clay_stack_trace_print ((const gchar *) full_prog_name,
                                          stdout, NULL);
                }
              break;

            default:
              break;
            }
        }
#endif /* ! G_OS_WIN32 */
    }
#endif /* ! G_OS_WIN32 || HAVE_EXCHNDL */

#if defined (G_OS_WIN32) && ! defined (CLAY_CONSOLE_COMPILATION)
  /* g_on_error_* don't do anything reasonable on Win32. */
  if (! eek_handled && ! the_errors_clay->no_interface)
    {
      char    *utf8  = g_strdup_printf ("%s: %s", reason, message);
      wchar_t *utf16 = g_utf8_to_utf16 (utf8, -1, NULL, NULL, NULL);

      MessageBoxW (NULL, utf16 ? utf16 : L"Generic error",
                   L"Clay", MB_OK | MB_ICONERROR);

      g_free (utf16);
      g_free (utf8);
    }
#endif

  exit (EXIT_FAILURE);
}
