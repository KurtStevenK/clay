/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayenv.h
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_ENV_H__
#define __CLAY_ENV_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifdef G_OS_WIN32
#  ifdef __CLAY_ENV_C__
#    define CLAYVAR extern __declspec(dllexport)
#  else  /* !__CLAY_ENV_C__ */
#    define CLAYVAR extern __declspec(dllimport)
#  endif /* !__CLAY_ENV_C__ */
#else  /* !G_OS_WIN32 */
#  define CLAYVAR extern
#endif

CLAYVAR const guint clay_major_version;
CLAYVAR const guint clay_minor_version;
CLAYVAR const guint clay_micro_version;


const gchar * clay_directory                   (void) G_GNUC_CONST;
const gchar * clay_installation_directory      (void) G_GNUC_CONST;
const gchar * clay_data_directory              (void) G_GNUC_CONST;
const gchar * clay_locale_directory            (void) G_GNUC_CONST;
const gchar * clay_sysconf_directory           (void) G_GNUC_CONST;
const gchar * clay_plug_in_directory           (void) G_GNUC_CONST;
const gchar * clay_cache_directory             (void) G_GNUC_CONST;
const gchar * clay_temp_directory              (void) G_GNUC_CONST;

GFile       * clay_directory_file              (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * clay_installation_directory_file (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * clay_data_directory_file         (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * clay_locale_directory_file       (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * clay_sysconf_directory_file      (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;
GFile       * clay_plug_in_directory_file      (const gchar *first_element,
                                                ...) G_GNUC_MALLOC;

GList       * clay_parse_search_path           (const gchar  *path,
                                                gint          max_paths,
                                                gboolean      check,
                                                GList       **check_failed);
gchar       * clay_create_search_path          (GList        *paths) G_GNUC_MALLOC;
void          clay_free_paths                  (GList        *paths);
gchar       * clay_get_user_writable_dir       (GList        *paths) G_GNUC_MALLOC;


/* Deprecated Functions */

CLAY_DEPRECATED_FOR(clay_parse_search_path)
GList       * clay_path_parse                  (const gchar  *path,
                                                gint          max_paths,
                                                gboolean      check,
                                                GList       **check_failed);
CLAY_DEPRECATED_FOR(clay_create_search_path)
gchar       * clay_path_to_str                 (GList        *path) G_GNUC_MALLOC;
CLAY_DEPRECATED_FOR(clay_free_paths)
void          clay_path_free                   (GList        *path);
CLAY_DEPRECATED_FOR(clay_get_user_writable_dir)
gchar       * clay_path_get_user_writable_dir  (GList        *path) G_GNUC_MALLOC;


G_END_DECLS

#endif  /*  __CLAY_ENV_H__  */
