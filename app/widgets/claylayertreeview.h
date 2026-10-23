/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylayertreeview.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#include "claydrawabletreeview.h"


#define CLAY_TYPE_LAYER_TREE_VIEW            (clay_layer_tree_view_get_type ())
#define CLAY_LAYER_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LAYER_TREE_VIEW, ClayLayerTreeView))
#define CLAY_LAYER_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LAYER_TREE_VIEW, ClayLayerTreeViewClass))
#define CLAY_IS_LAYER_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LAYER_TREE_VIEW))
#define CLAY_IS_LAYER_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LAYER_TREE_VIEW))
#define CLAY_LAYER_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LAYER_TREE_VIEW, ClayLayerTreeViewClass))


typedef struct _ClayLayerTreeViewClass   ClayLayerTreeViewClass;
typedef struct _ClayLayerTreeViewPrivate ClayLayerTreeViewPrivate;

struct _ClayLayerTreeView
{
  ClayDrawableTreeView      parent_instance;

  ClayLayerTreeViewPrivate *priv;
};

struct _ClayLayerTreeViewClass
{
  ClayDrawableTreeViewClass  parent_class;
};


GType   clay_layer_tree_view_get_type (void);
