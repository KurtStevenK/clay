/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaymath/claymath.h"

#include "text-types.h"

#include "core/clay-transform-utils.h"
#include "core/clayimage-undo.h"
#include "core/clayrasterizable.h"

#include "claytext.h"
#include "claytextlayer.h"
#include "claytextlayer-transform.h"


static ClayItemClass * clay_text_layer_parent_class (void) G_GNUC_CONST;

static gboolean  clay_text_layer_get_transformation  (ClayTextLayer *layer,
                                                      ClayMatrix3   *matrix);
static gboolean  clay_text_layer_set_transformation  (ClayTextLayer *layer,
                                                      ClayMatrix3   *matrix);

static gboolean  clay_text_layer_compose             (ClayTextLayer     *layer,
                                                      const ClayMatrix3 *incremental,
                                                      ClayMatrix3       *composed,
                                                      gdouble           *center_x,
                                                      gdouble           *center_y);
static void      clay_text_layer_commit              (ClayTextLayer     *layer,
                                                      ClayMatrix3       *composed,
                                                      gdouble            center_x,
                                                      gdouble            center_y,
                                                      gboolean           push_undo);


/*  The three non-destructive paths -- flip, rotate and transform -- all do the
 *  same three things, in an order that matters:
 *
 *    1. clay_text_layer_compose() decides whether the operation can be stored
 *       at all, and works out where the layer's centre ends up.  It mutates
 *       nothing, so callers can still fall back to the destructive path.
 *    2. the caller transforms the layer mask, if there is one.  This has to
 *       happen *before* the matrix is stored: storing re-renders, and
 *       clay_text_layer_render() resizes the mask to the layer's new size, so
 *       transforming afterwards double-applies.
 *    3. clay_text_layer_commit() stores the matrix and re-centres the layer,
 *       which also re-syncs the mask's offset to the layer's.
 */

static gboolean
clay_text_layer_compose (ClayTextLayer     *layer,
                         const ClayMatrix3 *incremental,
                         ClayMatrix3       *composed,
                         gdouble           *center_x,
                         gdouble           *center_y)
{
  ClayItem *item = CLAY_ITEM (layer);
  gdouble   rx;
  gdouble   ry;
  gint      ox;
  gint      oy;

  if (! clay_text_layer_get_transformation (layer, composed))
    return FALSE;

  /*  The translation column holds the sub-pixel remainder left over by the
   *  last re-centring -- see clay_text_layer_commit().  It has no rendering
   *  effect, so this is the only place it means anything: added back here,
   *  the operation pivots about where the layer really is instead of where
   *  it was last rounded to, and the error stays bounded at half a pixel
   *  rather than compounding over a sequence of transforms.
   */
  rx = composed->coeff[0][2];
  ry = composed->coeff[1][2];

  /*  where the centre of the layer ends up in image space.  Sampled before
   *  anything is stored, because storing re-renders and resizes the layer.
   */
  clay_item_get_offset (item, &ox, &oy);
  *center_x = ox + rx + clay_item_get_width  (item) / 2.0;
  *center_y = oy + ry + clay_item_get_height (item) / 2.0;
  clay_matrix3_transform_point (incremental,
                                *center_x, *center_y, center_x, center_y);

  /*  composed := incremental * composed  */
  clay_matrix3_mult (incremental, composed);

  if (! clay_matrix3_is_affine (composed))
    return FALSE;

  /*  The translation column never reaches the renderer --
   *  clay_text_layout_get_transform() hardcodes x0/y0 to zero -- so keep it
   *  out of the stored matrix and honour it by moving the item instead.
   */
  composed->coeff[0][2] = 0.0;
  composed->coeff[1][2] = 0.0;

  return TRUE;
}

