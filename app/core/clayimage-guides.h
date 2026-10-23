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


/*  public guide adding API
 */
ClayGuide * clay_image_add_hguide     (ClayImage *image,
                                       gint       position,
                                       gboolean   push_undo);
ClayGuide * clay_image_add_vguide     (ClayImage *image,
                                       gint       position,
                                       gboolean   push_undo);

/*  internal guide adding API, does not check the guide's position and
 *  is publicly declared only to be used from undo
 */
void        clay_image_add_guide      (ClayImage *image,
                                       ClayGuide *guide,
                                       gint       position);

void        clay_image_remove_guide   (ClayImage *image,
                                       ClayGuide *guide,
                                       gboolean   push_undo);
void        clay_image_move_guide     (ClayImage *image,
                                       ClayGuide *guide,
                                       gint       position,
                                       gboolean   push_undo);

GList     * clay_image_get_guides     (ClayImage *image);
ClayGuide * clay_image_get_guide      (ClayImage *image,
                                       guint32    id);
ClayGuide * clay_image_get_next_guide (ClayImage *image,
                                       guint32    id,
                                       gboolean  *guide_found);
