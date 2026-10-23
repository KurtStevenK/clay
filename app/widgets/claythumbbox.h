/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


#define CLAY_TYPE_THUMB_BOX            (clay_thumb_box_get_type ())
#define CLAY_THUMB_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_THUMB_BOX, ClayThumbBox))
#define CLAY_THUMB_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_THUMB_BOX, ClayThumbBoxClass))
#define CLAY_IS_THUMB_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_THUMB_BOX))
#define CLAY_IS_THUMB_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_THUMB_BOX))
#define CLAY_THUMB_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_THUMB_BOX, ClayThumbBoxClass))


typedef struct _ClayThumbBoxClass ClayThumbBoxClass;

struct _ClayThumbBox
{
  GtkFrame       parent_instance;

  ClayContext   *context;
  ClayImagefile *imagefile;
  GSList        *files;

  GtkWidget     *preview;
  GtkWidget     *filename;
  GtkWidget     *info;

  gboolean       progress_active;
  GtkWidget     *progress;

  guint          idle_id;
};

struct _ClayThumbBoxClass
{
  GtkFrameClass  parent_class;
};


GType       clay_thumb_box_get_type  (void);

GtkWidget * clay_thumb_box_new        (ClayContext  *context);

void        clay_thumb_box_take_file  (ClayThumbBox *box,
                                       GFile        *file);
void        clay_thumb_box_take_files (ClayThumbBox *box,
                                       GSList       *files);
