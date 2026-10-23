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


/*
 *  virtual functions of ClayDrawable -- don't call directly
 */
ClayTempBuf * clay_drawable_get_new_preview       (ClayViewable *viewable,
                                                   ClayContext  *context,
                                                   gint          width,
                                                   gint          height,
                                                   gint          scale_factor,
                                                   GeglColor    *fg_color);
GdkPixbuf   * clay_drawable_get_new_pixbuf        (ClayViewable *viewable,
                                                   ClayContext  *context,
                                                   gint          width,
                                                   gint          height,
                                                   gint          scale_factor,
                                                   GeglColor    *fg_color);

/*
 *  normal functions (no virtuals)
 */
const Babl  * clay_drawable_get_preview_format    (ClayDrawable *drawable);

ClayTempBuf * clay_drawable_get_sub_preview       (ClayDrawable *drawable,
                                                   gint          src_x,
                                                   gint          src_y,
                                                   gint          src_width,
                                                   gint          src_height,
                                                   gint          dest_width,
                                                   gint          dest_height,
                                                   gint          dest_scale_factor);
GdkPixbuf   * clay_drawable_get_sub_pixbuf        (ClayDrawable *drawable,
                                                   gint          src_x,
                                                   gint          src_y,
                                                   gint          src_width,
                                                   gint          src_height,
                                                   gint          dest_width,
                                                   gint          dest_height,
                                                   gint          dest_scale_factor);

ClayAsync   * clay_drawable_get_sub_preview_async (ClayDrawable *drawable,
                                                   gint          src_x,
                                                   gint          src_y,
                                                   gint          src_width,
                                                   gint          src_height,
                                                   gint          dest_width,
                                                   gint          dest_height,
                                                   gint          dest_scale_factor);
