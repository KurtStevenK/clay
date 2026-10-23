/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymessagebox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_MESSAGE_BOX            (clay_message_box_get_type ())
#define CLAY_MESSAGE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MESSAGE_BOX, ClayMessageBox))
#define CLAY_MESSAGE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MESSAGE_BOX, ClayMessageBoxClass))
#define CLAY_IS_MESSAGE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MESSAGE_BOX))
#define CLAY_IS_MESSAGE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MESSAGE_BOX))
#define CLAY_MESSAGE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MESSAGE_BOX, ClayMessageBoxClass))


typedef struct _ClayMessageBoxClass  ClayMessageBoxClass;

struct _ClayMessageBox
{
  GtkBox     parent_instance;

  gchar     *icon_name;
  gint       repeat;
  GtkWidget *label[3];
  GtkWidget *image;

  guint      idle_id;
};

struct _ClayMessageBoxClass
{
  GtkBoxClass  parent_class;
};


GType       clay_message_box_get_type         (void);

GtkWidget * clay_message_box_new              (const gchar    *icon_name);
void        clay_message_box_set_primary_text (ClayMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void        clay_message_box_set_text         (ClayMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void        clay_message_box_set_markup       (ClayMessageBox *box,
                                               const gchar    *format,
                                               ...) G_GNUC_PRINTF (2, 3);
gint        clay_message_box_repeat           (ClayMessageBox *box);
