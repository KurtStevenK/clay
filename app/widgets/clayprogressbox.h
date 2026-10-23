/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprogressbox.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PROGRESS_BOX            (clay_progress_box_get_type ())
#define CLAY_PROGRESS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PROGRESS_BOX, ClayProgressBox))
#define CLAY_PROGRESS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PROGRESS_BOX, ClayProgressBoxClass))
#define CLAY_IS_PROGRESS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PROGRESS_BOX))
#define CLAY_IS_PROGRESS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PROGRESS_BOX))
#define CLAY_PROGRESS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PROGRESS_BOX, ClayProgressBoxClass))


typedef struct _ClayProgressBoxClass  ClayProgressBoxClass;

struct _ClayProgressBox
{
  GtkBox     parent_instance;

  gboolean   active;
  gboolean   cancellable;
  gdouble    value;

  GtkWidget *label;
  GtkWidget *progress;
};

struct _ClayProgressBoxClass
{
  GtkBoxClass  parent_class;
};


GType       clay_progress_box_get_type (void);

GtkWidget * clay_progress_box_new      (void);
