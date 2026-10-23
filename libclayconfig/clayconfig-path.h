/* LIBCLAY - The Clay Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayconfig-path.h
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#if !defined (__CLAY_CONFIG_H_INSIDE__) && !defined (CLAY_CONFIG_COMPILATION)
#error "Only <libclayconfig/clayconfig.h> can be included directly."
#endif

#ifndef __CLAY_CONFIG_PATH_H__
#define __CLAY_CONFIG_PATH_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * CLAY_TYPE_CONFIG_PATH
 */

#define CLAY_TYPE_CONFIG_PATH               (clay_config_path_get_type ())
#define CLAY_VALUE_HOLDS_CONFIG_PATH(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_CONFIG_PATH))

GType               clay_config_path_get_type        (void);



/*
 * CLAY_TYPE_PARAM_CONFIG_PATH
 */

/**
 * ClayConfigPathType:
 * @CLAY_CONFIG_PATH_FILE:      A single file
 * @CLAY_CONFIG_PATH_FILE_LIST: A list of files
 * @CLAY_CONFIG_PATH_DIR:       A single folder
 * @CLAY_CONFIG_PATH_DIR_LIST:  A list of folders
 *
 * Types of config paths.
 **/
typedef enum
{
  CLAY_CONFIG_PATH_FILE,
  CLAY_CONFIG_PATH_FILE_LIST,
  CLAY_CONFIG_PATH_DIR,
  CLAY_CONFIG_PATH_DIR_LIST
} ClayConfigPathType;


#define CLAY_TYPE_PARAM_CONFIG_PATH            (clay_param_config_path_get_type ())
#define CLAY_IS_PARAM_SPEC_CONFIG_PATH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_CONFIG_PATH))

GType               clay_param_config_path_get_type  (void);

GParamSpec        * clay_param_spec_config_path      (const gchar  *name,
                                                      const gchar  *nick,
                                                      const gchar  *blurb,
                                                      ClayConfigPathType  type,
                                                      const gchar  *default_value,
                                                      GParamFlags   flags);

ClayConfigPathType  clay_param_spec_config_path_type (GParamSpec   *pspec);


/*
 * ClayConfigPath utilities
 */

gchar             * clay_config_path_expand          (const gchar  *path,
                                                      gboolean      recode,
                                                      GError      **error) G_GNUC_MALLOC;
GList             * clay_config_path_expand_to_files (const gchar  *path,
                                                      GError      **error) G_GNUC_MALLOC;

gchar             * clay_config_path_unexpand        (const gchar  *path,
                                                      gboolean      recode,
                                                      GError      **error) G_GNUC_MALLOC;

GFile             * clay_file_new_for_config_path    (const gchar  *path,
                                                      GError      **error) G_GNUC_MALLOC;
gchar             * clay_file_get_config_path        (GFile        *file,
                                                      GError      **error) G_GNUC_MALLOC;

gchar             * clay_config_build_data_path      (const gchar  *name) G_GNUC_MALLOC;
gchar             * clay_config_build_writable_path  (const gchar  *name) G_GNUC_MALLOC;
gchar             * clay_config_build_plug_in_path   (const gchar  *name) G_GNUC_MALLOC;
gchar             * clay_config_build_system_path    (const gchar  *name) G_GNUC_MALLOC;


G_END_DECLS

#endif /* __CLAY_CONFIG_PATH_H__ */
