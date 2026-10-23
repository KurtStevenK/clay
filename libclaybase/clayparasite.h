/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayparasite.h
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_PARASITE_H__
#define __CLAY_PARASITE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * CLAY_TYPE_PARASITE
 */

#define CLAY_TYPE_PARASITE               (clay_parasite_get_type ())
#define CLAY_VALUE_HOLDS_PARASITE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_PARASITE))

GType   clay_parasite_get_type           (void);


/*
 * CLAY_TYPE_PARAM_PARASITE
 */

#define CLAY_TYPE_PARAM_PARASITE           (clay_param_parasite_get_type ())
#define CLAY_IS_PARAM_SPEC_PARASITE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_PARASITE))

GType        clay_param_parasite_get_type  (void);

GParamSpec * clay_param_spec_parasite      (const gchar  *name,
                                            const gchar  *nick,
                                            const gchar  *blurb,
                                            GParamFlags   flags);

/**
 * CLAY_PARASITE_PERSISTENT:
 *
 * A persistent parasite will be saved to XCF and can be used again after
 * reloading. A non persistent parasite will only be available during the
 * current session.
 * See [struct@Clay.Parasite].
 **/
#define CLAY_PARASITE_PERSISTENT 1

/**
 * CLAY_PARASITE_UNDOABLE:
 *
 * An undoable parasite that was added, can be removed using the Undo action.
 * If this flag is not set, undoing will not change the parasite.
 * See [struct@Clay.Parasite].
 **/
#define CLAY_PARASITE_UNDOABLE   2

#define CLAY_PARASITE_ATTACH_PARENT     (0x80 << 8)
#define CLAY_PARASITE_PARENT_PERSISTENT (CLAY_PARASITE_PERSISTENT << 8)
#define CLAY_PARASITE_PARENT_UNDOABLE   (CLAY_PARASITE_UNDOABLE << 8)

#define CLAY_PARASITE_ATTACH_GRANDPARENT     (0x80 << 16)
#define CLAY_PARASITE_GRANDPARENT_PERSISTENT (CLAY_PARASITE_PERSISTENT << 16)
#define CLAY_PARASITE_GRANDPARENT_UNDOABLE   (CLAY_PARASITE_UNDOABLE << 16)


/**
 * ClayParasite:
 * @name:  the parasite name, USE A UNIQUE PREFIX
 * @flags: the parasite flags, see [const@Clay.PARASITE_PERSISTENT] and
 *   [const@Clay.PARASITE_UNDOABLE]
 * @size:  the parasite size in bytes
 * @data: (array length=size): the parasite data, the owner of the parasite is responsible
 *   for tracking byte order and internal structure
 **/
struct _ClayParasite
{
  gchar    *name;
  guint32   flags;
  guint32   size;
  gpointer  data;
};


ClayParasite * clay_parasite_new           (const gchar        *name,
                                            guint32             flags,
                                            guint32             size,
                                            gconstpointer       data);
void           clay_parasite_free          (ClayParasite       *parasite);

ClayParasite * clay_parasite_copy          (const ClayParasite *parasite);

gboolean       clay_parasite_compare       (const ClayParasite *a,
                                            const ClayParasite *b);

gboolean       clay_parasite_is_type       (const ClayParasite *parasite,
                                            const gchar        *name);
gboolean       clay_parasite_is_persistent (const ClayParasite *parasite);
gboolean       clay_parasite_is_undoable   (const ClayParasite *parasite);
gboolean       clay_parasite_has_flag      (const ClayParasite *parasite,
                                            gulong              flag);
gulong         clay_parasite_get_flags     (const ClayParasite *parasite);
const gchar  * clay_parasite_get_name      (const ClayParasite *parasite);

gconstpointer  clay_parasite_get_data      (const ClayParasite *parasite,
                                            guint32            *num_bytes);

G_END_DECLS

#endif /* __CLAY_PARASITE_H__ */
