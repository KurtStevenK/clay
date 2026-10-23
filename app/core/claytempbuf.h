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


ClayTempBuf * clay_temp_buf_new               (gint               width,
                                               gint               height,
                                               const Babl        *format) G_GNUC_WARN_UNUSED_RESULT;
ClayTempBuf * clay_temp_buf_new_from_pixbuf   (GdkPixbuf         *pixbuf,
                                               const Babl        *f_or_null) G_GNUC_WARN_UNUSED_RESULT;
ClayTempBuf * clay_temp_buf_copy              (const ClayTempBuf *src) G_GNUC_WARN_UNUSED_RESULT;

ClayTempBuf * clay_temp_buf_ref               (const ClayTempBuf *buf);
void          clay_temp_buf_unref             (const ClayTempBuf *buf);

ClayTempBuf * clay_temp_buf_scale             (const ClayTempBuf *buf,
                                               gint               width,
                                               gint               height) G_GNUC_WARN_UNUSED_RESULT;

gint          clay_temp_buf_get_width         (const ClayTempBuf *buf);
gint          clay_temp_buf_get_height        (const ClayTempBuf *buf);

const Babl  * clay_temp_buf_get_format        (const ClayTempBuf *buf);
void          clay_temp_buf_set_format        (ClayTempBuf       *buf,
                                               const Babl        *format);

guchar      * clay_temp_buf_get_data          (const ClayTempBuf *buf);
gsize         clay_temp_buf_get_data_size     (const ClayTempBuf *buf);

guchar      * clay_temp_buf_data_clear        (ClayTempBuf       *buf);

gpointer      clay_temp_buf_lock              (const ClayTempBuf *buf,
                                               const Babl        *format,
                                               GeglAccessMode     access_mode) G_GNUC_WARN_UNUSED_RESULT;
void          clay_temp_buf_unlock            (const ClayTempBuf *buf,
                                               gconstpointer      data);

gsize         clay_temp_buf_get_memsize       (const ClayTempBuf *buf);

GeglBuffer  * clay_temp_buf_create_buffer     (const ClayTempBuf *temp_buf) G_GNUC_WARN_UNUSED_RESULT;
GdkPixbuf   * clay_temp_buf_create_pixbuf     (const ClayTempBuf *temp_buf) G_GNUC_WARN_UNUSED_RESULT;

ClayTempBuf * clay_gegl_buffer_get_temp_buf   (GeglBuffer        *buffer);


/*  stats  */

guint64       clay_temp_buf_get_total_memsize (void);
