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

#ifndef  __CLAY_BOUNDARY_H__
#define  __CLAY_BOUNDARY_H__


/* half intensity for mask */
#define CLAY_BOUNDARY_HALF_WAY 0.5


typedef enum
{
  CLAY_BOUNDARY_WITHIN_BOUNDS,
  CLAY_BOUNDARY_IGNORE_BOUNDS
} ClayBoundaryType;


struct _ClayBoundSeg
{
  gint   x1;
  gint   y1;
  gint   x2;
  gint   y2;
  guint  open    : 1;
  guint  visited : 1;
};


ClayBoundSeg * clay_boundary_find      (GeglBuffer          *buffer,
                                        const GeglRectangle *region,
                                        const Babl          *format,
                                        ClayBoundaryType     type,
                                        gint                 x1,
                                        gint                 y1,
                                        gint                 x2,
                                        gint                 y2,
                                        gfloat               threshold,
                                        gint                *num_segs);
ClayBoundSeg * clay_boundary_sort      (const ClayBoundSeg  *segs,
                                        gint                 num_segs,
                                        gint                *num_groups);
ClayBoundSeg * clay_boundary_simplify  (ClayBoundSeg        *sorted_segs,
                                        gint                 num_groups,
                                        gint                *num_segs);

/* offsets in-place */
void       clay_boundary_offset        (ClayBoundSeg        *segs,
                                        gint                 num_segs,
                                        gint                 off_x,
                                        gint                 off_y);


#endif  /*  __CLAY_BOUNDARY_H__  */
