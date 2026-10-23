/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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


#define CLAY_TYPE_OBJECT (clay_object_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayObject,
                          clay_object,
                          CLAY, OBJECT,
                          GObject)


struct _ClayObjectClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void    (* disconnect)   (ClayObject *object);
  void    (* name_changed) (ClayObject *object);

  /*  virtual functions  */
  gint64  (* get_memsize)  (ClayObject *object,
                            gint64     *gui_size);
};


void          clay_object_set_name        (ClayObject       *object,
                                           const gchar      *name);
void          clay_object_set_name_safe   (ClayObject       *object,
                                           const gchar      *name);
void          clay_object_set_static_name (ClayObject       *object,
                                           const gchar      *name);
void          clay_object_take_name       (ClayObject       *object,
                                           gchar            *name);
const gchar * clay_object_get_name        (gconstpointer     object);
void          clay_object_name_changed    (ClayObject       *object);
void          clay_object_name_free       (ClayObject       *object);

gint          clay_object_name_collate    (ClayObject       *object1,
                                           ClayObject       *object2);
gint64        clay_object_get_memsize     (ClayObject       *object,
                                           gint64           *gui_size);
