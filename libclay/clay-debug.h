/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clay-debug.h
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

#ifndef __CLAY_DEBUG_H__
#define __CLAY_DEBUG_H__

G_BEGIN_DECLS


typedef enum
{
  CLAY_DEBUG_PID            = 1 << 0,
  CLAY_DEBUG_FATAL_WARNINGS = 1 << 1,
  CLAY_DEBUG_QUERY          = 1 << 2,
  CLAY_DEBUG_INIT           = 1 << 3,
  CLAY_DEBUG_RUN            = 1 << 4,
  CLAY_DEBUG_QUIT           = 1 << 5,
  CLAY_DEBUG_FATAL_CRITICALS = 1 << 6,

} ClayDebugFlag;


void   _clay_debug_init  (const gchar *basename);
void   _clay_debug_configure (ClayStackTraceMode stack_trace_mode);
guint  _clay_get_debug_flags (void);
void   _clay_debug_stop  (void);


G_END_DECLS

#endif /* __CLAY_DEBUG_H__ */
