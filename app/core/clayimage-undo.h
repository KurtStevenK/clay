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


gboolean        clay_image_undo_is_enabled      (ClayImage     *image);
gboolean        clay_image_undo_enable          (ClayImage     *image);
gboolean        clay_image_undo_disable         (ClayImage     *image);
gboolean        clay_image_undo_freeze          (ClayImage     *image);
gboolean        clay_image_undo_thaw            (ClayImage     *image);

gboolean        clay_image_undo                 (ClayImage     *image);
gboolean        clay_image_redo                 (ClayImage     *image);

gboolean        clay_image_strong_undo          (ClayImage     *image);
gboolean        clay_image_strong_redo          (ClayImage     *image);

ClayUndoStack * clay_image_get_undo_stack       (ClayImage     *image);
ClayUndoStack * clay_image_get_redo_stack       (ClayImage     *image);

void            clay_image_undo_free            (ClayImage     *image);

gint            clay_image_get_undo_group_count (ClayImage     *image);
gboolean        clay_image_undo_group_start     (ClayImage     *image,
                                                 ClayUndoType   undo_type,
                                                 const gchar   *name);
gboolean        clay_image_undo_group_end       (ClayImage     *image);

ClayUndo      * clay_image_undo_push            (ClayImage     *image,
                                                 GType          object_type,
                                                 ClayUndoType   undo_type,
                                                 const gchar   *name,
                                                 ClayDirtyMask  dirty_mask,
                                                 ...) G_GNUC_NULL_TERMINATED;

ClayUndo      * clay_image_undo_can_compress    (ClayImage     *image,
                                                 GType          object_type,
                                                 ClayUndoType   undo_type);
