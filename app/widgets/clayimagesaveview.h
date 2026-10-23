/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimagesaveview.h
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include "claycontainerlistview.h"


#define CLAY_TYPE_IMAGE_SAVE_VIEW (clay_image_save_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayImageSaveView,
                          clay_image_save_view,
                          CLAY, IMAGE_SAVE_VIEW,
                          ClayContainerListView)


struct _ClayImageSaveViewClass
{
  ClayContainerListViewClass  parent_class;
};


GtkWidget * clay_image_save_view_new (ClayContainer *container,
                                      ClayContext   *context,
                                      gint           view_size,
                                      gint           view_border_width);
