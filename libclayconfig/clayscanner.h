/* LIBCLAY - The Clay Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayscanner.h
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_SCANNER_H__
#define __CLAY_SCANNER_H__


/**
 * CLAY_TYPE_SCANNER:
 *
 * The type ID of the Clay scanner type which is a boxed type, used to
 * read config files.
 *
 * Since: 3.0
 */
#define CLAY_TYPE_SCANNER (clay_scnner_get_type ())


GType         clay_scanner_get_type                 (void);

ClayScanner * clay_scanner_new_file                 (GFile         *file,
                                                     GError       **error);
ClayScanner * clay_scanner_new_stream               (GInputStream  *input,
                                                     GError       **error);
ClayScanner * clay_scanner_new_string               (const gchar   *text,
                                                     gint           text_len,
                                                     GError       **error);

ClayScanner * clay_scanner_ref                      (ClayScanner   *scanner);
void          clay_scanner_unref                    (ClayScanner   *scanner);

gboolean      clay_scanner_parse_token              (ClayScanner   *scanner,
                                                     GTokenType     token);
gboolean      clay_scanner_parse_identifier         (ClayScanner   *scanner,
                                                     const gchar   *identifier);
gboolean      clay_scanner_parse_string             (ClayScanner   *scanner,
                                                     gchar        **dest);
gboolean      clay_scanner_parse_string_no_validate (ClayScanner   *scanner,
                                                     gchar        **dest);
gboolean      clay_scanner_parse_data               (ClayScanner   *scanner,
                                                     gint           length,
                                                     guint8       **dest);
gboolean      clay_scanner_parse_int                (ClayScanner   *scanner,
                                                     gint          *dest);
gboolean      clay_scanner_parse_int64              (ClayScanner   *scanner,
                                                     gint64        *dest);
gboolean      clay_scanner_parse_double             (ClayScanner   *scanner,
                                                     gdouble       *dest);
gboolean      clay_scanner_parse_boolean            (ClayScanner   *scanner,
                                                     gboolean      *dest);
gboolean      clay_scanner_parse_color              (ClayScanner   *scanner,
                                                     GeglColor    **color);
gboolean      clay_scanner_parse_matrix2            (ClayScanner   *scanner,
                                                     ClayMatrix2   *dest);


#endif /* __CLAY_SCANNER_H__ */
