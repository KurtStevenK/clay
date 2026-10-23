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


/*  Lowlevel API that is used for initializing entire drawables and
 *  buffers before they are used in images, they don't push an undo.
 */

void       clay_drawable_fill              (ClayDrawable        *drawable,
                                            ClayContext         *context,
                                            ClayFillType         fill_type);
void       clay_drawable_fill_buffer       (ClayDrawable        *drawable,
                                            GeglBuffer          *buffer,
                                            GeglColor           *color,
                                            ClayPattern         *pattern,
                                            gint                 pattern_offset_x,
                                            gint                 pattern_offset_y);


/*  Proper API that is used for actual editing (not just initializing)
 */

void       clay_drawable_fill_boundary     (ClayDrawable        *drawable,
                                            ClayFillOptions     *options,
                                            const ClayBoundSeg  *bound_segs,
                                            gint                 n_bound_segs,
                                            gint                 offset_x,
                                            gint                 offset_y,
                                            gboolean             push_undo);

gboolean   clay_drawable_fill_path         (ClayDrawable        *drawable,
                                            ClayFillOptions     *options,
                                            ClayPath            *path,
                                            gboolean             push_undo,
                                            GError             **error);

void       clay_drawable_fill_scan_convert (ClayDrawable        *drawable,
                                            ClayFillOptions     *options,
                                            ClayScanConvert     *scan_convert,
                                            gboolean             push_undo);
