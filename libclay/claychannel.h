/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claychannel.h
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

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_CHANNEL_H__
#define __CLAY_CHANNEL_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#include <libclay/claydrawable.h>


#define CLAY_TYPE_CHANNEL (clay_channel_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayChannel, clay_channel, CLAY, CHANNEL, ClayDrawable)


struct _ClayChannelClass
{
  ClayDrawableClass parent_class;

  /* Padding for future expansion */
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};


ClayChannel * clay_channel_get_by_id (gint32       channel_id);


G_END_DECLS

#endif /* __CLAY_CHANNEL_H__ */
