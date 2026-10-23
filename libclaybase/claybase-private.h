/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claybase-private.h
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_BASE_PRIVATE_H__
#define __CLAY_BASE_PRIVATE_H__


#include <libclaybase/claycpuaccel-private.h>
#include <libclaybase/clayenv-private.h>

#ifndef G_OS_WIN32
#include <libclaybase/claysignal-private.h>
#endif


typedef struct _ClayUnitVtable ClayUnitVtable;

struct _ClayUnitVtable
{
  /* These methods MUST only be set on libclay, not in core app. */
  gboolean      (* get_deletion_flag)                 (ClayUnit  *unit);
  gboolean      (* set_deletion_flag)                 (ClayUnit  *unit,
                                                       gboolean   deletion_flag);
  gchar       * (* get_data)                          (gint       unit_id,
                                                       gdouble   *factor,
                                                       gint      *digits,
                                                       gchar    **symbol,
                                                       gchar    **abbreviation);

  /* These methods MUST only be set on app, not in libclay. */
  ClayUnit    * (* get_user_unit)                     (gint       unit_id);


  /* Reserved methods. */
  void          (* _reserved_0)                       (void);
  void          (* _reserved_1)                       (void);
  void          (* _reserved_2)                       (void);
  void          (* _reserved_3)                       (void);
  void          (* _reserved_4)                       (void);
  void          (* _reserved_5)                       (void);
  void          (* _reserved_6)                       (void);
  void          (* _reserved_7)                       (void);
  void          (* _reserved_8)                       (void);
  void          (* _reserved_9)                       (void);
};


extern ClayUnitVtable  _clay_unit_vtable;
extern GHashTable     *_clay_units;


G_BEGIN_DECLS

void   clay_base_init              (ClayUnitVtable *vtable);
void   clay_base_exit              (void);
void   clay_base_compat_enums_init (void);

G_END_DECLS

#endif /* __CLAY_BASE_PRIVATE_H__ */
