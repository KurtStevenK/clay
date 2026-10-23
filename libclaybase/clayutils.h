/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_UTILS_H__
#define __CLAY_UTILS_H__

G_BEGIN_DECLS


gchar               * clay_utf8_strtrim              (const gchar       *str,
                                                      gint               max_chars) G_GNUC_MALLOC;
gchar               * clay_any_to_utf8               (const gchar       *str,
                                                      gssize             len,
                                                      const gchar       *warning_format,
                                                      ...) G_GNUC_PRINTF (3, 4) G_GNUC_MALLOC;
const gchar         * clay_filename_to_utf8          (const gchar       *filename);

const gchar         * clay_file_get_utf8_name        (GFile             *file);
gboolean              clay_file_has_extension        (GFile             *file,
                                                      const gchar       *extension);
gboolean              clay_file_show_in_file_manager (GFile             *file,
                                                      GError           **error);

gchar               * clay_strip_uline               (const gchar       *str) G_GNUC_MALLOC;
gchar               * clay_escape_uline              (const gchar       *str) G_GNUC_MALLOC;

gboolean              clay_is_canonical_identifier   (const gchar       *identifier);
gchar               * clay_canonicalize_identifier   (const gchar       *identifier) G_GNUC_MALLOC;

const ClayEnumDesc  * clay_enum_get_desc             (GEnumClass        *enum_class,
                                                      gint               value);
gboolean              clay_enum_get_value            (GType              enum_type,
                                                      gint               value,
                                                      const gchar      **value_name,
                                                      const gchar      **value_nick,
                                                      const gchar      **value_desc,
                                                      const gchar      **value_help);
const gchar         * clay_enum_value_get_desc       (GEnumClass        *enum_class,
                                                      const GEnumValue  *enum_value);
const gchar         * clay_enum_value_get_help       (GEnumClass        *enum_class,
                                                      const GEnumValue  *enum_value);
const gchar         * clay_enum_value_get_abbrev     (GEnumClass        *enum_class,
                                                      const GEnumValue  *enum_value);

const ClayFlagsDesc * clay_flags_get_first_desc      (GFlagsClass       *flags_class,
                                                      guint              value);
gboolean              clay_flags_get_first_value     (GType              flags_type,
                                                      guint              value,
                                                      const gchar      **value_name,
                                                      const gchar      **value_nick,
                                                      const gchar      **value_desc,
                                                      const gchar      **value_help);
const gchar         * clay_flags_value_get_desc      (GFlagsClass       *flags_class,
                                                      const GFlagsValue *flags_value);
const gchar         * clay_flags_value_get_help      (GFlagsClass       *flags_class,
                                                      const GFlagsValue *flags_value);
const gchar         * clay_flags_value_get_abbrev    (GFlagsClass       *flags_class,
                                                      const GFlagsValue *flags_value);

gboolean              clay_stack_trace_available     (gboolean           optimal);
gboolean              clay_stack_trace_print         (const gchar       *prog_name,
                                                      gpointer           stream,
                                                      gchar            **trace);
void                  clay_stack_trace_query         (const gchar       *prog_name);

void                  clay_range_estimate_settings   (gdouble            lower,
                                                      gdouble            upper,
                                                      gdouble           *step,
                                                      gdouble           *page,
                                                      gint              *digits);

void                  clay_bind_text_domain          (const gchar       *domain_name,
                                                      const gchar       *dir_name);

G_END_DECLS

#endif  /* __CLAY_UTILS_H__ */
