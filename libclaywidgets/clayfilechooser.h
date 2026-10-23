/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayfilechooser.c
 * Copyright (C) 2025 Jehan
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

#ifndef __CLAY_FILE_CHOOSER_H__
#define __CLAY_FILE_CHOOSER_H__

G_BEGIN_DECLS

#define CLAY_TYPE_FILE_CHOOSER (clay_file_chooser_get_type ())
G_DECLARE_FINAL_TYPE (ClayFileChooser, clay_file_chooser, CLAY, FILE_CHOOSER, GtkBox)


GtkWidget             * clay_file_chooser_new              (ClayFileChooserAction  action,
                                                            const gchar           *label,
                                                            const gchar           *title,
                                                            GFile                 *file);

ClayFileChooserAction   clay_file_chooser_get_action       (ClayFileChooser       *chooser);
void                    clay_file_chooser_set_action       (ClayFileChooser       *chooser,
                                                            ClayFileChooserAction  action);

GFile                 * clay_file_chooser_get_file         (ClayFileChooser       *chooser);
void                    clay_file_chooser_set_file         (ClayFileChooser       *chooser,
                                                            GFile                 *file);

const gchar           * clay_file_chooser_get_label        (ClayFileChooser       *chooser);
void                    clay_file_chooser_set_label        (ClayFileChooser       *chooser,
                                                            const gchar           *text);

const gchar           * clay_file_chooser_get_title        (ClayFileChooser       *chooser);
void                    clay_file_chooser_set_title        (ClayFileChooser       *chooser,
                                                            const gchar           *text);

GtkWidget             * clay_file_chooser_get_label_widget (ClayFileChooser       *chooser);


G_END_DECLS

#endif /* __CLAY_FILE_CHOOSER_H__ */
