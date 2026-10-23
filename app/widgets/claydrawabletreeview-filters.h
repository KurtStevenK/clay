/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawabletreeview-filters.h
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


gboolean   _clay_drawable_tree_view_filter_editor_show    (ClayDrawableTreeView *view,
                                                           ClayDrawable         *drawable,
                                                           GdkRectangle         *rect);
void       _clay_drawable_tree_view_filter_editor_hide    (ClayDrawableTreeView *view);

void       _clay_drawable_tree_view_filter_editor_destroy (ClayDrawableTreeView *view);
