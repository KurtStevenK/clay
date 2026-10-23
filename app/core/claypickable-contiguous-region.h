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


GeglBuffer * clay_pickable_contiguous_region_by_seed                (ClayPickable        *pickable,
                                                                     gboolean             antialias,
                                                                     gfloat               threshold,
                                                                     gboolean             select_transparent,
                                                                     ClaySelectCriterion  select_criterion,
                                                                     gboolean             diagonal_neighbors,
                                                                     gint                 x,
                                                                     gint                 y);

GeglBuffer * clay_pickable_contiguous_region_by_color               (ClayPickable        *pickable,
                                                                     gboolean             antialias,
                                                                     gfloat               threshold,
                                                                     gboolean             select_transparent,
                                                                     ClaySelectCriterion  select_criterion,
                                                                     GeglColor           *color,
                                                                     const GeglRectangle *roi);

GeglBuffer * clay_pickable_contiguous_region_by_line_art            (ClayPickable        *pickable,
                                                                     ClayLineArt         *line_art,
                                                                     GeglBuffer          *fill_buffer,
                                                                     GeglColor           *fill_color,
                                                                     gfloat               fill_threshold,
                                                                     gint                 fill_offset_x,
                                                                     gint                 fill_offset_y,
                                                                     gint                 x,
                                                                     gint                 y);
