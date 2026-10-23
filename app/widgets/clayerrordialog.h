/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayerrordialog.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_ERROR_DIALOG            (clay_error_dialog_get_type ())
#define CLAY_ERROR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ERROR_DIALOG, ClayErrorDialog))
#define CLAY_ERROR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ERROR_DIALOG, ClayErrorDialogClass))
#define CLAY_IS_ERROR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ERROR_DIALOG))
#define CLAY_IS_ERROR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ERROR_DIALOG))
#define CLAY_ERROR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ERROR_DIALOG, ClayErrorDialogClass))


typedef struct _ClayErrorDialogClass  ClayErrorDialogClass;

struct _ClayErrorDialog
{
  ClayDialog       parent_instance;

  GtkWidget       *vbox;

  GList           *messages;
  gboolean         overflow;
};

struct _ClayErrorDialogClass
{
  ClayDialogClass  parent_class;
};


GType       clay_error_dialog_get_type (void);

GtkWidget * clay_error_dialog_new      (const gchar     *title);
void        clay_error_dialog_add      (ClayErrorDialog *dialog,
                                        const gchar     *icon_name,
                                        const gchar     *domain,
                                        const gchar     *message);
