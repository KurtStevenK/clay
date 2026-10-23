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

#pragma once


#ifdef CLAY_RELEASE
#define CLAY_TIMER_START()
#define CLAY_TIMER_END(message)
#else
#define CLAY_TIMER_START() \
  { GTimer *_timer = g_timer_new ();

#define CLAY_TIMER_END(message) \
  g_debug ("%s: %s took %0.4f seconds\n", \
           G_STRFUNC, message, g_timer_elapsed (_timer, NULL)); \
  g_timer_destroy (_timer); }
#endif


#define MIN4(a,b,c,d) MIN (MIN ((a), (b)), MIN ((c), (d)))
#define MAX4(a,b,c,d) MAX (MAX ((a), (b)), MAX ((c), (d)))


/* TODO: when bumping GLib >= 2.80, use GTYPE_TO_POINTER and
 * GPOINTER_TO_TYPE directly.
 */
#define CLAYTYPE_TO_POINTER(t) ((gpointer) (guintptr) (t))
#define CLAYPOINTER_TO_TYPE(p) ((GType) (guintptr) (p))


gint          clay_get_pid                          (void);
guint64       clay_get_physical_memory_size         (void);
guint64       clay_get_available_memory_size        (void);
gchar       * clay_get_default_language             (const gchar       *category);
ClayUnit    * clay_get_default_unit                 (void);

gchar      ** clay_properties_append                (GType              object_type,
                                                     gint              *n_properties,
                                                     gchar            **names,
                                                     GValue           **values,
                                                     ...) G_GNUC_NULL_TERMINATED;
gchar      ** clay_properties_append_valist         (GType              object_type,
                                                     gint              *n_properties,
                                                     gchar            **names,
                                                     GValue           **values,
                                                     va_list            args);
void          clay_properties_free                  (gint               n_properties,
                                                     gchar            **names,
                                                     GValue            *values);

gchar       * clay_markup_extract_text              (const gchar       *markup);

const gchar * clay_enum_get_value_name              (GType              enum_type,
                                                     gint               value);

gboolean      clay_get_fill_params                  (ClayContext       *context,
                                                     ClayFillType       fill_type,
                                                     GeglColor        **color,
                                                     ClayPattern      **pattern,
                                                     GError           **error);

/* Common values for the n_snap_lines parameter of
 * clay_constrain_line.
 */
#define CLAY_CONSTRAIN_LINE_90_DEGREES 2
#define CLAY_CONSTRAIN_LINE_45_DEGREES 4
#define CLAY_CONSTRAIN_LINE_15_DEGREES 12

void          clay_constrain_line                   (gdouble            start_x,
                                                     gdouble            start_y,
                                                     gdouble           *end_x,
                                                     gdouble           *end_y,
                                                     gint               n_snap_lines,
                                                     gdouble            offset_angle,
                                                     gdouble            xres,
                                                     gdouble            yres);

gint          clay_file_compare                     (GFile             *file1,
                                                     GFile             *file2);
gboolean      clay_file_is_executable               (GFile             *file);
gchar       * clay_file_get_extension               (GFile             *file);
GFile       * clay_file_with_new_extension          (GFile             *file,
                                                     GFile             *ext_file);
gboolean      clay_file_delete_recursive            (GFile             *file,
                                                     GError           **error);
gboolean      clay_file_is_ancestor                 (GFile             *ancestor,
                                                     GFile             *descendant);

gchar       * clay_data_input_stream_read_line_always
                                                    (GDataInputStream  *stream,
                                                     gsize             *length,
                                                     GCancellable      *cancellable,
                                                     GError           **error);

gboolean      clay_data_input_stream_read_char      (GDataInputStream  *input,
                                                     gchar             *value,
                                                     GError           **error);
gboolean      clay_data_input_stream_read_short     (GDataInputStream  *input,
                                                     gint16            *value,
                                                     GError           **error);
gboolean      clay_data_input_stream_read_long      (GDataInputStream  *input,
                                                     gint32            *value,
                                                     GError           **error);
gboolean      clay_data_input_stream_read_ucs2_text (GDataInputStream  *input,
                                                     gchar            **value,
                                                     GError           **error);
gboolean      clay_data_input_stream_rle_decode     (GDataInputStream  *input,
                                                     gchar             *buffer,
                                                     gsize              buffer_size,
                                                     gint32             height,
                                                     GError           **error);

gboolean      clay_ascii_strtoi                     (const gchar       *nptr,
                                                     gchar            **endptr,
                                                     gint               base,
                                                     gint              *result);
gboolean      clay_ascii_strtod                     (const gchar       *nptr,
                                                     gchar            **endptr,
                                                     gdouble           *result);

gchar       * clay_appstream_to_pango_markup        (const gchar       *as_text);
void          clay_appstream_to_pango_markups       (const gchar       *as_text,
                                                     gchar            **introduction,
                                                     GList            **release_items);

gint          clay_g_list_compare                   (GList             *list1,
                                                     GList             *list2);

ClayTRCType   clay_suggest_trc_for_component_type   (ClayComponentType  component_type,
                                                     ClayTRCType        old_trc);

ClayAsync   * clay_idle_run_async                   (ClayRunAsyncFunc   func,
                                                     gpointer           user_data);
ClayAsync   * clay_idle_run_async_full              (gint               priority,
                                                     ClayRunAsyncFunc   func,
                                                     gpointer           user_data,
                                                     GDestroyNotify     user_data_destroy_func);

ClayImage   * clay_create_image_from_buffer         (Clay                *clay,
                                                     GeglBuffer          *buffer,
                                                     const gchar         *image_name);

gint          clay_view_size_get_larger             (gint                 view_size);
gint          clay_view_size_get_smaller            (gint                 view_size);

#ifdef G_OS_WIN32
gboolean      clay_win32_have_wintab                (void);
gboolean      clay_win32_have_windows_ink           (void);
#endif

gint          clay_version_cmp                      (const gchar         *v1,
                                                     const gchar         *v2);

GList       * clay_get_type_children                (GType                type,
                                                     GList               *types,
                                                     GList               *excluded);

const gchar * clay_get_enum_value_nick              (GType                enum_type,
                                                     gint                 value);
