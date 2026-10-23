/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayopendialog.h
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include "clayfiledialog.h"


#define CLAY_TYPE_OPEN_DIALOG            (clay_open_dialog_get_type ())
#define CLAY_OPEN_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPEN_DIALOG, ClayOpenDialog))
#define CLAY_OPEN_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_OPEN_DIALOG, ClayOpenDialogClass))
#define CLAY_IS_OPEN_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPEN_DIALOG))
#define CLAY_IS_OPEN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_OPEN_DIALOG))
#define CLAY_OPEN_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_OPEN_DIALOG, ClayOpenDialogClass))


typedef struct _ClayOpenDialogClass ClayOpenDialogClass;

struct _ClayOpenDialog
{
  ClayFileDialog       parent_instance;

  gboolean             open_as_layers;
  gboolean             open_as_link;
};

struct _ClayOpenDialogClass
{
  ClayFileDialogClass  parent_class;
};


GType       clay_open_dialog_get_type   (void);

GtkWidget * clay_open_dialog_new        (Clay           *clay);

void        clay_open_dialog_set_image  (ClayOpenDialog *dialog,
                                         ClayImage      *image,
                                         gboolean        open_as_layers,
                                         gboolean        open_as_link);
