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

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "claybezierdesc.h"
#include "clayboundary.h"
#include "claybrush.h"
#include "claybrush-boundary.h"
#include "claytempbuf.h"


static ClayBezierDesc *
clay_brush_transform_boundary_exact (ClayBrush *brush,
                                     gdouble    scale,
                                     gdouble    aspect_ratio,
                                     gdouble    angle,
                                     gboolean   reflect,
                                     gdouble    hardness)
{
  const ClayTempBuf *mask;

  mask = clay_brush_transform_mask (brush,
                                    scale, aspect_ratio,
                                    angle, reflect, hardness);

  if (mask)
    {
      GeglBuffer    *buffer;
      ClayBoundSeg  *bound_segs;
      gint           n_bound_segs;

      buffer = clay_temp_buf_create_buffer ((ClayTempBuf *) mask);

      bound_segs = clay_boundary_find (buffer, NULL,
                                       babl_format ("Y float"),
                                       CLAY_BOUNDARY_WITHIN_BOUNDS,
                                       0, 0,
                                       gegl_buffer_get_width  (buffer),
                                       gegl_buffer_get_height (buffer),
                                       0.0,
                                       &n_bound_segs);

      g_object_unref (buffer);

      if (bound_segs)
        {
          ClayBoundSeg *stroke_segs;
          gint          n_stroke_groups;

          stroke_segs = clay_boundary_sort (bound_segs, n_bound_segs,
                                            &n_stroke_groups);

          g_free (bound_segs);

          if (stroke_segs)
            {
              ClayBezierDesc *path;

              path = clay_bezier_desc_new_from_bound_segs (stroke_segs,
                                                           n_bound_segs,
                                                           n_stroke_groups);

              g_free (stroke_segs);

              return path;
            }
        }
    }

  return NULL;
}

static ClayBezierDesc *
clay_brush_transform_boundary_approx (ClayBrush *brush,
                                      gdouble    scale,
                                      gdouble    aspect_ratio,
                                      gdouble    angle,
                                      gboolean   reflect,
                                      gdouble    hardness)
{
  return clay_brush_transform_boundary_exact (brush,
                                              scale, aspect_ratio,
                                              angle, reflect, hardness);
}

ClayBezierDesc *
clay_brush_real_transform_boundary (ClayBrush *brush,
                                    gdouble    scale,
                                    gdouble    aspect_ratio,
                                    gdouble    angle,
                                    gboolean   reflect,
                                    gdouble    hardness,
                                    gint      *width,
                                    gint      *height)
{
  clay_brush_transform_size (brush, scale, aspect_ratio, angle, reflect,
                             width, height);

  if (*width < 256 && *height < 256)
    {
      return clay_brush_transform_boundary_exact (brush,
                                                  scale, aspect_ratio,
                                                  angle, reflect, hardness);
    }

  return clay_brush_transform_boundary_approx (brush,
                                               scale, aspect_ratio,
                                               angle, reflect, hardness);
}
