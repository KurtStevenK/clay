/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claybase-private.c
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

#include "config.h"

#include <gio/gio.h>

#include "claybasetypes.h"

#include "claybase-private.h"
#include "claycompatenums.h"


GHashTable     *_clay_units       = NULL;
ClayUnitVtable  _clay_unit_vtable = { NULL, };


void
clay_base_init (ClayUnitVtable *vtable)
{
  static gboolean clay_base_initialized = FALSE;

  g_return_if_fail (vtable != NULL);

  if (clay_base_initialized)
    g_error ("clay_base_init() must only be called once!");

  _clay_unit_vtable = *vtable;

  clay_base_compat_enums_init ();

  clay_base_initialized = TRUE;
}

void
clay_base_exit (void)
{
  g_clear_pointer (&_clay_units, g_hash_table_unref);
}

void
clay_base_compat_enums_init (void)
{
#if 0
  static gboolean clay_base_compat_initialized = FALSE;
  GQuark          quark;

  if (clay_base_compat_initialized)
    return;

  quark = g_quark_from_static_string ("clay-compat-enum");

  /*  This is how a compat enum is registered, leave one here for
   *  documentation purposes, remove it as soon as we get a real
   *  compat enum again
   */
  g_type_set_qdata (CLAY_TYPE_ADD_MASK_TYPE, quark,
                    (gpointer) CLAY_TYPE_ADD_MASK_TYPE_COMPAT);

  clay_base_compat_initialized = TRUE;
#endif
}
