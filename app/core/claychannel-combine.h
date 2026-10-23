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


void   clay_channel_combine_rect         (ClayChannel    *mask,
                                          ClayChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h);
void   clay_channel_combine_ellipse      (ClayChannel    *mask,
                                          ClayChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h,
                                          gboolean        antialias);
void   clay_channel_combine_ellipse_rect (ClayChannel    *mask,
                                          ClayChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h,
                                          gdouble         rx,
                                          gdouble         ry,
                                          gboolean        antialias);
void   clay_channel_combine_mask         (ClayChannel    *mask,
                                          ClayChannel    *add_on,
                                          ClayChannelOps  op,
                                          gint            off_x,
                                          gint            off_y);
void   clay_channel_combine_buffer       (ClayChannel    *mask,
                                          GeglBuffer     *add_on_buffer,
                                          ClayChannelOps  op,
                                          gint            off_x,
                                          gint            off_y);

void   clay_channel_combine_items        (ClayChannel    *mask,
                                          GList          *items,
                                          ClayChannelOps  op);
