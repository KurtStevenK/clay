/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybufferview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_BUFFER_VIEW            (clay_buffer_view_get_type ())
#define CLAY_BUFFER_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BUFFER_VIEW, ClayBufferView))
#define CLAY_BUFFER_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BUFFER_VIEW, ClayBufferViewClass))
#define CLAY_IS_BUFFER_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BUFFER_VIEW))
#define CLAY_IS_BUFFER_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BUFFER_VIEW))
#define CLAY_BUFFER_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BUFFER_VIEW, ClayBufferViewClass))


typedef struct _ClayBufferViewClass  ClayBufferViewClass;

struct _ClayBufferView
{
  ClayContainerEditor  parent_instance;

  GtkWidget           *clipboard_view;
  GtkWidget           *clipboard_label;

  GtkWidget           *paste_button;
  GtkWidget           *paste_into_button;
  GtkWidget           *paste_as_new_layer_button;
  GtkWidget           *paste_as_new_image_button;
  GtkWidget           *delete_button;
};

struct _ClayBufferViewClass
{
  ClayContainerEditorClass  parent_class;
};


GType       clay_buffer_view_get_type (void);

GtkWidget * clay_buffer_view_new      (ClayViewType     view_type,
                                       ClayContainer   *container,
                                       ClayContext     *context,
                                       gint             view_size,
                                       gint             view_border_width,
                                       ClayMenuFactory *menu_factory);
