/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayitemstack.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include "clayfilterstack.h"


#define CLAY_TYPE_ITEM_STACK (clay_item_stack_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayItemStack,
                          clay_item_stack,
                          CLAY, ITEM_STACK,
                          ClayFilterStack)


struct _ClayItemStackClass
{
  ClayFilterStackClass  parent_class;
};


ClayContainer * clay_item_stack_new                 (GType          item_type);

gint            clay_item_stack_get_n_items         (ClayItemStack *stack);
gboolean        clay_item_stack_is_flat             (ClayItemStack *stack);
GList         * clay_item_stack_get_item_iter       (ClayItemStack *stack);
GList         * clay_item_stack_get_item_list       (ClayItemStack *stack);
ClayItem      * clay_item_stack_get_item_by_tattoo  (ClayItemStack *stack,
                                                     ClayTattoo     tattoo);
ClayItem      * clay_item_stack_get_item_by_path    (ClayItemStack *stack,
                                                     GList         *path);
ClayItem      * clay_item_stack_get_parent_by_path  (ClayItemStack *stack,
                                                     GList         *path,
                                                     gint          *index);

void            clay_item_stack_invalidate_previews (ClayItemStack *stack);
void            clay_item_stack_profile_changed     (ClayItemStack *stack);
