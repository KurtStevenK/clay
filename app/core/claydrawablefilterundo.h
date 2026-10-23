/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "clayundo.h"


#define CLAY_TYPE_DRAWABLE_FILTER_UNDO            (clay_drawable_filter_undo_get_type ())
#define CLAY_DRAWABLE_FILTER_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE_FILTER_UNDO, ClayDrawableFilterUndo))
#define CLAY_DRAWABLE_FILTER_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAWABLE_FILTER_UNDO, ClayDrawableFilterUndoClass))
#define CLAY_IS_DRAWABLE_FILTER_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE_FILTER_UNDO))
#define CLAY_IS_DRAWABLE_FILTER_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAWABLE_FILTER_UNDO))
#define CLAY_DRAWABLE_FILTER_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAWABLE_FILTER_UNDO, ClayDrawableFilterUndoClass))


typedef struct _ClayDrawableFilterUndo      ClayDrawableFilterUndo;
typedef struct _ClayDrawableFilterUndoClass ClayDrawableFilterUndoClass;

struct _ClayDrawableFilterUndo
{
  ClayUndo               parent_instance;

  ClayDrawableFilter    *filter;
  gint                   row_index;
  gboolean               active;

  GeglNode              *node;
  gdouble                opacity;
  ClayLayerMode          paint_mode;
  ClayLayerColorSpace    blend_space;
  ClayLayerColorSpace    composite_space;
  ClayLayerCompositeMode composite_mode;
  ClayFilterRegion       region;
};

struct _ClayDrawableFilterUndoClass
{
  ClayUndoClass parent_class;
};


GType      clay_drawable_filter_undo_get_type (void);

ClayUndo * clay_drawable_filter_undo_can_compress_visibility (ClayImage *image,
                                                              GList     *filter_list);
