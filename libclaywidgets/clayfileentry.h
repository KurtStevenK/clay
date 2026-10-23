/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayfileentry.h
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
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

#ifndef CLAY_DISABLE_DEPRECATED

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_FILE_ENTRY_H__
#define __CLAY_FILE_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/* The whole ClayFileEntry widget was deprecated in 2006 (commit
 * 99f979e1189) apparently to be replaced by GtkFileChooserButton, yet
 * it is still used in the ClayPathEditor. For Clay 3.0, we removed the
 * header from installed headers and made the class and all functions
 * internal, though we still use it.
 *
 * TODO: eventually we want either this widget fully removed and
 * replaced by a generic GTK widget or reimplemented and made public
 * again.
 */

#define CLAY_TYPE_FILE_ENTRY (_clay_file_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayFileEntry, _clay_file_entry, CLAY, FILE_ENTRY, GtkBox)


G_GNUC_INTERNAL GtkWidget * _clay_file_entry_new          (const gchar   *title,
                                                           const gchar   *filename,
                                                           gboolean       dir_only,
                                                           gboolean       check_valid);

G_GNUC_INTERNAL gchar     * _clay_file_entry_get_filename (ClayFileEntry *entry);
G_GNUC_INTERNAL void        _clay_file_entry_set_filename (ClayFileEntry *entry,
                                                           const gchar   *filename);

G_GNUC_INTERNAL GtkWidget * _clay_file_entry_get_entry    (ClayFileEntry *entry);


G_END_DECLS

#endif /* __CLAY_FILE_ENTRY_H__ */

#endif /* CLAY_DISABLE_DEPRECATED */
