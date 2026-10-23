/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimagechooser.h
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


#define CLAY_TYPE_IMAGE_CHOOSER            (clay_image_chooser_get_type ())
#define CLAY_IMAGE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_CHOOSER, ClayImageChooser))
#define CLAY_IMAGE_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_CHOOSER, ClayImageChooserClass))
#define CLAY_IS_IMAGE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_CHOOSER))
#define CLAY_IS_IMAGE_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_CHOOSER))
#define CLAY_IMAGE_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_CHOOSER, ClayImageChooserClass))


typedef struct _ClayImageChooserPrivate ClayImageChooserPrivate;
typedef struct _ClayImageChooserClass   ClayImageChooserClass;

struct _ClayImageChooser
{
  GtkFrame                    parent_instance;

  ClayImageChooserPrivate *priv;
};

struct _ClayImageChooserClass
{
  GtkFrameClass  parent_class;

  /* Signals. */

  void (* activate) (ClayImageChooser *view,
                     ClayImage        *image);
};


GType          clay_image_chooser_get_type     (void);

GtkWidget    * clay_image_chooser_new          (ClayContext         *context,
                                                gint                 view_size,
                                                gint                 view_border_width);

ClayImage    * clay_image_chooser_get_image    (ClayImageChooser    *chooser);
void           clay_image_chooser_set_image    (ClayImageChooser    *chooser,
                                                ClayImage           *image);
