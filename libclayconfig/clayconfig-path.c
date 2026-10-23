/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayconfig-path.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <gio/gio.h>

#include "libclaybase/claybase.h"

#include "clayconfig-error.h"
#include "clayconfig-path.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: clayconfig-path
 * @title: ClayConfig-path
 * @short_description: File path utilities for libclayconfig.
 *
 * File path utilities for libclayconfig.
 **/


/**
 * clay_config_path_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a ClayConfigPath string property
 *
 * Since: 2.4
 **/
GType
clay_config_path_get_type (void)
{
  static GType path_type = 0;

  if (! path_type)
    {
      const GTypeInfo type_info = { 0, };

      path_type = g_type_register_static (G_TYPE_STRING, "ClayConfigPath",
                                          &type_info, 0);
    }

  return path_type;
}


/*
 * CLAY_TYPE_PARAM_CONFIG_PATH
 */

#define CLAY_PARAM_SPEC_CONFIG_PATH(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_CONFIG_PATH, ClayParamSpecConfigPath))

typedef struct _ClayParamSpecConfigPath ClayParamSpecConfigPath;

struct _ClayParamSpecConfigPath
{
  GParamSpecString    parent_instance;

  ClayConfigPathType  type;
};

static void  clay_param_config_path_class_init (GParamSpecClass *class);

/**
 * clay_param_config_path_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a directory path object
 *
 * Since: 2.4
 **/
GType
clay_param_config_path_get_type (void)
{
  static GType spec_type = 0;

  if (! spec_type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_config_path_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecConfigPath),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_STRING,
                                          "ClayParamConfigPath",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
clay_param_config_path_class_init (GParamSpecClass *class)
{
  class->value_type = CLAY_TYPE_CONFIG_PATH;
}

/**
 * clay_param_spec_config_path:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @type:          a #ClayConfigPathType value.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a filename, dir name,
 * or list of file or dir names.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
clay_param_spec_config_path (const gchar        *name,
                             const gchar        *nick,
                             const gchar        *blurb,
                             ClayConfigPathType  type,
                             const gchar        *default_value,
                             GParamFlags         flags)
{
  GParamSpecString *pspec;

  pspec = g_param_spec_internal (CLAY_TYPE_PARAM_CONFIG_PATH,
                                 name, nick, blurb, flags);

  pspec->default_value = g_strdup (default_value);

  CLAY_PARAM_SPEC_CONFIG_PATH (pspec)->type = type;

  return G_PARAM_SPEC (pspec);
}

/**
 * clay_param_spec_config_path_type:
 * @pspec:         A #GParamSpec for a path param
 *
 * Tells whether the path param encodes a filename,
 * dir name, or list of file or dir names.
 *
 * Returns: a #ClayConfigPathType value
 *
 * Since: 2.4
 **/
ClayConfigPathType
clay_param_spec_config_path_type (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_CONFIG_PATH (pspec), 0);

  return CLAY_PARAM_SPEC_CONFIG_PATH (pspec)->type;
}


/*
 * ClayConfig path utilities
 */

static gchar        * clay_config_path_expand_only   (const gchar  *path,
                                                      GError      **error) G_GNUC_MALLOC;
static inline gchar * clay_config_path_extract_token (const gchar **str);
static gchar        * clay_config_path_unexpand_only (const gchar  *path) G_GNUC_MALLOC;


/**
 * clay_config_build_data_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the `clayrc` file. The path
 * returned by [func@Clay.ConfigInterface.build_data_path] includes first a
 * directory below the user's clay directory then one in the system-wide
 * data directory.
 *
 * Note that you cannot use this path directly with [func@Clay.parse_search_path].
 * As it is in the `clayrc` notation, you first need to expand and
 * recode it using [func@Clay.ConfigPath.expand].
 *
 * Returns: a newly allocated string
 *
 * Since: 2.4
 **/
