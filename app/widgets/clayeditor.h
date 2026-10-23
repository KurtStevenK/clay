/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_EDITOR (clay_editor_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayEditor,
                          clay_editor,
                          CLAY, EDITOR,
                          GtkBox)


struct _ClayEditorClass
{
  GtkBoxClass  parent_class;
};


GtkWidget * clay_editor_new               (void);

void        clay_editor_create_menu       (ClayEditor           *editor,
                                           ClayMenuFactory      *menu_factory,
                                           const gchar          *menu_identifier,
                                           const gchar          *ui_path,
                                           gpointer              popup_data);
gboolean    clay_editor_popup_menu        (ClayEditor           *editor,
                                           ClayMenuPositionFunc  position_func,
                                           gpointer              position_data);
gboolean    clay_editor_popup_menu_at_pointer
                                          (ClayEditor           *editor,
                                           const GdkEvent       *trigger_event);
gboolean   clay_editor_popup_menu_at_rect (ClayEditor           *editor,
                                           GdkWindow            *window,
                                           const GdkRectangle   *rect,
                                           GdkGravity            rect_anchor,
                                           GdkGravity            menu_anchor,
                                           const GdkEvent       *trigger_event);

GtkWidget * clay_editor_add_button        (ClayEditor           *editor,
                                           const gchar          *icon_name,
                                           const gchar          *tooltip,
                                           const gchar          *help_id,
                                           GCallback             callback,
                                           GCallback             extended_callback,
                                           GObject              *callback_data);
GtkWidget * clay_editor_add_icon_box      (ClayEditor           *editor,
                                           GType                 enum_type,
                                           const gchar          *icon_prefix,
                                           GCallback             callback,
                                           gpointer              callback_data);

GtkWidget * clay_editor_get_top_box       (ClayEditor           *editor);

GtkWidget * clay_editor_add_action_button (ClayEditor           *editor,
                                           const gchar          *group_name,
                                           const gchar          *action_name,
                                           ...) G_GNUC_NULL_TERMINATED;
void        clay_editor_set_action_sensitive
                                          (ClayEditor          *editor,
                                           const gchar         *group_name,
                                           const gchar         *action_name,
                                           gboolean             sensitive,
                                           const gchar         *reason);

void        clay_editor_set_show_name     (ClayEditor          *editor,
                                           gboolean             show);
void        clay_editor_set_name          (ClayEditor          *editor,
                                           const gchar         *name);

void        clay_editor_set_box_style     (ClayEditor          *editor,
                                           GtkBox              *box);
ClayUIManager *
            clay_editor_get_ui_manager    (ClayEditor          *editor);
GtkBox    * clay_editor_get_button_box    (ClayEditor          *editor);
ClayMenuFactory *
            clay_editor_get_menu_factory  (ClayEditor          *editor);
gpointer  * clay_editor_get_popup_data    (ClayEditor          *editor);
gchar     * clay_editor_get_ui_path       (ClayEditor          *editor);
