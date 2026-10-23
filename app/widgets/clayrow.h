/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrow.h
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_ROW (clay_row_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayRow,
                          clay_row,
                          CLAY, ROW,
                          GtkListBoxRow)


struct _ClayRowClass
{
  GtkListBoxRowClass  parent_class;

  /*  virtual functions  */
  void     (* set_context)     (ClayRow      *row,
                                ClayContext  *context);
  void     (* set_viewable)    (ClayRow      *row,
                                ClayViewable *viewable);
  void     (* set_view_size)   (ClayRow      *row);

  void     (* icon_changed)    (ClayRow      *row);
  void     (* name_changed)    (ClayRow      *row);
  void     (* monitor_changed) (ClayRow      *row);

  /*  signal for keybinding  */
  void     (* edit_name)       (ClayRow      *row);

  /*  virtual functions  */
  gboolean (* name_edited)     (ClayRow      *row,
                                const gchar  *new_name);
};


GtkWidget    * clay_row_new             (GType         row_type,
                                         ClayContext  *context,
                                         ClayViewable *viewable,
                                         gint          view_size,
                                         gint          view_border_width);

void           clay_row_set_context     (ClayRow      *row,
                                         ClayContext  *context);
ClayContext  * clay_row_get_context     (ClayRow      *row);

void           clay_row_set_viewable    (ClayRow      *row,
                                         ClayViewable *viewable);
ClayViewable * clay_row_get_viewable    (ClayRow      *row);

void           clay_row_set_view_size   (ClayRow      *row,
                                         gint          view_size,
                                         gint          view_border_width);
gint           clay_row_get_view_size   (ClayRow      *row,
                                         gint         *view_border_width);

void           clay_row_monitor_changed (ClayRow      *row);


/*  protected  */

GtkWidget    * _clay_row_get_box        (ClayRow      *row);
GtkWidget    * _clay_row_get_icon       (ClayRow      *row);
GtkWidget    * _clay_row_get_view       (ClayRow      *row);
GtkWidget    * _clay_row_get_label      (ClayRow      *row);

GtkWidget    * _clay_row_add_toggle     (ClayRow      *row,
                                         const gchar  *icon_name,
                                         GtkWidget   **icon);
