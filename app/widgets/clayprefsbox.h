/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprefsbox.h
 * Copyright (C) 2013-2016 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define CLAY_TYPE_PREFS_BOX (clay_prefs_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayPrefsBox,
                          clay_prefs_box,
                          CLAY, PREFS_BOX,
                          GtkBox)


struct _ClayPrefsBoxClass
{
  GtkBoxClass  parent_class;
};


GtkWidget   * clay_prefs_box_new                   (void);

GtkWidget   * clay_prefs_box_add_page              (ClayPrefsBox *box,
                                                    const gchar  *icon_name,
                                                    const gchar  *page_title,
                                                    const gchar  *tree_label,
                                                    const gchar  *help_id,
                                                    GtkTreeIter  *parent,
                                                    GtkTreeIter  *iter);

const gchar * clay_prefs_box_get_current_icon_name (ClayPrefsBox *box);
const gchar * clay_prefs_box_get_current_help_id   (ClayPrefsBox *box);

void          clay_prefs_box_set_header_visible    (ClayPrefsBox *box,
                                                    gboolean      header_visible);
void          clay_prefs_box_set_page_scrollable   (ClayPrefsBox *box,
                                                    GtkWidget    *page,
                                                    gboolean      scrollable);
GtkWidget   * clay_prefs_box_set_page_resettable   (ClayPrefsBox *box,
                                                    GtkWidget    *page,
                                                    const gchar  *label);

GtkWidget   * clay_prefs_box_get_tree_view         (ClayPrefsBox *box);
GtkWidget   * clay_prefs_box_get_stack             (ClayPrefsBox *box);
