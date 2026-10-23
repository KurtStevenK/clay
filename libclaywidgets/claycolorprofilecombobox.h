/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorprofilecombobox.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_COLOR_PROFILE_COMBO_BOX_H__
#define __CLAY_COLOR_PROFILE_COMBO_BOX_H__

G_BEGIN_DECLS

#define CLAY_TYPE_COLOR_PROFILE_COMBO_BOX (clay_color_profile_combo_box_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorProfileComboBox, clay_color_profile_combo_box, CLAY, COLOR_PROFILE_COMBO_BOX, GtkComboBox)


GtkWidget * clay_color_profile_combo_box_new                (GtkWidget    *dialog,
                                                             GFile        *history);
GtkWidget * clay_color_profile_combo_box_new_with_model     (GtkWidget    *dialog,
                                                             GtkTreeModel *model);

void        clay_color_profile_combo_box_add_file           (ClayColorProfileComboBox *combo,
                                                             GFile                    *file,
                                                             const gchar              *label);

void        clay_color_profile_combo_box_set_active_file    (ClayColorProfileComboBox *combo,
                                                             GFile                    *file,
                                                             const gchar              *label);
void        clay_color_profile_combo_box_set_active_profile (ClayColorProfileComboBox *combo,
                                                             ClayColorProfile         *profile);

GFile *     clay_color_profile_combo_box_get_active_file    (ClayColorProfileComboBox *combo);


G_END_DECLS

#endif  /* __CLAY_COLOR_PROFILE_COMBO_BOX_H__ */
