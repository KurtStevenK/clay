/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claychunkiterator.h
 * Copyright (C) 2019 Ell
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


ClayChunkIterator * clay_chunk_iterator_new               (cairo_region_t      *region);

void                clay_chunk_iterator_set_tile_rect     (ClayChunkIterator   *iter,
                                                           const GeglRectangle *rect);

void                clay_chunk_iterator_set_priority_rect (ClayChunkIterator   *iter,
                                                           const GeglRectangle *rect);

void                clay_chunk_iterator_set_interval      (ClayChunkIterator   *iter,
                                                           gdouble              interval);

gboolean            clay_chunk_iterator_next              (ClayChunkIterator   *iter);
gboolean            clay_chunk_iterator_get_rect          (ClayChunkIterator   *iter,
                                                           GeglRectangle       *rect);

cairo_region_t    * clay_chunk_iterator_stop              (ClayChunkIterator   *iter,
                                                           gboolean             free_region);
