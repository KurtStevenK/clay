/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayenv.c
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
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

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include "libclaybase/claywin32-io.h"
#endif

#include "claybasetypes.h"

#define __CLAY_ENV_C__
#include "clayenv.h"
#include "clayenv-private.h"
#include "clayversion.h"
#include "clayversion-private.h"
#include "clayreloc.h"

#ifdef G_OS_WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#ifndef S_IWUSR
# define S_IWUSR _S_IWRITE
#endif
#ifndef S_IWGRP
#define S_IWGRP (_S_IWRITE>>3)
#define S_IWOTH (_S_IWRITE>>6)
#endif
#ifndef S_ISDIR
# define __S_ISTYPE(mode, mask) (((mode) & _S_IFMT) == (mask))
# define S_ISDIR(mode)  __S_ISTYPE((mode), _S_IFDIR)
#endif
#define uid_t gint
#define gid_t gint
#define geteuid() 0
#define getegid() 0

#include <shlobj.h>

/* Constant available since Shell32.dll 4.72 */
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif

#endif


/**
 * SECTION: clayenv
 * @title: clayenv
 * @short_description: Functions to access the Clay environment.
 *
 * A set of functions to find the locations of Clay's data directories
 * and configuration files.
 **/


static gchar * clay_env_get_dir   (const gchar *clay_env_name,
                                   const gchar *compile_time_dir,
                                   const gchar *relative_subdir);


static gchar    *clay_temp_dir           = NULL;
static gboolean  clay_temp_dir_generated = FALSE;

CLAY_WARNING_API_BREAK("Move these variables to a private header.")
const guint clay_major_version = CLAY_MAJOR_VERSION;
const guint clay_minor_version = CLAY_MINOR_VERSION;
const guint clay_micro_version = CLAY_MICRO_VERSION;


/**
 * clay_env_init:
 * @plug_in: must be %TRUE if this function is called from a plug-in
 *
 * You should never call this function directly. While the symbol is
 * exported, the function is not declared in header and it is therefore
 * neither considered public nor stable.
 *
 * It is being called for you automatically (by means of the
 * [func@Clay.MAIN] macro that every C plug-in runs or directly with
 * [func@Clay.main] in binding). Calling it again will cause a fatal
 * error.
 *
 * Since: 2.4
 */
