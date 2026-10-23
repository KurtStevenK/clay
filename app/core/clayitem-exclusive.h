/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitem-exclusive.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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


typedef gboolean        (* ClayItemIsEnabledFunc)    (ClayItem  *item);
typedef gboolean        (* ClayItemCanSetFunc)       (ClayItem  *item);
typedef void            (* ClayItemSetFunc)          (ClayItem  *item,
                                                      gboolean   enabled,
                                                      gboolean   push_undo);
typedef gboolean        (* ClayItemIsPropLocked)     (ClayItem  *item);
typedef ClayUndo      * (*ClayItemUndoPush)          (ClayImage     *image,
                                                      const gchar   *undo_desc,
                                                      ClayItem      *item);

void   clay_item_toggle_exclusive_visible (ClayItem             *item,
                                           gboolean              only_selected,
                                           ClayContext          *context);
void   clay_item_toggle_exclusive         (ClayItem             *item,
                                           ClayItemIsEnabledFunc is_enabled,
                                           ClayItemSetFunc       set_prop,
                                           ClayItemCanSetFunc    can_set,
                                           ClayItemIsPropLocked  is_prop_locked,
                                           ClayItemUndoPush      undo_push,
                                           const gchar          *undo_desc,
                                           ClayUndoType          group_undo_type,
                                           gboolean              only_selected,
                                           ClayContext          *context);