static void
clay_text_layer_commit (ClayTextLayer *layer,
                        ClayMatrix3   *composed,
                        gdouble        center_x,
                        gdouble        center_y,
                        gboolean       push_undo)
{
  ClayItem *item = CLAY_ITEM (layer);
  gdouble   want_x;
  gdouble   want_y;
  gint      ox;
  gint      oy;
  gint      nx;
  gint      ny;

  clay_text_layer_set_transformation (layer, composed);

  /*  set_transformation() re-rendered synchronously, so the item is already
   *  at its new size; re-centre it on the transformed centre.
   *
   *  SIGNED_ROUND, not ROUND: a transformed layer routinely lands at a
   *  negative offset, and ROUND() truncates towards zero there.
   */
  clay_item_get_offset (item, &ox, &oy);
  want_x = center_x - clay_item_get_width  (item) / 2.0;
  want_y = center_y - clay_item_get_height (item) / 2.0;
  nx     = SIGNED_ROUND (want_x);
  ny     = SIGNED_ROUND (want_y);

  clay_item_translate (item, nx - ox, ny - oy, push_undo);

  /*  Keep what the rounding threw away.  Item offsets are integers, so a
   *  layer can only ever sit on a whole pixel; without this the remainder is
   *  lost at every step and a sequence of transforms walks the layer across
   *  the canvas.  clay_text_layer_compose() adds it back next time.
   *
   *  This is a second property set, and so a second render, but only on the
   *  committed transform rather than on each motion event.
   */
  if (want_x != nx || want_y != ny)
    clay_text_layer_set (layer, NULL,
                         "offset-x", want_x - nx,
                         "offset-y", want_y - ny,
                         NULL);
}


void
clay_text_layer_scale (ClayItem               *item,
                       gint                    new_width,
                       gint                    new_height,
                       gint                    new_offset_x,
                       gint                    new_offset_y,
                       ClayInterpolationType   interpolation_type,
                       ClayProgress           *progress)
{
  ClayTextLayer *layer  = CLAY_TEXT_LAYER (item);
  ClayLayerMask *mask   = clay_layer_get_mask (CLAY_LAYER (layer));
  ClayMatrix3    current;
  ClayMatrix3    scale;
  gint           width  = clay_item_get_width  (item);
  gint           height = clay_item_get_height (item);
  gint           ox;
  gint           oy;

  if ((width <= 0     || height <= 0)     ||
      (new_width <= 0 || new_height <= 0) ||
      ! clay_text_layer_get_transformation (layer, &current))
    goto destructive;

  clay_matrix3_identity (&scale);
  clay_matrix3_scale (&scale,
                      (gdouble) new_width  / (gdouble) width,
                      (gdouble) new_height / (gdouble) height);

  /*  current := scale * current  */
  clay_matrix3_mult (&scale, &current);

  if (! clay_matrix3_is_affine (&current))
    goto destructive;

  current.coeff[0][2] = 0.0;
  current.coeff[1][2] = 0.0;

  /*  The mask has to be scaled before the matrix is stored.  Storing it
   *  re-renders, and clay_text_layer_render() resizes the mask to the
   *  layer's new size; scaling afterwards would either double-apply or
   *  operate on an already-reconciled mask.
   */
  if (mask)
    clay_item_scale (CLAY_ITEM (mask), new_width, new_height,
                     new_offset_x, new_offset_y,
                     interpolation_type, progress);

  if (! clay_text_layer_set_transformation (layer, &current))
    goto destructive;

  /*  the glyphs are rendered through the matrix, so the layer lands at
   *  the requested origin even if its bounding box rounds differently
   *  from the requested size.
   *
   *  This must be undoable: the stored matrix and the item offset are
   *  restored by separate undo steps, and clay_item_scale() has already
   *  opened a group to hold both.  Translating the layer also re-syncs
   *  the mask's offset to it.
   */
  clay_item_get_offset (item, &ox, &oy);
  clay_item_translate (item, new_offset_x - ox, new_offset_y - oy,
                       clay_item_is_attached (item));
  return;

 destructive:
  clay_text_layer_parent_class ()->scale (item,
                                          new_width, new_height,
                                          new_offset_x, new_offset_y,
                                          interpolation_type, progress);
}

void
clay_text_layer_flip (ClayItem            *item,
                      ClayContext         *context,
                      ClayOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (item);
  ClayLayerMask *mask  = clay_layer_get_mask (CLAY_LAYER (layer));
  ClayMatrix3    incremental;
  ClayMatrix3    composed;
  gdouble        center_x;
  gdouble        center_y;

  clay_matrix3_identity (&incremental);
  clay_transform_matrix_flip (&incremental, flip_type, axis);

  if (clay_text_layer_compose (layer, &incremental, &composed,
                               &center_x, &center_y))
    {
      if (mask)
        clay_item_flip (CLAY_ITEM (mask), context,
                        flip_type, axis, clip_result);

      clay_text_layer_commit (layer, &composed, center_x, center_y,
                              clay_item_is_attached (item));
    }
  else
    {
      clay_text_layer_parent_class ()->flip (item, context,
                                             flip_type, axis, clip_result);
    }
}

