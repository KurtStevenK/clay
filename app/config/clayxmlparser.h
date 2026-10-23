/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ClayXmlParser
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#pragma once


ClayXmlParser * clay_xml_parser_new              (const GMarkupParser *markup_parser,
                                                  gpointer             user_data);
gboolean        clay_xml_parser_parse_file       (ClayXmlParser       *parser,
                                                  const gchar         *filename,
                                                  GError             **error);
gboolean        clay_xml_parser_parse_gfile      (ClayXmlParser       *parser,
                                                  GFile               *file,
                                                  GError             **error);
gboolean        clay_xml_parser_parse_fd         (ClayXmlParser       *parser,
                                                  gint                 fd,
                                                  GError             **error);
gboolean        clay_xml_parser_parse_io_channel (ClayXmlParser       *parser,
                                                  GIOChannel          *io,
                                                  GError             **error);
gboolean        clay_xml_parser_parse_buffer     (ClayXmlParser       *parser,
                                                  const gchar         *buffer,
                                                  gssize               len,
                                                  GError             **error);
void            clay_xml_parser_free             (ClayXmlParser       *parser);
