/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawabletreeview.h
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

#include "clayitemtreeview.h"


#define CLAY_TYPE_DRAWABLE_TREE_VIEW            (clay_drawable_tree_view_get_type ())
#define CLAY_DRAWABLE_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE_TREE_VIEW, ClayDrawableTreeView))
#define CLAY_DRAWABLE_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAWABLE_TREE_VIEW, ClayDrawableTreeViewClass))
#define CLAY_IS_DRAWABLE_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE_TREE_VIEW))
#define CLAY_IS_DRAWABLE_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAWABLE_TREE_VIEW))
#define CLAY_DRAWABLE_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAWABLE_TREE_VIEW, ClayDrawableTreeViewClass))


typedef struct _ClayDrawableTreeViewClass         ClayDrawableTreeViewClass;
typedef struct _ClayDrawableTreeViewPrivate       ClayDrawableTreeViewPrivate;
typedef struct _ClayDrawableTreeViewFiltersEditor ClayDrawableTreeViewFiltersEditor;

struct _ClayDrawableTreeView
{
  ClayItemTreeView                   parent_instance;

  ClayDrawableTreeViewPrivate       *priv;
  ClayDrawableTreeViewFiltersEditor *editor;
};

struct _ClayDrawableTreeViewClass
{
  ClayItemTreeViewClass  parent_class;
};


GType   clay_drawable_tree_view_get_type (void);
