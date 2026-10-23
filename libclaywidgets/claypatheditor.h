/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypatheditor.h
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_PATH_EDITOR_H__
#define __CLAY_PATH_EDITOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PATH_EDITOR (clay_path_editor_get_type ())
G_DECLARE_FINAL_TYPE (ClayPathEditor, clay_path_editor, CLAY, PATH_EDITOR, GtkBox)


/* For information look into the C source or the html documentation */

GtkWidget * clay_path_editor_new               (const gchar    *title,
                                                const gchar    *path);

gchar     * clay_path_editor_get_path          (ClayPathEditor *editor);
void        clay_path_editor_set_path          (ClayPathEditor *editor,
                                                const gchar    *path);

gchar     * clay_path_editor_get_writable_path (ClayPathEditor *editor);
void        clay_path_editor_set_writable_path (ClayPathEditor *editor,
                                                const gchar    *path);

gboolean    clay_path_editor_get_dir_writable  (ClayPathEditor *editor,
                                                const gchar    *directory);
void        clay_path_editor_set_dir_writable  (ClayPathEditor *editor,
                                                const gchar    *directory,
                                                gboolean        writable);

G_END_DECLS

#endif /* __CLAY_PATH_EDITOR_H__ */
