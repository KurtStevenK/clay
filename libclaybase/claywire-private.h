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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_WIRE_H__
#define __CLAY_WIRE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef struct _ClayWireMessage  ClayWireMessage;

typedef void     (* ClayWireReadFunc)    (GIOChannel      *channel,
                                          ClayWireMessage *msg,
                                          gpointer         user_data);
typedef void     (* ClayWireWriteFunc)   (GIOChannel      *channel,
                                          ClayWireMessage *msg,
                                          gpointer         user_data);
typedef void     (* ClayWireDestroyFunc) (ClayWireMessage *msg);
typedef gboolean (* ClayWireIOFunc)      (GIOChannel      *channel,
                                          const guint8    *buf,
                                          gulong           count,
                                          gpointer         user_data);
typedef gboolean (* ClayWireFlushFunc)   (GIOChannel      *channel,
                                          gpointer         user_data);


struct _ClayWireMessage
{
  guint32  type;
  gpointer data;
};


void      clay_wire_register      (guint32              type,
                                   ClayWireReadFunc     read_func,
                                   ClayWireWriteFunc    write_func,
                                   ClayWireDestroyFunc  destroy_func);

void      clay_wire_set_reader    (ClayWireIOFunc       read_func);
void      clay_wire_set_writer    (ClayWireIOFunc       write_func);
void      clay_wire_set_flusher   (ClayWireFlushFunc    flush_func);

gboolean  clay_wire_read          (GIOChannel          *channel,
                                   guint8              *buf,
                                   gsize                count,
                                   gpointer             user_data);
gboolean  clay_wire_write         (GIOChannel          *channel,
                                   const guint8        *buf,
                                   gsize                count,
                                   gpointer             user_data);
gboolean  clay_wire_flush         (GIOChannel          *channel,
                                   gpointer             user_data);

gboolean  clay_wire_error         (void);
void      clay_wire_clear_error   (void);

gboolean  clay_wire_read_msg      (GIOChannel          *channel,
                                   ClayWireMessage     *msg,
                                   gpointer             user_data);
gboolean  clay_wire_write_msg     (GIOChannel          *channel,
                                   ClayWireMessage     *msg,
                                   gpointer             user_data);

void      clay_wire_destroy       (ClayWireMessage     *msg);


/*  for internal use in libclaybase  */

G_GNUC_INTERNAL gboolean  _clay_wire_read_int64   (GIOChannel     *channel,
                                                   guint64        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_read_int32   (GIOChannel     *channel,
                                                   guint32        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_read_int16   (GIOChannel     *channel,
                                                   guint16        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_read_int8    (GIOChannel     *channel,
                                                   guint8         *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_read_double  (GIOChannel     *channel,
                                                   gdouble        *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_read_string  (GIOChannel     *channel,
                                                   gchar         **data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean
                     _clay_wire_read_gegl_color   (GIOChannel     *channel,
                                                   GBytes        **pixel_data,
                                                   GBytes        **icc_data,
                                                   gchar         **encoding,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_write_int64  (GIOChannel     *channel,
                                                   const guint64  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_write_int32  (GIOChannel     *channel,
                                                   const guint32  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_write_int16  (GIOChannel     *channel,
                                                   const guint16  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_write_int8   (GIOChannel     *channel,
                                                   const guint8   *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_write_double (GIOChannel     *channel,
                                                   const gdouble  *data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean  _clay_wire_write_string (GIOChannel     *channel,
                                                   gchar         **data,
                                                   gint            count,
                                                   gpointer        user_data);
G_GNUC_INTERNAL gboolean
                     _clay_wire_write_gegl_color  (GIOChannel     *channel,
                                                   GBytes        **pixel_data,
                                                   GBytes        **icc_data,
                                                   gchar         **encoding,
                                                   gint            count,
                                                   gpointer        user_data);


G_END_DECLS

#endif /* __CLAY_WIRE_H__ */
