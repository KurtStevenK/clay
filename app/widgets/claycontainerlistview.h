/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerlistview.h
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CONTAINER_LIST_VIEW (clay_container_list_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayContainerListView,
                          clay_container_list_view,
                          CLAY, CONTAINER_LIST_VIEW,
                          ClayContainerBox)


struct _ClayContainerListViewClass
{
  ClayContainerBoxClass       parent_class;

  GtkListBoxCreateWidgetFunc  create_row_func;
};


GtkWidget * clay_container_list_view_new (ClayContainer *container,
                                          ClayContext   *context,
                                          gint           view_size,
                                          gint           view_border_width);

void        clay_container_list_view_set_search_entry
                                         (ClayContainerListView *list_view,
                                          GtkSearchEntry        *entry);