gchar *
clay_config_build_data_path (const gchar *name)
{
  if (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"))
    /* Unit-testing mode: the source directory is where data is found. */
    return g_strconcat (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"),
                        G_DIR_SEPARATOR_S, "data",
                        G_DIR_SEPARATOR_S, name,
                        NULL);
  else
    return g_strconcat ("${clay_dir}", G_DIR_SEPARATOR_S, name,
                        G_SEARCHPATH_SEPARATOR_S,
                        "${clay_data_dir}", G_DIR_SEPARATOR_S, name,
                        NULL);
}

/**
 * clay_config_build_plug_in_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the clayrc file. The path
 * returned by [func@Clay.ConfigInterface.build_plug_in_path] includes first a
 * directory below the user's clay directory then one in the system-wide
 * plug-in directory.
 *
 * Note that you cannot use this path directly with [func@Clay.parse_search_path].
 * As it is in the `clayrc` notation, you first need to expand and
 * recode it using [func@Clay.ConfigPath.expand]
 *
 * Returns: a newly allocated string
 *
 * Since: 2.4
 **/
gchar *
clay_config_build_plug_in_path (const gchar *name)
{
  return g_strconcat ("${clay_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${clay_plug_in_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}

/**
 * clay_config_build_writable_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the clayrc file. The path
 * returned by `func@Clay.ConfigInterface.build_writable_path] is just the writable
 * parts of the search path constructed by [func@Clay.ConfigInterface.build_data_path].
 *
 * Note that you cannot use this path directly with [func@Clay.parse_search_path].
 * As it is in the clayrc notation, you first need to expand and
 * recode it using [func@Clay.ConfigPath.expand].
 *
 * Returns: a newly allocated string
 *
 * Since: 2.4
 **/
gchar *
clay_config_build_writable_path (const gchar *name)
{
  return g_strconcat ("${clay_dir}", G_DIR_SEPARATOR_S, name, NULL);
}

/**
 * clay_config_build_system_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the clayrc file. The path
 * returned by [func@Clay.ConfigInterface.build_system_path] is just the read-only
 * parts of the search path constructed by [func@Clay.ConfigInterface.build_plug_in_path].
 *
 * Note that you cannot use this path directly with [func@Clay.parse_search_path].
 * As it is in the clayrc notation, you first need to expand and
 * recode it using [func@Clay.ConfigPath.expand].
 *
 * Returns: a newly allocated string
 *
 * Since: 2.10.6
 **/
gchar *
clay_config_build_system_path (const gchar *name)
{
  return g_strconcat ("${clay_plug_in_dir}", G_DIR_SEPARATOR_S, name, NULL);
}

/**
 * clay_config_path_expand:
 * @path:   a NUL-terminated string in UTF-8 encoding
 * @recode: whether to convert to the filesystem's encoding
 * @error:  return location for errors
 *
 * Paths as stored in clayrc and other config files have to be treated
 * special. The string may contain special identifiers such as for
 * example "${clay_dir}" that have to be substituted before use. Also
 * the user's filesystem may be in a different encoding than UTF-8
 * (which is what is used for the `clayrc`). This function does the
 * variable substitution for you and can also attempt to convert to
 * the filesystem encoding.
 *
 * To reverse the expansion, use [func@Clay.ConfigPath.unexpand].
 *
 * Returns: a newly allocated NUL-terminated string
 *
 * Since: 2.4
 **/
gchar *
clay_config_path_expand (const gchar  *path,
                         gboolean      recode,
                         GError      **error)
{
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (recode)
    {
      gchar *retval;
      gchar *expanded = clay_config_path_expand_only (path, error);

      if (! expanded)
        return NULL;

      retval = g_filename_from_utf8 (expanded, -1, NULL, NULL, error);

      g_free (expanded);

      return retval;
    }

  return clay_config_path_expand_only (path, error);
}

/**
 * clay_config_path_expand_to_files:
 * @path:  a NUL-terminated string in UTF-8 encoding
 * @error: return location for errors
 *
 * Paths as stored in the `clayrc` have to be treated specially. The
 * string may contain special identifiers such as for example
 * "${clay_dir}" that have to be substituted before use. Also the user's
 * filesystem may be in a different encoding than UTF-8 (which is what
 * is used for the clayrc).
 *
 * This function runs @path through [func@Clay.ConfigPath.expand] and
 * [func@Clay.parse_search_path], then turns the filenames returned by
 * `clay_parse_search_path()` into %GFile using [func@Gio.File.new_for_path].
 *
 * Returns: (element-type GFile) (transfer full): a #GList of newly allocated #GFile objects.
 *
 * Since: 2.10
 **/
GList *
clay_config_path_expand_to_files (const gchar  *path,
                                  GError      **error)
{
  GList *files;
  GList *list;
  gchar *expanded;

  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  expanded = clay_config_path_expand (path, TRUE, error);

  if (! expanded)
    return NULL;

  files = clay_parse_search_path (expanded, 256, FALSE, NULL);

  g_free (expanded);

  for (list = files; list; list = g_list_next (list))
    {
      gchar *dir = list->data;

      list->data = g_file_new_for_path (dir);
      g_free (dir);
    }

  return files;
}

/**
 * clay_config_path_unexpand:
 * @path:   a NUL-terminated string
 * @recode: whether @path is in filesystem encoding or UTF-8
 * @error:  return location for errors
 *
 * The inverse operation of [func@Clay.ConfigPath.expand].
 *
 * This function takes a @path and tries to substitute the first
 * elements by well-known special identifiers such as for example
 * "${clay_dir}". The unexpanded path can then be stored in clayrc and
 * other config files.
 *
 * If @recode is %TRUE then @path is in local filesystem encoding,
 * if @recode is %FALSE then @path is assumed to be UTF-8.
 *
 * Returns: a newly allocated NUL-terminated UTF-8 string
 *
 * Since: 2.10
 **/
gchar *
clay_config_path_unexpand (const gchar  *path,
                           gboolean      recode,
                           GError      **error)
{
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (recode)
    {
      gchar *retval;
      gchar *utf8 = g_filename_to_utf8 (path, -1, NULL, NULL, error);

      if (! utf8)
        return NULL;

      retval = clay_config_path_unexpand_only (utf8);

      g_free (utf8);

      return retval;
    }

  return clay_config_path_unexpand_only (path);
}

/**
 * clay_file_new_for_config_path:
 * @path:   a NUL-terminated string in UTF-8 encoding
 * @error:  return location for errors
 *
 * Expands @path using [func@Clay.ConfigPath.expand] and
 * returns a #GFile for the expanded path.
 *
 * To reverse the expansion, use [func@Clay.file_get_config_path].
 *
 * Returns: (nullable) (transfer full): a newly allocated #GFile,
 *          or %NULL if the expansion failed.
 *
 * Since: 2.10
 **/
GFile *
clay_file_new_for_config_path (const gchar  *path,
                               GError      **error)
{
  GFile *file = NULL;
  gchar *expanded;

  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  expanded = clay_config_path_expand (path, TRUE, error);

  if (expanded)
    {
      file = g_file_new_for_path (expanded);
      g_free (expanded);
    }

  return file;
}

/**
 * clay_file_get_config_path:
 * @file:   a #GFile
 * @error:  return location for errors
 *
 * Unexpands @file's path using [func@Clay.ConfigPath.unexpand] and
 * returns the unexpanded path.
 *
 * The inverse operation of [func@Clay.file_new_for_config_path].
 *
 * Returns: a newly allocated NUL-terminated UTF-8 string, or %NULL if
 *          unexpanding failed.
 *
 * Since: 2.10
 **/
gchar *
clay_file_get_config_path (GFile   *file,
                           GError **error)
{
  gchar *unexpanded = NULL;
  gchar *path;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (path)
    {
      unexpanded = clay_config_path_unexpand (path, TRUE, error);
      g_free (path);
    }
  else
    {
      g_set_error_literal (error, 0, 0,
                           _("File has no path representation"));
    }

  return unexpanded;
}


/*  private functions  */

#define SUBSTS_ALLOC 4

static gchar *
clay_config_path_expand_only (const gchar  *path,
                              GError      **error)
{
  const gchar *home;
  const gchar *p;
  const gchar *s;
  gchar       *n;
  gchar       *token;
  gchar       *filename = NULL;
  gchar       *expanded = NULL;
  gchar      **substs   = NULL;
  guint        n_substs = 0;
  gint         length   = 0;
  guint        i;

  home = g_get_home_dir ();
  if (home)
    home = clay_filename_to_utf8 (home);

  p = path;

  while (*p)
    {
      if (*p == '~' && home)
        {
          length += strlen (home);
          p += 1;
        }
      else if ((token = clay_config_path_extract_token (&p)) != NULL)
        {
          for (i = 0; i < n_substs; i++)
            if (strcmp (substs[2*i], token) == 0)
              break;

          if (i < n_substs)
            {
              s = substs[2*i+1];
            }
          else
            {
              s = NULL;

              if (strcmp (token, "clay_dir") == 0)
                s = clay_directory ();
              else if (strcmp (token, "clay_data_dir") == 0)
                s = clay_data_directory ();
              else if (strcmp (token, "clay_plug_in_dir") == 0 ||
                       strcmp (token, "clay_plugin_dir") == 0)
                s = clay_plug_in_directory ();
              else if (strcmp (token, "clay_sysconf_dir") == 0)
                s = clay_sysconf_directory ();
              else if (strcmp (token, "clay_installation_dir") == 0)
                s = clay_installation_directory ();
              else if (strcmp (token, "clay_cache_dir") == 0)
                s = clay_cache_directory ();
              else if (strcmp (token, "clay_temp_dir") == 0)
                s = clay_temp_directory ();

              if (!s)
                s = g_getenv (token);

#ifdef G_OS_WIN32
              /* The default user clayrc on Windows references
               * ${TEMP}, but not all Windows installations have that
               * environment variable, even if it should be kinda
               * standard. So special-case it.
               */
              if (!s && strcmp (token, "TEMP") == 0)
                s = g_get_tmp_dir ();
#endif  /* G_OS_WIN32 */
            }

          if (!s)
            {
              g_set_error (error, CLAY_CONFIG_ERROR, CLAY_CONFIG_ERROR_PARSE,
                           _("Cannot expand ${%s}"), token);
              g_free (token);
              goto cleanup;
            }

          if (n_substs % SUBSTS_ALLOC == 0)
            substs = g_renew (gchar *, substs, 2 * (n_substs + SUBSTS_ALLOC));

          substs[2*n_substs]     = token;
          substs[2*n_substs + 1] = (gchar *) clay_filename_to_utf8 (s);

          length += strlen (substs[2*n_substs + 1]);

          n_substs++;
        }
      else
        {
          length += g_utf8_skip[(const guchar) *p];
          p = g_utf8_next_char (p);
        }
    }

  if (n_substs == 0)
    return g_strdup (path);

  expanded = g_new (gchar, length + 1);

  p = path;
  n = expanded;

  while (*p)
    {
      if (*p == '~' && home)
        {
          *n = '\0';

          g_strlcat (n, home, length + 1);
          n += strlen (home);
          p += 1;
        }
      else if ((token = clay_config_path_extract_token (&p)) != NULL)
        {
          for (i = 0; i < n_substs; i++)
            {
              if (strcmp (substs[2*i], token) == 0)
                {
                  s = substs[2*i+1];

                  *n = '\0';
                  g_strlcat (n, s, length + 1);
                  n += strlen (s);

                  break;
                }
            }

          g_free (token);
        }
      else
        {
          *n++ = *p++;
        }
    }

  *n = '\0';

 cleanup:
  for (i = 0; i < n_substs; i++)
    g_free (substs[2*i]);

  g_free (substs);
  g_free (filename);

  return expanded;
}

static inline gchar *
clay_config_path_extract_token (const gchar **str)
{
  const gchar *p;
  gchar       *token;

  if (strncmp (*str, "${", 2))
    return NULL;

  p = *str + 2;

  while (*p && (*p != '}'))
    p = g_utf8_next_char (p);

  if (! *p)
    return NULL;

  token = g_strndup (*str + 2, g_utf8_pointer_to_offset (*str + 2, p));

  *str = p + 1; /* after the closing bracket */

  return token;
}

static gchar *
clay_config_path_unexpand_only (const gchar *path)
{
  const struct
  {
    const gchar *id;
    const gchar *prefix;
  }
  identifiers[] =
  {
    { "${clay_plug_in_dir}",      clay_plug_in_directory () },
    { "${clay_data_dir}",         clay_data_directory () },
    { "${clay_sysconf_dir}",      clay_sysconf_directory () },
    { "${clay_installation_dir}", clay_installation_directory () },
    { "${clay_cache_dir}",        clay_cache_directory () },
    { "${clay_temp_dir}",         clay_temp_directory () },
    { "${clay_dir}",              clay_directory () }
  };

  GList *files;
  GList *list;
  gchar *unexpanded;

  files = clay_parse_search_path (path, 256, FALSE, NULL);

  for (list = files; list; list = g_list_next (list))
    {
      gchar *dir = list->data;
      gint   i;

      for (i = 0; i < G_N_ELEMENTS (identifiers); i++)
        {
          if (g_str_has_prefix (dir, identifiers[i].prefix))
            {
              gchar *tmp = g_strconcat (identifiers[i].id,
                                        dir + strlen (identifiers[i].prefix),
                                        NULL);

              g_free (dir);
              list->data = tmp;

              break;
            }
        }
    }

  unexpanded = clay_create_search_path (files);

  clay_free_paths (files);

  return unexpanded;
}