void
clay_env_init (gboolean plug_in)
{
  static gboolean  clay_env_initialized = FALSE;
  const gchar     *data_home = g_get_user_data_dir ();

  if (clay_env_initialized)
    g_error ("clay_env_init() must only be called once!");

  clay_env_initialized = TRUE;

#ifndef G_OS_WIN32
  if (plug_in)
    {
      _clay_reloc_init_lib (NULL);
    }
  else if (_clay_reloc_init (NULL))
    {
      /* Set $LD_LIBRARY_PATH to ensure that plugins can be loaded. */

      const gchar *ldpath = g_getenv ("LD_LIBRARY_PATH");
      gchar       *libdir = g_build_filename (clay_installation_directory (),
                                              "lib",
                                              NULL);

      if (ldpath && *ldpath)
        {
          gchar *tmp = g_strconcat (libdir, ":", ldpath, NULL);

          g_setenv ("LD_LIBRARY_PATH", tmp, TRUE);

          g_free (tmp);
        }
      else
        {
          g_setenv ("LD_LIBRARY_PATH", libdir, TRUE);
        }

      g_free (libdir);
    }
#endif

  /* The user data directory (XDG_DATA_HOME on Unix) is used to store
   * various data, like crash logs (win32) or recently used file history
   * (by GTK+). Yet it may be absent, in particular on non-Linux
   * platforms. Make sure it exists.
   */
  if (! g_file_test (data_home, G_FILE_TEST_IS_DIR))
    {
      if (g_mkdir_with_parents (data_home, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        {
          g_warning ("Failed to create the data directory '%s': %s",
                     data_home, g_strerror (errno));
        }
    }
}

/**
 * clay_env_exit:
 * @plug_in: must be %TRUE if this function is called from a plug-in
 *
 * You should never call this function directly. While the symbol is
 * exported, the function is not declared in header and it is therefore
 * neither considered public nor stable.
 *
 * It is being called for you automatically (by means of the
 * [func@Clay.MAIN] macro that every C plug-in runs or directly with
 * [func@Clay.main] in binding).
 *
 * Since: 3.2.2
 */
void
clay_env_exit (gboolean plug_in)
{
  if (clay_temp_dir_generated)
    {
      if (g_rmdir (clay_temp_dir) == -1)
        g_printerr ("%s: failed to delete the temporary folder `%s`: %s\n",
                    G_STRFUNC, clay_temp_dir, g_strerror (errno));
    }

  g_clear_pointer (&clay_temp_dir, g_free);
}

#ifdef G_OS_WIN32

static char *
get_known_folder (REFKNOWNFOLDERID id)
{
  wchar_t *path_utf16 = NULL;
  char    *path       = NULL;

  if (SUCCEEDED (SHGetKnownFolderPath (id, KF_FLAG_DEFAULT, NULL, &path_utf16)))
    path = g_utf16_to_utf8 (path_utf16, -1, NULL, NULL, NULL);

  if (path_utf16)
    CoTaskMemFree (path_utf16);

  return path;
}

extern IMAGE_DOS_HEADER __ImageBase;

static HMODULE
this_module (void)
{
  return (HMODULE) &__ImageBase;
}

#endif

/**
 * clay_directory:
 *
 * Returns the user-specific Clay settings directory. If the
 * environment variable `CLAY3_DIRECTORY` exists, it is used. If it is
 * an absolute path, it is used as is. If it is a relative path, it
 * is taken to be a subdirectory of the home directory. If it is a
 * relative path, and no home directory can be determined, it is taken
 * to be a subdirectory of [func@Clay.data_directory].
 *
 * The usual case is that no `CLAY3_DIRECTORY` environment variable
 * exists, and then we use a subdirectory of the local configuration
 * directory (which subdirectory may vary depending on build setup):
 *
 * - UNIX: `$XDG_CONFIG_HOME` (defaults to `$HOME/.config/`)
 *
 * - Windows: `CSIDL_APPDATA`
 *
 * - OSX (UNIX exception): the Application Support Directory.
 *
 * If neither the configuration nor home directory exist,
 * [func@GLib.get_user_config_dir] will return
 * `{tmp}/{user_name}/.config/` where the temporary directory `{tmp}` and
 * the `{user_name}` are determined according to platform rules.
 *
 * In any case, we always return some non-empty string, whether it
 * corresponds to an existing directory or not.
 *
 * In config files such as `clayrc`, the string "${clay_dir}" expands to
 * this directory.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always
 * UTF-8.)
 *
 * Returns: (type filename): The user-specific Clay settings directory.
 **/
const gchar *
clay_directory (void)
{
  static gchar *clay_dir          = NULL;
  static gchar *last_env_clay_dir = NULL;

  const gchar  *env_clay_dir;

  env_clay_dir = g_getenv ("CLAY3_DIRECTORY");

  if (clay_dir)
    {
      gboolean clay3_directory_changed = FALSE;

      /* We have constructed the clay_dir already. We can return
       * clay_dir unless some parameter clay_dir depends on has
       * changed. For now we just check for changes to CLAY3_DIRECTORY
       */
      clay3_directory_changed =
        (env_clay_dir == NULL &&
         last_env_clay_dir != NULL) ||
        (env_clay_dir != NULL &&
         last_env_clay_dir == NULL) ||
        (env_clay_dir != NULL &&
         last_env_clay_dir != NULL &&
         strcmp (env_clay_dir, last_env_clay_dir) != 0);

      if (! clay3_directory_changed)
        {
          return clay_dir;
        }
      else
        {
          /* Free the old clay_dir and go on to update it */
          g_free (clay_dir);
          clay_dir = NULL;
        }
    }

  /* Remember the CLAY3_DIRECTORY to next invocation so we can check
   * if it changes
   */
  g_free (last_env_clay_dir);
  last_env_clay_dir = g_strdup (env_clay_dir);

  if (env_clay_dir)
    {
      if (g_path_is_absolute (env_clay_dir))
        {
          clay_dir = g_strdup (env_clay_dir);
        }
      else
        {
          const gchar *home_dir = g_get_home_dir ();

          if (home_dir)
            clay_dir = g_build_filename (home_dir, env_clay_dir, NULL);
          else
            clay_dir = g_build_filename (clay_data_directory (), env_clay_dir, NULL);
        }
    }
  else if (g_path_is_absolute (CLAYDIR))
    {
      clay_dir = g_strdup (CLAYDIR);
    }
  else
    {
#ifdef PLATFORM_OSX

      NSAutoreleasePool *pool;
      NSArray           *path;
      NSString          *library_dir;

      pool = [[NSAutoreleasePool alloc] init];

      path = NSSearchPathForDirectoriesInDomains (NSApplicationSupportDirectory,
                                                  NSUserDomainMask, YES);
      library_dir = [path objectAtIndex:0];

      clay_dir = g_build_filename ([library_dir UTF8String],
                                   CLAYDIR, CLAY_USER_VERSION, NULL);

      [pool drain];

#elif defined G_OS_WIN32

      char *conf_dir = get_known_folder (&FOLDERID_RoamingAppData);

      clay_dir = g_build_filename (conf_dir,
                                   CLAYDIR, CLAY_USER_VERSION, NULL);
      g_free(conf_dir);

#else /* UNIX */

      const gchar *snap_path;

      if (g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS))
        {                       /* Linux flatpak version */
          const gchar *host_xdg_config_home = g_getenv ("HOST_XDG_CONFIG_HOME");

          if (host_xdg_config_home == NULL)
            clay_dir =  g_build_filename (g_get_home_dir (),
                                          ".config",
                                          CLAYDIR, CLAY_USER_VERSION,
                                          NULL);
          else
            clay_dir =  g_build_filename (host_xdg_config_home,
                                          CLAYDIR, CLAY_USER_VERSION,
                                          NULL);
        }

      snap_path = g_getenv ("SNAP");
      if (snap_path && g_file_test (snap_path, G_FILE_TEST_IS_DIR))
        {
          const gchar *snap_real_home = g_getenv ("SNAP_REAL_HOME");

          if (snap_real_home == NULL)
            clay_dir =  g_build_filename (g_get_home_dir (),
                                          ".config",
                                          CLAYDIR, CLAY_USER_VERSION,
                                          NULL);
          else
            clay_dir =  g_build_filename (snap_real_home,
                                          ".config",
                                          CLAYDIR, CLAY_USER_VERSION,
                                          NULL);
        }

      if (clay_dir == NULL)
        {
          /* g_get_user_config_dir () always returns a path as a non-null
           * and non-empty string
           */
          clay_dir = g_build_filename (g_get_user_config_dir (),
                                       CLAYDIR, CLAY_USER_VERSION, NULL);
        }

#endif /* PLATFORM_OSX */
    }

  return clay_dir;
}

/**
 * clay_installation_directory:
 *
 * Returns the top installation directory of Clay. On Unix the
 * compile-time defined installation prefix is used. On Windows, the
 * installation directory as deduced from the executable's full
 * filename is used. On OSX we ask `[NSBundle mainBundle]` for the
 * resource path to check if Clay is part of a relocatable bundle.
 *
 * In config files such as clayrc, the string "${clay_installation_dir}"
 * expands to this directory.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8.)
 *
 * Since: 2.8
 *
 * Returns: (type filename): The toplevel installation directory of Clay.
 **/
const gchar *
clay_installation_directory (void)
{
  static gchar *toplevel = NULL;

  if (toplevel)
    return toplevel;

#ifdef G_OS_WIN32

  toplevel = g_win32_get_package_installation_directory_of_module (this_module ());
  if (! toplevel)
    g_error ("g_win32_get_package_installation_directory_of_module() failed");

#elif PLATFORM_OSX

  {
    NSAutoreleasePool *pool;
    NSString          *resource_path;
    gchar             *resource_path_test;
    NSString          *app_path;
    gchar             *basename;
    gchar             *basepath;
    gchar             *dirname;

    pool = [[NSAutoreleasePool alloc] init];

    resource_path = [[NSBundle mainBundle] resourcePath];
    app_path = [[NSBundle mainBundle] bundlePath];

    resource_path_test = g_build_filename([resource_path UTF8String], "share",
                                          CLAY_PACKAGE, CLAY_DATA_VERSION, NULL);
    if (g_file_test (resource_path_test, G_FILE_TEST_IS_DIR))
      {
        /* Legacy CircleCI era relocatable code */
        basename = g_path_get_basename ([resource_path UTF8String]);
        basepath = g_path_get_dirname ([resource_path UTF8String]);
      }
    else
      {
         /* Modern GitLab CI era relocatable code */
        basename = g_path_get_basename ([app_path UTF8String]);
        basepath = g_path_get_dirname ([app_path UTF8String]);
      }
    dirname  = g_path_get_basename (basepath);

    if (! strcmp (basename, ".libs"))
      {
        /*  we are running from the source dir, do normal unix things  */

        toplevel = _clay_reloc_find_prefix (PREFIX);
      }
    else if (! strcmp (basename, "bin"))
      {
        /*  we are running the main app, but not from a bundle, the resource
         *  path is the directory which contains the executable
         */

        toplevel = g_strdup (basepath);
      }
    else if (! strcmp (dirname, "plug-ins") ||
             ! strcmp (dirname, "extensions"))
      {
        /*  same for plug-ins and extensions in subdirectory, go three
         *  levels up from prefix/lib/clay/x.y
         */

        gchar *tmp  = g_path_get_dirname (basepath);
        gchar *tmp2 = g_path_get_dirname (tmp);
        gchar *tmp3 = g_path_get_dirname (tmp2);

        toplevel = g_path_get_dirname (tmp3);

        g_free (tmp);
        g_free (tmp2);
        g_free (tmp3);
      }
    else if (strstr (basepath, "/Cellar/"))
      {
        /*  we are running from a Python.framework bundle built in homebrew
         *  during the build phase
         */

        gchar *fulldir = g_strdup (basepath);
        gchar *lastdir = g_path_get_basename (fulldir);
        gchar *tmp_fulldir;

        while (strcmp (lastdir, "Cellar"))
          {
            tmp_fulldir = g_path_get_dirname (fulldir);

            g_free (lastdir);
            g_free (fulldir);

            fulldir = tmp_fulldir;
            lastdir = g_path_get_basename (fulldir);
          }
        toplevel = g_path_get_dirname (fulldir);

        g_free (fulldir);
        g_free (lastdir);
      }
    else
      {
        /*  if none of the above match, we assume that we are really in a bundle  */

        if (g_file_test (resource_path_test, G_FILE_TEST_IS_DIR))
          {
            /* Legacy CircleCI era relocatable prefix */
            toplevel = g_strdup ([resource_path UTF8String]);
          }
        else
          {
            /* Modern GitLab CI era relocatable prefix */
            toplevel = g_strconcat ([app_path UTF8String], "/Contents", NULL);
          }
      }

    g_free (basename);
    g_free (basepath);
    g_free (dirname);
    g_free (resource_path_test);

    [pool drain];
  }

#else

  toplevel = _clay_reloc_find_prefix (PREFIX);

#endif

  return toplevel;
}

/**
 * clay_data_directory:
 *
 * Returns the default top directory for Clay data. If the environment
 * variable `CLAY3_DATADIR` exists, that is used. It should be an
 * absolute pathname. Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as
 * deduced from the executable's full filename is used.
 *
 * Note that the actual directories used for Clay data files can be
 * overridden by the user in the preferences dialog.
 *
 * In config files such as clayrc, the string "${clay_data_dir}" expands
 * to this directory.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8).
 *
 * Returns: (type filename): The top directory for Clay data.
 **/
const gchar *
clay_data_directory (void)
{
  static gchar *clay_data_dir = NULL;

  if (! clay_data_dir)
    {
      gchar *tmp = g_build_filename ("share",
                                     CLAY_PACKAGE,
                                     CLAY_DATA_VERSION,
                                     NULL);

      clay_data_dir = clay_env_get_dir ("CLAY3_DATADIR", CLAYDATADIR, tmp);
      g_free (tmp);
    }

  return clay_data_dir;
}

/**
 * clay_locale_directory:
 *
 * Returns the top directory for Clay locale files. If the environment
 * variable `CLAY3_LOCALEDIR` exists, that is used. It should be an
 * absolute pathname. Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8).
 *
 * On UNIX the returned string can be passed directly to the `bindtextdomain()`
 * function from `libintl`; on Windows the returned string can be converted to
 * UTF-16 and passed to the `wbindtextdomain()` function from `libintl`.
 *
 * Returns: (type filename): The top directory for Clay locale files.
 */
const gchar *
clay_locale_directory (void)
{
  static gchar *clay_locale_dir = NULL;

  if (! clay_locale_dir)
    {
      gchar *tmp = g_build_filename ("share",
                                     "locale",
                                     NULL);

      clay_locale_dir = clay_env_get_dir ("CLAY3_LOCALEDIR", LOCALEDIR, tmp);
      g_free (tmp);
    }

  return clay_locale_dir;
}

/**
 * clay_sysconf_directory:
 *
 * Returns the top directory for Clay config files. If the environment
 * variable `CLAY3_SYSCONFDIR` exists, that is used.  It should be an
 * absolute pathname. Otherwise, on Unix the compile-time defined
 * directory is used. On Windows, the installation directory as deduced
 * from the executable's full filename is used.
 *
 * In config files such as clayrc, the string "${clay_sysconf_dir}"
 * expands to this directory.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8).
 *
 * Returns: (type filename): The top directory for Clay config files.
 **/
const gchar *
clay_sysconf_directory (void)
{
  static gchar *clay_sysconf_dir = NULL;

  if (! clay_sysconf_dir)
    {
      gchar *tmp = g_build_filename ("etc",
                                     CLAY_PACKAGE,
                                     CLAY_SYSCONF_VERSION,
                                     NULL);

      clay_sysconf_dir = clay_env_get_dir ("CLAY3_SYSCONFDIR", CLAYSYSCONFDIR, tmp);
      g_free (tmp);
    }

  return clay_sysconf_dir;
}

/**
 * clay_plug_in_directory:
 *
 * Returns the default top directory for Clay plug-ins and modules. If
 * the environment variable `CLAY3_PLUGINDIR` exists, that is used.  It
 * should be an absolute pathname. Otherwise, on Unix the compile-time
 * defined directory is used. On Windows, the installation directory
 * as deduced from the executable's full filename is used.
 *
 * Note that the actual directories used for Clay plug-ins and modules
 * can be overridden by the user in the preferences dialog.
 *
 * In config files such as clayrc, the string "${clay_plug_in_dir}"
 * expands to this directory.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8).
 *
 * Returns: (type filename): The top directory for Clay plug_ins and modules.
 **/
const gchar *
clay_plug_in_directory (void)
{
  static gchar *clay_plug_in_dir = NULL;

  if (! clay_plug_in_dir)
    {
      gchar *tmp = g_build_filename ("lib",
                                     CLAY_PACKAGE,
                                     CLAY_PLUGIN_VERSION,
                                     NULL);

      clay_plug_in_dir = clay_env_get_dir ("CLAY3_PLUGINDIR", PLUGINDIR, tmp);
      g_free (tmp);
    }

  return clay_plug_in_dir;
}

/**
 * clay_cache_directory:
 *
 * Returns the default top directory for Clay cached files. If the
 * environment variable `CLAY3_CACHEDIR` exists, that is used.  It
 * should be an absolute pathname.  Otherwise, a subdirectory of the
 * directory returned by [GLib.get_user_cache_dir] is used.
 *
 * Note that the actual directories used for Clay caches files can
 * be overridden by the user in the preferences dialog.
 *
 * In config files such as clayrc, the string "${clay_cache_dir}"
 * expands to this directory.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8).
 *
 * Since: 2.10.10
 *
 * Returns: (type filename): The default top directory for Clay cached files.
 **/
const gchar *
clay_cache_directory (void)
{
  static gchar *clay_cache_dir = NULL;

  if (! clay_cache_dir)
    {
      gchar *tmp = g_build_filename (g_get_user_cache_dir (),
                                     CLAY_PACKAGE,
                                     CLAY_USER_VERSION,
                                     NULL);

      clay_cache_dir = clay_env_get_dir ("CLAY3_CACHEDIR", NULL, tmp);
      g_free (tmp);
    }

  return clay_cache_dir;
}

/**
 * clay_temp_directory:
 *
 * Returns the default top directory for Clay temporary files. If the
 * environment variable `CLAY3_TEMPDIR` exists, that is used. It
 * should be an absolute pathname. Otherwise, a subdirectory of the
 * directory returned by [func@GLib.get_tmp_dir] is used.
 *
 * In config files such as clayrc, the string "${clay_temp_dir}" expands
 * to this directory.
 *
 * Note that the actual directories used for Clay temporary files can
 * be overridden by the user in the preferences dialog.
 *
 * The returned string is owned by Clay and must not be modified or
 * freed. The returned string is in the encoding used for filenames by
 * GLib, which isn't necessarily UTF-8 (on Windows it is always UTF-8).
 *
 * The returned directory path might already exists, or it might not. It
 * is your responsibility to make sure it does before using it.
 *
 * Since: 2.10.10
 *
 * Returns: (type filename): The default top directory for Clay temporary files.
 **/
const gchar *
clay_temp_directory (void)
{
  if (! clay_temp_dir)
    {
      GError *error = NULL;

      clay_temp_dir = clay_env_get_dir ("CLAY3_TEMPDIR", NULL, NULL);
      if (clay_temp_dir)
        return clay_temp_dir;

      clay_temp_dir = g_dir_make_tmp (CLAY_PACKAGE "-" CLAY_USER_VERSION "-XXXXXXX", &error);
      clay_temp_dir_generated = TRUE;
      if (clay_temp_dir == NULL)
        {
          g_critical ("%s: failed to create temporary directory: %s\n",
                      G_STRFUNC, error->message);
          g_clear_error (&error);

          clay_temp_dir_generated = FALSE;
        }
    }

  return clay_temp_dir;
}

static GFile *
clay_child_file (const gchar *parent,
                 const gchar *element,
                 va_list      args)
{
  GFile *file = g_file_new_for_path (parent);

  while (element)
    {
      GFile *child = g_file_get_child (file, element);

      g_object_unref (file);
      file = child;

      element = va_arg (args, const gchar *);
    }

  return file;
}

/**
 * clay_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 user's Clay directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the user's Clay directory, or the Clay
 * directory itself if @first_element is %NULL.
 *
 * See also: [func@Clay.directory].
 *
 * Since: 2.10
 *
 * Returns: (transfer full): a new @GFile for the path.
 **/
GFile *
clay_directory_file (const gchar *first_element,
                     ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = clay_child_file (clay_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * clay_installation_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 top installation directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the installation directory, or the installation
 * directory itself if @first_element is %NULL.
 *
 * See also: [func@Clay.installation_directory].
 *
 * Since: 2.10.10
 *
 * Returns: (transfer full): a new @GFile for the path.
 **/
GFile *
clay_installation_directory_file (const gchar *first_element,
                                  ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = clay_child_file (clay_installation_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * clay_data_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 data directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the data directory, or the data directory
 * itself if @first_element is %NULL.
 *
 * See also: [func@Clay.data_directory].
 *
 * Since: 2.10
 *
 * Returns: (transfer full): a new @GFile for the path.
 **/
GFile *
clay_data_directory_file (const gchar *first_element,
                          ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = clay_child_file (clay_data_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * clay_locale_directory_file: (skip)
 * @first_element: the first element of a path to a file in the
 *                 locale directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the locale directory, or the locale directory
 * itself if @first_element is %NULL.
 *
 * See also: [func@Clay.locale_directory].
 *
 * Since: 2.10
 *
 * Returns: (transfer full): a new @GFile for the path.
 **/
GFile *
clay_locale_directory_file (const gchar *first_element,
                            ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = clay_child_file (clay_locale_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * clay_sysconf_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 sysconf directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the sysconf directory, or the sysconf directory
 * itself if @first_element is %NULL.
 *
 * See also: [func@Clay.sysconf_directory].
 *
 * Since: 2.10
 *
 * Returns: (transfer full): a new @GFile for the path.
 **/
GFile *
clay_sysconf_directory_file (const gchar *first_element,
                             ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = clay_child_file (clay_sysconf_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * clay_plug_in_directory_file:
 * @first_element: the first element of a path to a file in the
 *                 plug-in directory, or %NULL.
 * @...: a %NULL terminated list of the remaining elements of the path
 *       to the file.
 *
 * Returns a #GFile in the plug-in directory, or the plug-in directory
 * itself if @first_element is %NULL.
 *
 * See also: clay_plug_in_directory().
 *
 * Since: 2.10
 *
 * Returns: (transfer full):
 *          a new @GFile for the path, Free with g_object_unref().
 **/
GFile *
clay_plug_in_directory_file (const gchar *first_element,
                             ...)
{
  GFile   *file;
  va_list  args;

  va_start (args, first_element);
  file = clay_child_file (clay_plug_in_directory (), first_element, args);
  va_end (args);

  return file;
}

/**
 * clay_path_runtime_fix:
 * @path: A pointer to a string (allocated with g_malloc) that is
 *        (or could be) a pathname.
 *
 * On Windows, this function checks if the string pointed to by @path
 * starts with the compile-time prefix, and in that case, replaces the
 * prefix with the run-time one.  @path should be a pointer to a
 * dynamically allocated (with g_malloc, g_strconcat, etc) string. If
 * the replacement takes place, the original string is deallocated,
 * and *@path is replaced with a pointer to a new string with the
 * run-time prefix spliced in.
 *
 * On Linux and other Unices, it does the same thing, but only if BinReloc
 * support is enabled, and only if we are not running Clay in-build directory.
 */
static void
clay_path_runtime_fix (gchar **path)
{
#if defined (G_OS_WIN32) && defined (PREFIX)
  gchar *p;

  /* Yes, I do mean forward slashes below */
  if (strncmp (*path, PREFIX "/", strlen (PREFIX "/")) == 0)
    {
      /* This is a compile-time entry. Replace the path with the
       * real one on this machine.
       */
      p = *path;
      *path = g_strconcat (clay_installation_directory (),
                           "\\",
                           *path + strlen (PREFIX "/"),
                           NULL);
      g_free (p);
    }
  /* Replace forward slashes with backslashes, just for
   * completeness */
  p = *path;
  while ((p = strchr (p, '/')) != NULL)
    {
      *p = '\\';
      p++;
    }
#elif defined (G_OS_WIN32)
  /* without defineing PREFIX do something useful too */
  gchar *p = *path;
  if (!g_path_is_absolute (p))
    {
      *path = g_build_filename (clay_installation_directory (), *path, NULL);
      g_free (p);
    }
#elif defined (ENABLE_RELOCATABLE_RESOURCES)
  gchar *p;

  /* XXX: I could actually test any of the other CLAY_TESTING_* environment
   * variables. The goal is only to check if we are running Clay from within the
   * build directory. In such case, no substitution should happen.
   */
  if (! g_getenv ("CLAY_TESTING_PLUGINDIRS") &&
      strncmp (*path, PREFIX G_DIR_SEPARATOR_S,
               strlen (PREFIX G_DIR_SEPARATOR_S)) == 0)
    {
      /* This is a compile-time entry. Replace the path with the
       * real one on this machine.
       */
      p = *path;
      *path = g_build_filename (clay_installation_directory (),
                                *path + strlen (PREFIX G_DIR_SEPARATOR_S),
                                NULL);
      g_free (p);
    }
#endif
}

/**
 * clay_parse_search_path:
 * @path: (type filename): A list of directories separated by [const@GLib.SEARCHPATH_SEPARATOR].
 * @max_paths:    The maximum number of directories to return.
 * @check:        %TRUE if you want the directories to be checked.
 * @check_failed: (element-type filename) (out callee-allocates): a #GList of path elements for which the check failed.
 *
 * Returns: (element-type filename) (transfer full): A #GList of all directories in @path.
 **/
GList *
clay_parse_search_path (const gchar  *path,
                        gint          max_paths,
                        gboolean      check,
                        GList       **check_failed)
{
  gchar    **patharray;
  GList     *list      = NULL;
  GList     *fail_list = NULL;
  gint       i;
  gboolean   exists    = TRUE;

  if (!path || !*path || max_paths < 1 || max_paths > 256)
    return NULL;

  patharray = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, max_paths);

  for (i = 0; i < max_paths; i++)
    {
      GString *dir;

      if (! patharray[i])
        break;

#ifndef G_OS_WIN32
      if (*patharray[i] == '~')
        {
          dir = g_string_new (g_get_home_dir ());
          g_string_append (dir, patharray[i] + 1);
        }
      else
#endif
        {
          clay_path_runtime_fix (&patharray[i]);
          dir = g_string_new (patharray[i]);
        }

      if (check)
        exists = g_file_test (dir->str, G_FILE_TEST_IS_DIR);

      if (exists)
        {
          GList *dup;

          /*  check for duplicate entries, see bug #784502  */
          for (dup = list; dup; dup = g_list_next (dup))
            {
              if (! strcmp (dir->str, dup->data))
                break;
            }

          /*  only add to the list if it's not a duplicate  */
          if (! dup)
            list = g_list_prepend (list, g_strdup (dir->str));
        }
      else if (check_failed)
        {
          fail_list = g_list_prepend (fail_list, g_strdup (dir->str));
        }

      g_string_free (dir, TRUE);
    }

  g_strfreev (patharray);

  list = g_list_reverse (list);

  if (check && check_failed)
    {
      fail_list = g_list_reverse (fail_list);
      *check_failed = fail_list;
    }

  return list;
}

/**
 * clay_create_search_path:
 * @paths: (element-type filename): A list of directories as returned by [func@Clay.parse_search_path].
 *
 * Returns: (type filename) (transfer full): A searchpath string separated
 *          by [const@GLib.SEARCHPATH_SEPARATOR].
 **/
gchar *
clay_create_search_path (GList *paths)
{
  GString *str    = NULL;
  GList   *list;
  gchar   *retval = NULL;

  for (list = paths; list; list = g_list_next (list))
    {
      gchar *dir = list->data;

      if (str)
        {
          g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
          g_string_append (str, dir);
        }
      else
        {
          str = g_string_new (dir);
        }
    }

  if (str)
    retval = g_string_free (str, FALSE);

  return retval;
}

/**
 * clay_free_paths:
 * @paths: (element-type filename): A list of directories as returned by [func@Clay.parse_search_path].
 *
 * This function frees the memory allocated for the list and the strings
 * it contains.
 **/
void
clay_free_paths (GList *paths)
{
  g_list_free_full (paths, (GDestroyNotify) g_free);
}

/**
 * clay_get_user_writable_dir:
 * @paths: (element-type filename): A list of directories as returned by [func@Clay.parse_search_path].
 *
 * Returns: (transfer full): The first directory in @paths where the user has write permission.
 **/
gchar *
clay_get_user_writable_dir (GList *paths)
{
  GList    *list;
  uid_t     euid;
  gid_t     egid;
  GStatBuf  filestat;
  gint      err;

  g_return_val_if_fail (paths != NULL, NULL);

  euid = geteuid ();
  egid = getegid ();

  for (list = paths; list; list = g_list_next (list))
    {
      gchar *dir = list->data;

      /*  check if directory exists  */
      err = g_stat (dir, &filestat);

      /*  this is tricky:
       *  if a file is e.g. owned by the current user but not user-writable,
       *  the user has no permission to write to the file regardless
       *  of their group's or other's write permissions
       */
      if (!err && S_ISDIR (filestat.st_mode) &&

          ((filestat.st_mode & S_IWUSR) ||

           ((filestat.st_mode & S_IWGRP) &&
            (euid != filestat.st_uid)) ||

           ((filestat.st_mode & S_IWOTH) &&
            (euid != filestat.st_uid) &&
            (egid != filestat.st_gid))))
        {
          return g_strdup (dir);
        }
    }

  return NULL;
}


/* Deprecated Public Functions */

/**
 * clay_path_parse:
 * @path: (type filename): A list of directories separated by [const@GLib.SEARCHPATH_SEPARATOR].
 * @max_paths:    The maximum number of directories to return.
 * @check:        %TRUE if you want the directories to be checked.
 * @check_failed: (element-type filename) (out callee-allocates): a #GList of path elements for which the check failed.
 *
 * Returns: (element-type filename) (transfer full): A #GList of all directories in @path.
 *
 * Deprecated: 3.4: Use [func@Clay.parse_search_path] instead.
 **/
GList *
clay_path_parse (const gchar  *path,
                 gint          max_paths,
                 gboolean      check,
                 GList       **check_failed)
{
  return clay_parse_search_path (path, max_paths, check, check_failed);
}

/**
 * clay_path_to_str:
 * @path: (element-type filename): A list of directories as returned by `clay_path_parse()`.
 *
 * Returns: (type filename) (transfer full): A searchpath string separated
 *          by [const@GLib.SEARCHPATH_SEPARATOR].
 *
 * Deprecated: 3.4: Use [func@Clay.create_search_path] instead.
 **/
gchar *
clay_path_to_str (GList *path)
{
  return clay_create_search_path (path);
}

/**
 * clay_path_free:
 * @path: (element-type filename): A list of directories as returned by `clay_path_parse()`.
 *
 * This function frees the memory allocated for the list and the strings
 * it contains.
 *
 * Deprecated: 3.4: Use [func@Clay.free_paths] instead.
 **/
void
clay_path_free (GList *path)
{
  clay_free_paths (path);
}

/**
 * clay_path_get_user_writable_dir:
 * @path: (element-type filename): A list of directories as returned by `clay_path_parse()`.
 *
 * Returns: (transfer full): The first directory in @path where the user has write permission.
 *
 * Deprecated: 3.4: Use [func@Clay.get_user_writable_dir] instead.
 **/
gchar *
clay_path_get_user_writable_dir (GList *path)
{
  return clay_get_user_writable_dir (path);
}


/* Private Functions */

static gchar *
clay_env_get_dir (const gchar *clay_env_name,
                  const gchar *compile_time_dir,
                  const gchar *relative_subdir)
{
  const gchar *env = g_getenv (clay_env_name);

  if (env)
    {
      if (! g_path_is_absolute (env))
        g_error ("%s environment variable should be an absolute path.",
                 clay_env_name);

      return g_strdup (env);
    }
  else if (compile_time_dir)
    {
      gchar *retval = g_strdup (compile_time_dir);

      clay_path_runtime_fix (&retval);

      return retval;
    }
  else if (relative_subdir && ! g_path_is_absolute (relative_subdir))
    {
      return g_build_filename (clay_installation_directory (),
                               relative_subdir,
                               NULL);
    }

  return g_strdup (relative_subdir);
}
