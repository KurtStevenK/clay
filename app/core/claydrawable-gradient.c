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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "clay.h"
#include "claychannel.h"
#include "claycontext.h"
#include "claydrawable-gradient.h"
#include "claygradient.h"
#include "clayimage.h"
#include "clayprogress.h"

#include "clay-intl.h"


/*  public functions  */

void
clay_drawable_gradient (ClayDrawable                *drawable,
                        ClayContext                 *context,
                        ClayGradient                *gradient,
                        GeglDistanceMetric           metric,
                        ClayLayerMode                paint_mode,
                        ClayGradientType             gradient_type,
                        gdouble                      opacity,
                        gdouble                      offset,
                        ClayRepeatMode               repeat,
                        gboolean                     reverse,
                        ClayGradientBlendColorSpace  blend_color_space,
                        gboolean                     supersample,
                        gint                         max_depth,
                        gdouble                      threshold,
                        gboolean                     dither,
                        gdouble                      startx,
                        gdouble                      starty,
                        gdouble                      endx,
                        gdouble                      endy,
                        ClayProgress                *progress)
{
  ClayImage  *image;
  GeglBuffer *buffer;
  GeglBuffer *shapeburst = NULL;
  GeglNode   *render;
  gint        x, y, width, height;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (CLAY_IS_GRADIENT (gradient));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  image = clay_item_get_image (CLAY_ITEM (drawable));

  if (! clay_item_mask_intersect (CLAY_ITEM (drawable), &x, &y, &width, &height))
    return;

  clay_set_busy (image->clay);

  /*  Always create an alpha temp buf (for generality) */
  buffer = gegl_buffer_new (GEGL_RECTANGLE (x, y, width, height),
                            clay_drawable_get_format_with_alpha (drawable));

  if (gradient_type >= CLAY_GRADIENT_SHAPEBURST_ANGULAR &&
      gradient_type <= CLAY_GRADIENT_SHAPEBURST_DIMPLED)
    {
      shapeburst =
        clay_drawable_gradient_shapeburst_distmap (drawable, metric,
                                                   GEGL_RECTANGLE (x, y, width, height),
                                                   progress);
    }

  clay_drawable_gradient_adjust_coords (drawable,
                                        gradient_type,
                                        GEGL_RECTANGLE (x, y, width, height),
                                        &startx, &starty, &endx, &endy);

  render = gegl_node_new_child (NULL,
                                "operation",                  "clay:gradient",
                                "context",                    context,
                                "gradient",                   gradient,
                                "start-x",                    startx,
                                "start-y",                    starty,
                                "end-x",                      endx,
                                "end-y",                      endy,
                                "gradient-type",              gradient_type,
                                "gradient-repeat",            repeat,
                                "offset",                     offset,
                                "gradient-reverse",           reverse,
                                "gradient-blend-color-space", blend_color_space,
                                "supersample",                supersample,
                                "supersample-depth",          max_depth,
                                "supersample-threshold",      threshold,
                                "dither",                     dither,
                                NULL);

  clay_gegl_apply_operation (shapeburst, progress, C_("undo-type", "Gradient"),
                             render,
                             buffer, GEGL_RECTANGLE (x, y, width, height),
                             FALSE);

  g_object_unref (render);

  if (shapeburst)
    g_object_unref (shapeburst);

  clay_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (x, y, width, height),
                              TRUE, C_("undo-type", "Gradient"),
                              opacity, paint_mode,
                              CLAY_LAYER_COLOR_SPACE_AUTO,
                              CLAY_LAYER_COLOR_SPACE_AUTO,
                              clay_layer_mode_get_paint_composite_mode (paint_mode),
                              NULL, x, y);

  clay_drawable_update (drawable, x, y, width, height);

  g_object_unref (buffer);

  clay_unset_busy (image->clay);
}

