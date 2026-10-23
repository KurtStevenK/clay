/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claychanneltreeview.h
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


#define CLAY_TYPE_CHANNEL_TREE_VIEW            (clay_channel_tree_view_get_type ())
#define CLAY_CHANNEL_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CHANNEL_TREE_VIEW, ClayChannelTreeView))
#define CLAY_CHANNEL_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CHANNEL_TREE_VIEW, ClayChannelTreeViewClass))
#define CLAY_IS_CHANNEL_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CHANNEL_TREE_VIEW))
#define CLAY_IS_CHANNEL_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CHANNEL_TREE_VIEW))
#define CLAY_CHANNEL_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CHANNEL_TREE_VIEW, ClayChannelTreeViewClass))


typedef struct _ClayChannelTreeViewClass   ClayChannelTreeViewClass;
typedef struct _ClayChannelTreeViewPrivate ClayChannelTreeViewPrivate;

struct _ClayChannelTreeView
{
  ClayDrawableTreeView        parent_instance;

  ClayChannelTreeViewPrivate *priv;
};

struct _ClayChannelTreeViewClass
{
  ClayDrawableTreeViewClass  parent_class;
};


GType   clay_channel_tree_view_get_type (void);
