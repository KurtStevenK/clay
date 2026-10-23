/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimageview.h
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

#include "claycontainereditor.h"


#define CLAY_TYPE_IMAGE_VIEW            (clay_image_view_get_type ())
#define CLAY_IMAGE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_VIEW, ClayImageView))
#define CLAY_IMAGE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_VIEW, ClayImageViewClass))
#define CLAY_IS_IMAGE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_VIEW))
#define CLAY_IS_IMAGE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_VIEW))
#define CLAY_IMAGE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_VIEW, ClayImageViewClass))


typedef struct _ClayImageViewClass ClayImageViewClass;

struct _ClayImageView
{
  ClayContainerEditor  parent_instance;

  GtkWidget           *raise_button;
  GtkWidget           *new_button;
  GtkWidget           *delete_button;

  ClayImage           *image;
};

struct _ClayImageViewClass
{
  ClayContainerEditorClass  parent_class;
};


GType       clay_image_view_get_type (void);

GtkWidget * clay_image_view_new      (ClayViewType     view_type,
                                      ClayContainer   *container,
                                      ClayContext     *context,
                                      gint             view_size,
                                      gint             view_border_width,
                                      ClayMenuFactory *menu_factory);