GeglBuffer *
clay_drawable_gradient_shapeburst_distmap (ClayDrawable        *drawable,
                                           GeglDistanceMetric   metric,
                                           const GeglRectangle *region,
                                           ClayProgress        *progress)
{
  ClayChannel *mask;
  ClayImage   *image;
  GeglBuffer  *dist_buffer;
  GeglBuffer  *temp_buffer;
  GeglNode    *shapeburst;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);

  image = clay_item_get_image (CLAY_ITEM (drawable));

  /*  allocate the distance map  */
  dist_buffer = gegl_buffer_new (region, babl_format ("Y float"));

  /*  allocate the selection mask copy  */
  temp_buffer = gegl_buffer_new (region, babl_format ("Y float"));

  mask = clay_image_get_mask (image);

  /*  If the image mask is not empty, use it as the shape burst source  */
  if (! clay_channel_is_empty (mask))
    {
      gint x, y, width, height;
      gint off_x, off_y;

      clay_item_mask_intersect (CLAY_ITEM (drawable), &x, &y, &width, &height);
      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      /*  copy the mask to the temp mask  */
      clay_gegl_buffer_copy (
        clay_drawable_get_buffer (CLAY_DRAWABLE (mask)),
        GEGL_RECTANGLE (x + off_x, y + off_y, width, height),
        GEGL_ABYSS_NONE, temp_buffer, region);
    }
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (clay_drawable_has_alpha (drawable))
        {
          const Babl *component_format;

          component_format = babl_format ("A float");

          /*  extract the alpha into the temp mask  */
          gegl_buffer_set_format (temp_buffer, component_format);
          clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), region,
                                 GEGL_ABYSS_NONE,
                                 temp_buffer, region);
          gegl_buffer_set_format (temp_buffer, NULL);
        }
      else
        {
          GeglColor *white = gegl_color_new ("white");

          /*  Otherwise, just fill the shapeburst to white  */
          gegl_buffer_set_color (temp_buffer, NULL, white);
          g_object_unref (white);
        }
    }

  shapeburst = gegl_node_new_child (NULL,
                                    "operation", "gegl:distance-transform",
                                    "normalize", TRUE,
                                    "metric",    metric,
                                    NULL);

  if (progress)
    clay_gegl_progress_connect (shapeburst, progress,
                                _("Calculating distance map"));

  clay_gegl_apply_operation (temp_buffer, NULL, NULL,
                             shapeburst,
                             dist_buffer, region, FALSE);

  g_object_unref (shapeburst);

  g_object_unref (temp_buffer);

  return dist_buffer;
}

void
clay_drawable_gradient_adjust_coords (ClayDrawable        *drawable,
                                      ClayGradientType     gradient_type,
                                      const GeglRectangle *region,
                                      gdouble             *startx,
                                      gdouble             *starty,
                                      gdouble             *endx,
                                      gdouble             *endy)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (region != NULL);
  g_return_if_fail (startx != NULL);
  g_return_if_fail (starty != NULL);
  g_return_if_fail (endx != NULL);
  g_return_if_fail (endy != NULL);

  /* we potentially adjust the gradient coordinates according to the gradient
   * type, so that in cases where the gradient span is not related to the
   * segment length, the gradient cache (in ClayOperationGradient) is big
   * enough not to produce banding.
   */

  switch (gradient_type)
    {
    /* for conical gradients, use a segment with the original origin and
     * direction, whose length is the circumference of the largest circle
     * centered at the origin, passing through one of the regions's vertices.
     */
    case CLAY_GRADIENT_CONICAL_SYMMETRIC:
    case CLAY_GRADIENT_CONICAL_ASYMMETRIC:
      {
        gdouble     r = 0.0;
        ClayVector2 v;

        r = MAX (r, hypot (region->x - *startx,
                           region->y - *starty));
        r = MAX (r, hypot (region->x + region->width - *startx,
                           region->y - *starty));
        r = MAX (r, hypot (region->x - *startx,
                           region->y + region->height - *starty));
        r = MAX (r, hypot (region->x + region->width - *startx,
                           region->y + region->height - *starty));

        /* symmetric conical gradients only span half a revolution, and
         * therefore require only half the cache size.
         */
        if (gradient_type == CLAY_GRADIENT_CONICAL_SYMMETRIC)
          r /= 2.0;

        clay_vector2_set (&v, *endx - *startx, *endy - *starty);
        clay_vector2_normalize (&v);
        clay_vector2_mul (&v, 2.0 * G_PI * r);

        *endx = *startx + v.x;
        *endy = *starty + v.y;
      }
      break;

    /* for shaped gradients, only the segment's length matters; use the
     * regions's diagonal, which is the largest possible distance between two
     * points in the region.
     */
    case CLAY_GRADIENT_SHAPEBURST_ANGULAR:
    case CLAY_GRADIENT_SHAPEBURST_SPHERICAL:
    case CLAY_GRADIENT_SHAPEBURST_DIMPLED:
      *startx = region->x;
      *starty = region->y;
      *endx   = region->x + region->width;
      *endy   = region->y + region->height;
      break;

    default:
      break;
    }
}
