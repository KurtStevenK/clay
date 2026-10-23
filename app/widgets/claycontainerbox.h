/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerbox.h
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

#include "clayeditor.h"


#define CLAY_TYPE_CONTAINER_BOX            (clay_container_box_get_type ())
#define CLAY_CONTAINER_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_BOX, ClayContainerBox))
#define CLAY_CONTAINER_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_BOX, ClayContainerBoxClass))
#define CLAY_IS_CONTAINER_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_BOX))
#define CLAY_IS_CONTAINER_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_BOX))
#define CLAY_CONTAINER_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_BOX, ClayContainerBoxClass))


typedef struct _ClayContainerBoxClass  ClayContainerBoxClass;

struct _ClayContainerBox
{
  ClayEditor  parent_instance;

  GtkWidget  *scrolled_win;
};

struct _ClayContainerBoxClass
{
  ClayEditorClass  parent_class;
};


GType     clay_container_box_get_type         (void);

void      clay_container_box_set_size_request (ClayContainerBox *box,
                                               gint              width,
                                               gint              height);


G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayContainerBox, g_object_unref)
