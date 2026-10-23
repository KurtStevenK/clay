/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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


ClayTemplate * clay_image_new_get_last_template (Clay            *clay,
                                                 ClayImage       *image);
void           clay_image_new_set_last_template (Clay            *clay,
                                                 ClayTemplate    *template);

ClayImage    * clay_image_new_from_template     (Clay            *clay,
                                                 ClayTemplate    *template,
                                                 ClayContext     *context);
ClayImage    * clay_image_new_from_drawable     (Clay            *clay,
                                                 ClayDrawable    *drawable);
ClayImage    * clay_image_new_from_drawables    (Clay            *clay,
                                                 GList           *drawables,
                                                 gboolean         copy_selection,
                                                 gboolean         tag_copies);
ClayImage    * clay_image_new_from_component    (Clay            *clay,
                                                 ClayImage       *image,
                                                 ClayChannelType  component);
ClayImage    * clay_image_new_from_buffer       (Clay            *clay,
                                                 ClayBuffer      *buffer);
ClayImage    * clay_image_new_from_pixbuf       (Clay            *clay,
                                                 GdkPixbuf       *pixbuf,
                                                 const gchar     *layer_name);
