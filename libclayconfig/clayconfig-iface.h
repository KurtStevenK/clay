/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_CONFIG_IFACE_H__
#define __CLAY_CONFIG_IFACE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#define CLAY_TYPE_CONFIG (clay_config_get_type ())
G_DECLARE_INTERFACE (ClayConfig, clay_config, CLAY, CONFIG, GObject)

struct _ClayConfigInterface
{
  GTypeInterface base_iface;

  gboolean     (* serialize)            (ClayConfig       *config,
                                         ClayConfigWriter *writer,
                                         gpointer          data);
  gboolean     (* deserialize)          (ClayConfig       *config,
                                         GScanner         *scanner,
                                         gint              nest_level,
                                         gpointer          data);
  ClayConfig * (* deserialize_create)   (GType             type,
                                         GScanner         *scanner,
                                         gint              nest_level,
                                         gpointer          data);
  gboolean     (* serialize_property)   (ClayConfig       *config,
                                         guint             property_id,
                                         const GValue     *value,
                                         GParamSpec       *pspec,
                                         ClayConfigWriter *writer);
  gboolean     (* deserialize_property) (ClayConfig       *config,
                                         guint             property_id,
                                         GValue           *value,
                                         GParamSpec       *pspec,
                                         GScanner         *scanner,
                                         GTokenType       *expected);
  ClayConfig * (* duplicate)            (ClayConfig       *config);
  gboolean     (* equal)                (ClayConfig       *a,
                                         ClayConfig       *b);
  void         (* reset)                (ClayConfig       *config);
  gboolean     (* copy)                 (ClayConfig       *src,
                                         ClayConfig       *dest,
                                         GParamFlags       flags);
};


gboolean   clay_config_serialize_to_file     (ClayConfig          *config,
                                              GFile               *file,
                                              const gchar         *header,
                                              const gchar         *footer,
                                              gpointer             data,
                                              GError             **error);
gboolean   clay_config_serialize_to_stream   (ClayConfig          *config,
                                              GOutputStream       *output,
                                              const gchar         *header,
                                              const gchar         *footer,
                                              gpointer             data,
                                              GError             **error);
gboolean   clay_config_serialize_to_fd       (ClayConfig          *config,
                                              gint                 fd,
                                              gpointer             data);
gchar    * clay_config_serialize_to_string   (ClayConfig          *config,
                                              gpointer             data);
ClayParasite *
           clay_config_serialize_to_parasite (ClayConfig          *config,
                                              const gchar         *parasite_name,
                                              guint                parasite_flags,
                                              gpointer             data);

gboolean   clay_config_deserialize_file      (ClayConfig          *config,
                                              GFile               *file,
                                              gpointer             data,
                                              GError             **error);
gboolean   clay_config_deserialize_stream    (ClayConfig          *config,
                                              GInputStream        *input,
                                              gpointer             data,
                                              GError             **error);
gboolean   clay_config_deserialize_string    (ClayConfig          *config,
                                              const gchar         *text,
                                              gint                 text_len,
                                              gpointer             data,
                                              GError             **error);
gboolean   clay_config_deserialize_parasite  (ClayConfig          *config,
                                              const ClayParasite  *parasite,
                                              gpointer             data,
                                              GError             **error);

gboolean   clay_config_deserialize_return    (GScanner            *scanner,
                                              GTokenType           expected_token,
                                              gint                 nest_level);

gboolean   clay_config_serialize             (ClayConfig          *config,
                                              ClayConfigWriter    *writer,
                                              gpointer             data);
gboolean   clay_config_deserialize           (ClayConfig          *config,
                                              GScanner            *scanner,
                                              gint                 nest_level,
                                              gpointer             data);
gpointer   clay_config_duplicate             (ClayConfig          *config);
gboolean   clay_config_is_equal_to           (ClayConfig          *a,
                                              ClayConfig          *b);
void       clay_config_reset                 (ClayConfig          *config);
gboolean   clay_config_copy                  (ClayConfig          *src,
                                              ClayConfig          *dest,
                                              GParamFlags          flags);

guint      clay_config_get_xcf_version       (ClayConfig          *config);
void       clay_config_set_xcf_version       (ClayConfig          *config,
                                              guint                xcf_version);


G_END_DECLS

#endif  /* __CLAY_CONFIG_IFACE_H__ */
