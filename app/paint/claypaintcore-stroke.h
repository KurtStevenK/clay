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


gboolean   clay_paint_core_stroke          (ClayPaintCore      *core,
                                            ClayDrawable       *drawable,
                                            ClayPaintOptions   *paint_options,
                                            ClayCoords         *strokes,
                                            gsize               n_strokes,
                                            gboolean            push_undo,
                                            GError            **error);
gboolean   clay_paint_core_stroke_boundary (ClayPaintCore      *core,
                                            ClayDrawable       *drawable,
                                            ClayPaintOptions   *paint_options,
                                            gboolean            emulate_dynamics,
                                            const ClayBoundSeg *bound_segs,
                                            gint                n_bound_segs,
                                            gint                offset_x,
                                            gint                offset_y,
                                            gboolean            push_undo,
                                            GError            **error);
gboolean   clay_paint_core_stroke_path     (ClayPaintCore      *core,
                                            ClayDrawable       *drawable,
                                            ClayPaintOptions   *paint_options,
                                            gboolean            emulate_dynamics,
                                            ClayPath           *path,
                                            gboolean            push_undo,
                                            GError            **error);
