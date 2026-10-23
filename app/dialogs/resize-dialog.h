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


typedef void (* ClayResizeCallback) (GtkWidget    *dialog,
                                     ClayViewable *viewable,
                                     ClayContext  *context,
                                     gint          width,
                                     gint          height,
                                     ClayUnit     *unit,
                                     gint          offset_x,
                                     gint          offset_y,
                                     gdouble       xres,
                                     gdouble       yres,
                                     ClayUnit     *res_unit,
                                     ClayFillType  fill_type,
                                     ClayItemSet   layer_set,
                                     gboolean      resize_text_layers,
                                     gpointer      user_data);


GtkWidget * resize_dialog_new (ClayViewable       *viewable,
                               ClayContext        *context,
                               const gchar        *title,
                               const gchar        *role,
                               GtkWidget          *parent,
                               ClayHelpFunc        help_func,
                               const gchar        *help_id,
                               ClayUnit           *unit,
                               ClayFillType        fill_type,
                               ClayItemSet         layer_set,
                               gboolean            resize_text_layers,
                               ClayResizeCallback  callback,
                               gpointer            user_data);
