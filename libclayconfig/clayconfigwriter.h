/* LIBCLAY - The Clay Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayConfigWriter
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_CONFIG_WRITER_H__
#define __CLAY_CONFIG_WRITER_H__


/**
 * CLAY_TYPE_CONFIG_WRITER:
 *
 * The type ID of the "ClayConfigWriter" type which is a boxed type,
 * used to write config files.
 *
 * Since: 3.0
 */
#define CLAY_TYPE_CONFIG_WRITER (clay_config_writer_get_type ())


GType              clay_config_writer_get_type     (void);

ClayConfigWriter * clay_config_writer_new_from_file     (GFile             *file,
                                                         gboolean           atomic,
                                                         const gchar       *header,
                                                         GError           **error);
ClayConfigWriter * clay_config_writer_new_from_stream   (GOutputStream     *output,
                                                         const gchar       *header,
                                                         GError           **error);
ClayConfigWriter * clay_config_writer_new_from_fd       (gint               fd);
ClayConfigWriter * clay_config_writer_new_from_string   (GString           *string);

ClayConfigWriter * clay_config_writer_ref          (ClayConfigWriter  *writer);
void               clay_config_writer_unref        (ClayConfigWriter  *writer);

void               clay_config_writer_open         (ClayConfigWriter  *writer,
                                                    const gchar       *name);
void               clay_config_writer_comment_mode (ClayConfigWriter  *writer,
                                                    gboolean           enable);

void               clay_config_writer_print        (ClayConfigWriter  *writer,
                                                    const gchar       *string,
                                                    gint               len);
void               clay_config_writer_printf       (ClayConfigWriter  *writer,
                                                    const gchar       *format,
                                                    ...) G_GNUC_PRINTF (2, 3);
void               clay_config_writer_identifier   (ClayConfigWriter  *writer,
                                                    const gchar       *identifier);
void               clay_config_writer_string       (ClayConfigWriter  *writer,
                                                    const gchar       *string);
void               clay_config_writer_data         (ClayConfigWriter  *writer,
                                                    gint               length,
                                                    const guint8      *data);
void               clay_config_writer_comment      (ClayConfigWriter  *writer,
                                                    const gchar       *comment);
void               clay_config_writer_linefeed     (ClayConfigWriter  *writer);


void               clay_config_writer_revert       (ClayConfigWriter  *writer);
void               clay_config_writer_close        (ClayConfigWriter  *writer);
gboolean           clay_config_writer_finish       (ClayConfigWriter  *writer,
                                                    const gchar       *footer,
                                                    GError           **error);


#endif /* __CLAY_CONFIG_WRITER_H__ */
