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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claydialogconfig.h"

#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-mask.h"
#include "gegl/clay-gegl-mask-combine.h"
#include "gegl/clay-gegl-utils.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "clay.h"
#include "claychannel.h"
#include "claydrawable.h"
#include "claydrawable-bucket-fill.h"
#include "clayfilloptions.h"
#include "claystrokeoptions.h"
#include "clayimage.h"
#include "claylineart.h"
#include "claypickable.h"
#include "claypickable-contiguous-region.h"

#include "clay-intl.h"


/*  public functions  */

void
clay_drawable_bucket_fill (ClayDrawable         *drawable,
                           ClayFillOptions      *options,
                           gboolean              fill_transparent,
                           ClaySelectCriterion   fill_criterion,
                           gdouble               threshold,
                           gboolean              sample_merged,
                           gboolean              diagonal_neighbors,
                           gdouble               seed_x,
                           gdouble               seed_y)
{
  ClayImage  *image;
  GeglBuffer *buffer;
  gdouble     mask_x;
  gdouble     mask_y;
  gint        width, height;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  image = clay_item_get_image (CLAY_ITEM (drawable));

  clay_set_busy (image->clay);

  buffer = clay_drawable_get_bucket_fill_buffer (drawable, options,
                                                 fill_transparent, fill_criterion,
                                                 threshold, FALSE, sample_merged,
                                                 diagonal_neighbors,
                                                 seed_x, seed_y, NULL,
                                                 &mask_x, &mask_y, &width, &height);

  if (buffer)
    {
      /*  Apply it to the image  */
      clay_drawable_apply_buffer (drawable, buffer,
                                  GEGL_RECTANGLE (0, 0, width, height),
                                  TRUE, C_("undo-type", "Bucket Fill"),
                                  clay_context_get_opacity (CLAY_CONTEXT (options)),
                                  clay_context_get_paint_mode (CLAY_CONTEXT (options)),
                                  CLAY_LAYER_COLOR_SPACE_AUTO,
                                  CLAY_LAYER_COLOR_SPACE_AUTO,
                                  clay_layer_mode_get_paint_composite_mode
                                  (clay_context_get_paint_mode (CLAY_CONTEXT (options))),
                                  NULL, (gint) mask_x, mask_y);
      g_object_unref (buffer);

      clay_drawable_update (drawable, mask_x, mask_y, width, height);
    }

  clay_unset_busy (image->clay);
}

/**
 * clay_drawable_get_bucket_fill_buffer:
 * @drawable: the #ClayDrawable to edit.
 * @options:
 * @fill_transparent:
 * @fill_criterion:
 * @threshold:
 * @show_all:
 * @sample_merged:
 * @diagonal_neighbors:
 * @seed_x: X coordinate to start the fill.
 * @seed_y: Y coordinate to start the fill.
 * @mask_buffer: mask of the fill in-progress when in an interactive
 *               filling process. Set to NULL if you need a one-time
 *               fill.
 * @mask_x: returned x bound of @mask_buffer.
 * @mask_y: returned x bound of @mask_buffer.
 * @mask_width: returned width bound of @mask_buffer.
 * @mask_height: returned height bound of @mask_buffer.
 *
 * Creates the fill buffer for a bucket fill operation on @drawable,
 * without actually applying it (if you want to apply it directly as a
 * one-time operation, use clay_drawable_bucket_fill() instead). If
 * @mask_buffer is not NULL, the intermediate fill mask will also be
 * returned. This fill mask can later be reused in successive calls to
 * clay_drawable_get_bucket_fill_buffer() for interactive filling.
 *
 * Returns: a fill buffer which can be directly applied to @drawable, or
 *          used in a drawable filter as preview.
 */
