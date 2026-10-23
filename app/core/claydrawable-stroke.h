/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawable-stroke.h
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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


void       clay_drawable_stroke_boundary     (ClayDrawable       *drawable,
                                              ClayStrokeOptions  *options,
                                              const ClayBoundSeg *bound_segs,
                                              gint                n_bound_segs,
                                              gint                offset_x,
                                              gint                offset_y,
                                              gboolean            push_undo);

gboolean   clay_drawable_stroke_path         (ClayDrawable       *drawable,
                                              ClayStrokeOptions  *options,
                                              ClayPath           *path,
                                              gboolean            push_undo,
                                              GError            **error);

void       clay_drawable_stroke_scan_convert (ClayDrawable      *drawable,
                                              ClayStrokeOptions *options,
                                              ClayScanConvert   *scan_convert,
                                              gboolean           push_undo);
