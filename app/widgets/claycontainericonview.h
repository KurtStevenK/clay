/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainericonview.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "claycontainerbox.h"


#define CLAY_TYPE_CONTAINER_ICON_VIEW            (clay_container_icon_view_get_type ())
#define CLAY_CONTAINER_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_ICON_VIEW, ClayContainerIconView))
#define CLAY_CONTAINER_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_ICON_VIEW, ClayContainerIconViewClass))
#define CLAY_IS_CONTAINER_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_ICON_VIEW))
#define CLAY_IS_CONTAINER_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_ICON_VIEW))
#define CLAY_CONTAINER_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_ICON_VIEW, ClayContainerIconViewClass))


typedef struct _ClayContainerIconViewClass   ClayContainerIconViewClass;
typedef struct _ClayContainerIconViewPrivate ClayContainerIconViewPrivate;

struct _ClayContainerIconView
{
  ClayContainerBox              parent_instance;

  GtkTreeModel                 *model;
  gint                          n_model_columns;
  GType                         model_columns[16];

  GtkIconView                  *view;

  GtkCellRenderer              *renderer_cell;

  ClayContainerIconViewPrivate *priv;
};

struct _ClayContainerIconViewClass
{
  ClayContainerBoxClass  parent_class;
};


GType       clay_container_icon_view_get_type (void);

GtkWidget * clay_container_icon_view_new      (ClayContainer *container,
                                               ClayContext   *context,
                                               gint           view_size,
                                               gint           view_border_width);