GeglBuffer *
clay_drawable_get_bucket_fill_buffer (ClayDrawable         *drawable,
                                      ClayFillOptions      *options,
                                      gboolean              fill_transparent,
                                      ClaySelectCriterion   fill_criterion,
                                      gdouble               threshold,
                                      gboolean              show_all,
                                      gboolean              sample_merged,
                                      gboolean              diagonal_neighbors,
                                      gdouble               seed_x,
                                      gdouble               seed_y,
                                      GeglBuffer          **mask_buffer,
                                      gdouble              *mask_x,
                                      gdouble              *mask_y,
                                      gint                 *mask_width,
                                      gint                 *mask_height)
{
  ClayImage    *image;
  ClayPickable *pickable;
  GeglBuffer   *buffer;
  GeglBuffer   *new_mask;
  gboolean      antialias;
  gint          x, y, width, height;
  gint          mask_offset_x = 0;
  gint          mask_offset_y = 0;
  gint          sel_x, sel_y, sel_width, sel_height;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);

  image = clay_item_get_image (CLAY_ITEM (drawable));

  if (! clay_item_mask_intersect (CLAY_ITEM (drawable),
                                  &sel_x, &sel_y, &sel_width, &sel_height))
    return NULL;

  if (mask_buffer && *mask_buffer && threshold == 0.0)
    {
      gfloat pixel;

      gegl_buffer_sample (*mask_buffer, seed_x, seed_y, NULL, &pixel,
                          babl_format ("Y float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (pixel != 0.0)
        /* Already selected. This seed won't change the selection. */
        return NULL;
    }

  clay_set_busy (image->clay);

  if (sample_merged)
    {
      if (! show_all)
        pickable = CLAY_PICKABLE (image);
      else
        pickable = CLAY_PICKABLE (clay_image_get_projection (image));
    }
  else
    {
      pickable = CLAY_PICKABLE (drawable);
    }

  antialias = clay_fill_options_get_antialias (options);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  new_mask = clay_pickable_contiguous_region_by_seed (pickable,
                                                      antialias,
                                                      threshold,
                                                      fill_transparent,
                                                      fill_criterion,
                                                      diagonal_neighbors,
                                                      (gint) seed_x,
                                                      (gint) seed_y);
  if (mask_buffer && *mask_buffer)
    {
      clay_gegl_mask_combine_buffer (new_mask, *mask_buffer,
                                     CLAY_CHANNEL_OP_ADD, 0, 0);
      g_object_unref (*mask_buffer);
    }

  if (mask_buffer)
    *mask_buffer = new_mask;

  clay_gegl_mask_bounds (new_mask, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, intersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  clay_drawable_apply_buffer().
   */
  if (! clay_channel_is_empty (clay_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      if (! clay_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          /*  The fill region and the selection are disjoint; bail.  */

          if (! mask_buffer)
            g_object_unref (new_mask);

          clay_unset_busy (image->clay);

          return NULL;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      ClayItem *item = CLAY_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      clay_item_get_offset (item, &off_x, &off_y);

      clay_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                clay_item_get_width (item),
                                clay_item_get_height (item),

                                &x, &y, &width, &height);

      mask_offset_x = x;
      mask_offset_y = y;

      /*  translate mask bounds to drawable coords  */
      x -= off_x;
      y -= off_y;
    }
  else
    {
      mask_offset_x = x;
      mask_offset_y = y;
    }

  buffer = clay_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            -x, -y);

  clay_gegl_apply_opacity (buffer, NULL, NULL, buffer, new_mask,
                           -mask_offset_x, -mask_offset_y, 1.0);

  if (mask_x)
    *mask_x = x;
  if (mask_y)
    *mask_y = y;
  if (mask_width)
    *mask_width = width;
  if (mask_height)
    *mask_height = height;

  if (! mask_buffer)
    g_object_unref (new_mask);

  clay_unset_busy (image->clay);

  return buffer;
}

/**
 * clay_drawable_get_line_art_fill_buffer:
 * @drawable: the #ClayDrawable to edit.
 * @line_art: the #ClayLineArt computed as fill source.
 * @options: the #ClayFillOptions.
 * @sample_merged:
 * @fill_color_as_line_art: do we add pixels in @drawable filled with
 *                          fill color to the line art?
 * @fill_color_threshold: threshold value to determine fill color.
 * @seed_x: X coordinate to start the fill.
 * @seed_y: Y coordinate to start the fill.
 * @mask_buffer: mask of the fill in-progress when in an interactive
 *               filling process. Set to NULL if you need a one-time
 *               fill.
 * @mask_x: returned x bound of @mask_buffer.
 * @mask_y: returned y bound of @mask_buffer.
 * @mask_width: returned width bound of @mask_buffer.
 * @mask_height: returned height bound of @mask_buffer.
 *
 * Creates the fill buffer for a bucket fill operation on @drawable
 * based on @line_art and @options, without actually applying it.
 * If @mask_buffer is not NULL, the intermediate fill mask will also be
 * returned. This fill mask can later be reused in successive calls to
 * clay_drawable_get_line_art_fill_buffer() for interactive filling.
 *
 * The @fill_color_as_line_art option is a special feature where we
 * consider pixels in @drawable already in the fill color as part of the
 * line art. This is a post-process, i.e. that this is not taken into
 * account while @line_art is computed, making this a fast addition
 * processing allowing to close some area manually.
 *
 * Returns: a fill buffer which can be directly applied to @drawable, or
 *          used in a drawable filter as preview.
 */
GeglBuffer *
clay_drawable_get_line_art_fill_buffer (ClayDrawable      *drawable,
                                        ClayLineArt       *line_art,
                                        ClayFillOptions   *options,
                                        gboolean           sample_merged,
                                        gboolean           fill_color_as_line_art,
                                        gdouble            fill_color_threshold,
                                        gboolean           line_art_stroke,
                                        ClayStrokeOptions *stroke_options,
                                        gdouble            seed_x,
                                        gdouble            seed_y,
                                        GeglBuffer       **mask_buffer,
                                        gdouble           *mask_x,
                                        gdouble           *mask_y,
                                        gint              *mask_width,
                                        gint              *mask_height)
{
  ClayImage  *image;
  GeglBuffer *buffer;
  GeglBuffer *new_mask;
  GeglBuffer *rendered_mask;
  GeglBuffer *fill_buffer   = NULL;
  GeglColor  *fill_color    = NULL;
  gint        fill_offset_x = 0;
  gint        fill_offset_y = 0;
  gint        x, y, width, height;
  gint        mask_offset_x = 0;
  gint        mask_offset_y = 0;
  gint        sel_x, sel_y, sel_width, sel_height;
  gdouble     feather_radius;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);

  image = clay_item_get_image (CLAY_ITEM (drawable));

  if (! clay_item_mask_intersect (CLAY_ITEM (drawable),
                                  &sel_x, &sel_y, &sel_width, &sel_height))
    return NULL;

  if (mask_buffer && *mask_buffer)
    {
      gfloat pixel;

      gegl_buffer_sample (*mask_buffer, seed_x, seed_y, NULL, &pixel,
                          babl_format ("Y float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      if (pixel != 0.0)
        /* Already selected. This seed won't change the selection. */
        return NULL;
    }

  clay_set_busy (image->clay);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  if (fill_color_as_line_art)
    {
      ClayPickable *pickable = clay_line_art_get_input (line_art);

      /* This cannot be a pattern fill. */
      g_return_val_if_fail (clay_fill_options_get_style (options) != CLAY_FILL_STYLE_PATTERN,
                            NULL);
      /* Meaningful only in above/below layer cases. */
      g_return_val_if_fail (CLAY_IS_DRAWABLE (pickable), NULL);

      if (clay_fill_options_get_style (options) == CLAY_FILL_STYLE_FG_COLOR)
        fill_color = clay_context_get_foreground (CLAY_CONTEXT (options));
      else if (clay_fill_options_get_style (options) == CLAY_FILL_STYLE_BG_COLOR)
        fill_color = clay_context_get_background (CLAY_CONTEXT (options));

      g_return_val_if_fail (fill_color != NULL, NULL);

      fill_buffer   = clay_drawable_get_buffer (drawable);
      fill_offset_x = clay_item_get_offset_x (CLAY_ITEM (drawable)) -
                      clay_item_get_offset_x (CLAY_ITEM (pickable));
      fill_offset_y = clay_item_get_offset_y (CLAY_ITEM (drawable)) -
                      clay_item_get_offset_y (CLAY_ITEM (pickable));
    }
  new_mask = clay_pickable_contiguous_region_by_line_art (NULL, line_art,
                                                          fill_buffer,
                                                          fill_color,
                                                          fill_color_threshold,
                                                          fill_offset_x,
                                                          fill_offset_y,
                                                          (gint) seed_x,
                                                          (gint) seed_y);
  if (mask_buffer && *mask_buffer)
    {
      clay_gegl_mask_combine_buffer (new_mask, *mask_buffer,
                                     CLAY_CHANNEL_OP_ADD, 0, 0);
      g_object_unref (*mask_buffer);
    }
  if (mask_buffer)
    *mask_buffer = new_mask;

  rendered_mask = clay_gegl_buffer_dup (new_mask, NULL);
  if (clay_fill_options_get_feather (options, &feather_radius))
    {
      /* Feathering for the line art algorithm is not applied during
       * mask creation because we just want to apply it on the borders
       * of the mask at the end (since the mask can evolve, we don't
       * want to actually touch the mask, but only the intermediate
       * rendered results).
       */
      clay_gegl_apply_feather (rendered_mask, NULL, NULL, rendered_mask, NULL,
                               feather_radius, feather_radius, TRUE);
    }

  if (line_art_stroke)
    {
      /* Similarly to feathering, stroke only happens on the rendered
       * result, not on the returned mask.
       */
      ClayChannel       *channel;
      ClayChannel       *stroked;
      GList             *drawables;
      ClayContext       *context = clay_get_user_context (image->clay);
      GError            *error   = NULL;
      GeglColor         *white   = gegl_color_new ("white");

      context = clay_config_duplicate (CLAY_CONFIG (context));
      /* As we are stroking a mask, we need to set color to white. */
      clay_context_set_foreground (CLAY_CONTEXT (context), white);
      g_object_unref (white);

      channel = clay_channel_new_from_buffer (image, new_mask, NULL, NULL);
      stroked = clay_channel_new_from_buffer (image, rendered_mask, NULL, NULL);
      clay_image_add_hidden_item (image, CLAY_ITEM (channel));
      clay_image_add_hidden_item (image, CLAY_ITEM (stroked));
      drawables = g_list_prepend (NULL, stroked);

      if (! clay_item_stroke (CLAY_ITEM (channel), drawables,
                              context,
                              stroke_options,
                              NULL, FALSE, NULL, &error))
        {
          g_warning ("%s: stroking failed with: %s\n",
                     G_STRFUNC, error ? error->message : "no error message");
          g_clear_error (&error);
        }

      g_list_free (drawables);

      clay_pickable_flush (CLAY_PICKABLE (stroked));
      g_object_unref (rendered_mask);
      rendered_mask = clay_drawable_get_buffer (CLAY_DRAWABLE (stroked));
      g_object_ref (rendered_mask);

      clay_image_remove_hidden_item (image, CLAY_ITEM (channel));
      g_object_unref (channel);
      clay_image_remove_hidden_item (image, CLAY_ITEM (stroked));
      g_object_unref (stroked);

      g_object_unref (context);
    }

  clay_gegl_mask_bounds (rendered_mask, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, intersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  clay_drawable_apply_buffer().
   */
  if (! clay_channel_is_empty (clay_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      if (! clay_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          if (! mask_buffer)
            g_object_unref (new_mask);
          /*  The fill region and the selection are disjoint; bail.  */
          clay_unset_busy (image->clay);

          return NULL;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      ClayItem *item = CLAY_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      clay_item_get_offset (item, &off_x, &off_y);

      clay_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                clay_item_get_width (item),
                                clay_item_get_height (item),

                                &x, &y, &width, &height);

      mask_offset_x = x;
      mask_offset_y = y;

      /*  translate mask bounds to drawable coords  */
      x -= off_x;
      y -= off_y;
    }
  else
    {
      mask_offset_x = x;
      mask_offset_y = y;
    }

  buffer = clay_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height),
                                            -x, -y);

  clay_gegl_apply_opacity (buffer, NULL, NULL, buffer, rendered_mask,
                           -mask_offset_x, -mask_offset_y, 1.0);

  if (mask_x)
    *mask_x = x;
  if (mask_y)
    *mask_y = y;
  if (mask_width)
    *mask_width = width;
  if (mask_height)
    *mask_height = height;

  if (! mask_buffer)
    g_object_unref (new_mask);

  g_object_unref (rendered_mask);

  clay_unset_busy (image->clay);

  return buffer;
}
