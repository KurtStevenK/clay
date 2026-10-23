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


ClayObject  * clay_edit_cut                (ClayImage       *image,
                                            GList           *drawables,
                                            ClayContext     *context,
                                            GError         **error);
ClayObject  * clay_edit_copy               (ClayImage       *image,
                                            GList           *drawables,
                                            ClayContext     *context,
                                            gboolean         copy_for_cut,
                                            GError         **error);
ClayBuffer  * clay_edit_copy_visible       (ClayImage       *image,
                                            ClayContext     *context,
                                            GError         **error);

GList       * clay_edit_paste              (ClayImage       *image,
                                            GList           *drawables,
                                            ClayObject      *paste,
                                            ClayPasteType    paste_type,
                                            ClayContext     *context,
                                            gboolean         merged,
                                            gint             viewport_x,
                                            gint             viewport_y,
                                            gint             viewport_width,
                                            gint             viewport_height);
ClayImage   * clay_edit_paste_as_new_image (Clay            *clay,
                                            ClayObject      *paste,
                                            ClayContext     *context);

const gchar * clay_edit_named_cut          (ClayImage       *image,
                                            const gchar     *name,
                                            GList           *drawables,
                                            ClayContext     *context,
                                            GError         **error);
const gchar * clay_edit_named_copy         (ClayImage       *image,
                                            const gchar     *name,
                                            GList           *drawables,
                                            ClayContext     *context,
                                            GError         **error);
const gchar * clay_edit_named_copy_visible (ClayImage       *image,
                                            const gchar     *name,
                                            ClayContext     *context,
                                            GError         **error);