void
clay_text_layer_rotate (ClayItem         *item,
                        ClayContext      *context,
                        ClayRotationType  rotate_type,
                        gdouble           center_x,
                        gdouble           center_y,
                        gboolean          clip_result)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (item);
  ClayLayerMask *mask  = clay_layer_get_mask (CLAY_LAYER (layer));
  ClayMatrix3    incremental;
  ClayMatrix3    composed;
  gdouble        new_center_x;
  gdouble        new_center_y;

  clay_matrix3_identity (&incremental);
  clay_transform_matrix_rotate (&incremental, rotate_type,
                                center_x, center_y);

  if (clay_text_layer_compose (layer, &incremental, &composed,
                               &new_center_x, &new_center_y))
    {
      if (mask)
        clay_item_rotate (CLAY_ITEM (mask), context,
                          rotate_type, center_x, center_y, clip_result);

      clay_text_layer_commit (layer, &composed, new_center_x, new_center_y,
                              clay_item_is_attached (item));
    }
  else
    {
      clay_text_layer_parent_class ()->rotate (item, context,
                                               rotate_type, center_x, center_y,
                                               clip_result);
    }
}

void
clay_text_layer_transform (ClayItem               *item,
                           ClayContext            *context,
                           const ClayMatrix3      *matrix,
                           ClayTransformDirection  direction,
                           ClayInterpolationType   interpolation_type,
                           ClayTransformResize     clip_result,
                           ClayProgress           *progress,
                           gboolean                push_undo)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (item);
  ClayLayerMask *mask  = clay_layer_get_mask (CLAY_LAYER (layer));
  ClayMatrix3    incremental;
  ClayMatrix3    composed;
  gdouble        center_x;
  gdouble        center_y;

  if (clay_matrix3_is_identity (matrix))
    return;

  /*  Clipping depends on the intermediate result of each step, which a
   *  single composed matrix cannot reproduce, so only "adjust" transforms
   *  can be stored non-destructively.  Same reasoning as
   *  clay_link_layer_transform().
   */
  if (clip_result == CLAY_TRANSFORM_RESIZE_ADJUST)
    {
      incremental = *matrix;

      if (direction == CLAY_TRANSFORM_BACKWARD)
        clay_matrix3_invert (&incremental);

      if (clay_text_layer_compose (layer, &incremental, &composed,
                                   &center_x, &center_y))
        {
          if (mask)
            clay_item_transform (CLAY_ITEM (mask), context, matrix, direction,
                                 interpolation_type, clip_result, progress);

          clay_text_layer_commit (layer, &composed, center_x, center_y,
                                  push_undo);
          return;
        }
    }

  clay_text_layer_parent_class ()->transform (item, context,
                                              matrix, direction,
                                              interpolation_type, clip_result,
                                              progress, push_undo);
}

static ClayItemClass *
clay_text_layer_parent_class (void)
{
  static ClayItemClass *parent_class = NULL;

  if (! parent_class)
    {
      gpointer klass = g_type_class_peek (CLAY_TYPE_TEXT_LAYER);

      parent_class = g_type_class_peek_parent (klass);
    }

  return parent_class;
}

static gboolean
clay_text_layer_get_transformation (ClayTextLayer *layer,
                                    ClayMatrix3   *matrix)
{
  if (! layer->text || clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    return FALSE;

  clay_text_get_transformation (layer->text, matrix);

  return TRUE;
}

static gboolean
clay_text_layer_set_transformation (ClayTextLayer *layer,
                                    ClayMatrix3   *matrix)
{
  ClayMatrix2  trafo;

  if (! clay_matrix3_is_affine (matrix))
    return FALSE;

  trafo.coeff[0][0] = matrix->coeff[0][0];
  trafo.coeff[0][1] = matrix->coeff[0][1];
  trafo.coeff[1][0] = matrix->coeff[1][0];
  trafo.coeff[1][1] = matrix->coeff[1][1];

  clay_text_layer_set (CLAY_TEXT_LAYER (layer), NULL,
                       "transformation", &trafo,
                       "offset-x",       matrix->coeff[0][2],
                       "offset-y",       matrix->coeff[1][2],
                       NULL);

  return TRUE;
}
