/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImageProfileView
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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

#include "clayimageparasiteview.h"


#define CLAY_TYPE_IMAGE_PROFILE_VIEW            (clay_image_profile_view_get_type ())
#define CLAY_IMAGE_PROFILE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_PROFILE_VIEW, ClayImageProfileView))
#define CLAY_IMAGE_PROFILE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_PROFILE_VIEW, ClayImageProfileViewClass))
#define CLAY_IS_IMAGE_PROFILE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_PROFILE_VIEW))
#define CLAY_IS_IMAGE_PROFILE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_PROFILE_VIEW))
#define CLAY_IMAGE_PROFILE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_PROFILE_VIEW, ClayImageProfileViewClass))


typedef struct _ClayImageProfileViewClass ClayImageProfileViewClass;

struct _ClayImageProfileView
{
  ClayImageParasiteView  parent_instance;

  ClayColorProfileView  *profile_view;
};

struct _ClayImageProfileViewClass
{
  ClayImageParasiteViewClass  parent_class;
};


GType       clay_image_profile_view_get_type (void);

GtkWidget * clay_image_profile_view_new      (ClayImage *image);
