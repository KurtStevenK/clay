/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitemtreeview-search.h
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


void   _clay_item_tree_view_search_create       (ClayItemTreeView *view,
                                                 GtkWidget        *parent);
void   _clay_item_tree_view_search_destroy      (ClayItemTreeView *view);

void   _clay_item_tree_view_search_show         (ClayItemTreeView *view);
void   _clay_item_tree_view_search_hide         (ClayItemTreeView *view);

void   _clay_item_tree_view_search_update_links (ClayItemTreeView *view,
                                                 ClayImage        *image,
                                                 GType             item_type);
