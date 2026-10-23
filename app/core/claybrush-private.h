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


struct _ClayBrushPrivate
{
  ClayTempBuf     *mask;           /*  the actual mask                    */
  ClayTempBuf     *blurred_mask;    /*  blurred actual mask cached          */
  ClayTempBuf     *pixmap;         /*  optional pixmap data               */
  ClayTempBuf     *blurred_pixmap;  /*  optional pixmap data blurred cache  */

  gdouble          blur_hardness;

  gint             n_horz_mipmaps;
  gint             n_vert_mipmaps;
  ClayTempBuf    **mask_mipmaps;
  ClayTempBuf    **pixmap_mipmaps;

  gint             spacing;    /*  brush's spacing                */
  ClayVector2      x_axis;     /*  for calculating brush spacing  */
  ClayVector2      y_axis;     /*  for calculating brush spacing  */

  gint             use_count;  /*  for keeping the caches alive   */
  ClayBrushCache  *mask_cache;
  ClayBrushCache  *pixmap_cache;
  ClayBrushCache  *boundary_cache;
};
