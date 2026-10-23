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

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "clay-utils.h"
#include "claybezierdesc.h"
#include "claychannel.h"
#include "claydrawable-fill.h"
#include "clayerror.h"
#include "clayfilloptions.h"
#include "clayimage.h"
#include "claypattern.h"
#include "claypickable.h"
#include "clayscanconvert.h"

#include "path/claypath.h"

#include "clay-intl.h"


/*  public functions  */

void
clay_drawable_fill (ClayDrawable *drawable,
                    ClayContext  *context,
                    ClayFillType  fill_type)
{
  GeglColor   *color;
  ClayPattern *pattern;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  if (fill_type == CLAY_FILL_TRANSPARENT &&
      ! clay_drawable_has_alpha (drawable))
    {
      fill_type = CLAY_FILL_BACKGROUND;
    }

  if (! clay_get_fill_params (context, fill_type, &color, &pattern, NULL))
    return;

  clay_drawable_fill_buffer (drawable,
                             clay_drawable_get_buffer (drawable),
                             color, pattern, 0, 0);

  clay_drawable_update (drawable, 0, 0, -1, -1);

  g_clear_object (&color);
}

void
clay_drawable_fill_buffer (ClayDrawable  *drawable,
                           GeglBuffer    *buffer,
                           GeglColor     *color,
                           ClayPattern   *pattern,
                           gint           pattern_offset_x,
                           gint           pattern_offset_y)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (GEGL_IS_COLOR (color) || pattern != NULL);
  g_return_if_fail (pattern == NULL || CLAY_IS_PATTERN (pattern));

  if (pattern)
    {
      GeglBuffer       *src_buffer;
      GeglBuffer       *dest_buffer;
      ClayColorProfile *src_profile;
      ClayColorProfile *dest_profile;

      src_buffer = clay_pattern_create_buffer (pattern);

      src_profile  = clay_babl_format_get_color_profile (
                       gegl_buffer_get_format (src_buffer));
      dest_profile = clay_color_managed_get_color_profile (
                       CLAY_COLOR_MANAGED (drawable));

      if (clay_color_transform_can_gegl_copy (src_profile, dest_profile))
        {
          dest_buffer = g_object_ref (src_buffer);
        }
      else
        {
          dest_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                         gegl_buffer_get_format (buffer));

          clay_gegl_convert_color_profile (
            src_buffer,  NULL, src_profile,
            dest_buffer, NULL, dest_profile,
            CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL,
            TRUE,
            NULL);
        }

      g_object_unref (src_profile);

      gegl_buffer_set_pattern (buffer, NULL, dest_buffer,
                               pattern_offset_x, pattern_offset_y);

      g_object_unref (src_buffer);
      g_object_unref (dest_buffer);
    }
  else
    {
      if (! clay_drawable_has_alpha (drawable))
        {
          color = gegl_color_duplicate (color);
          clay_color_set_alpha (color, 1.0);
        }

      gegl_buffer_set_color (buffer, NULL, color);

      if (! clay_drawable_has_alpha (drawable))
        g_object_unref (color);
    }
}

void
clay_drawable_fill_boundary (ClayDrawable       *drawable,
                             ClayFillOptions    *options,
                             const ClayBoundSeg *bound_segs,
                             gint                n_bound_segs,
                             gint                offset_x,
                             gint                offset_y,
                             gboolean            push_undo)
{
  ClayScanConvert *scan_convert;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (clay_fill_options_get_style (options) !=
                    CLAY_FILL_STYLE_PATTERN ||
                    clay_context_get_pattern (CLAY_CONTEXT (options)) != NULL);

  scan_convert = clay_scan_convert_new_from_boundary (bound_segs, n_bound_segs,
                                                      offset_x, offset_y);

  if (scan_convert)
    {
      clay_drawable_fill_scan_convert (drawable, options,
                                       scan_convert, push_undo);
      clay_scan_convert_free (scan_convert);
    }
}

gboolean
clay_drawable_fill_path (ClayDrawable     *drawable,
                         ClayFillOptions  *options,
                         ClayPath         *path,
                         gboolean          push_undo,
                         GError          **error)
{
  const ClayBezierDesc *bezier;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), FALSE);
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (CLAY_IS_PATH (path), FALSE);
  g_return_val_if_fail (clay_fill_options_get_style (options) !=
                        CLAY_FILL_STYLE_PATTERN ||
                        clay_context_get_pattern (CLAY_CONTEXT (options)) != NULL,
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bezier = clay_path_get_bezier (path);

  if (bezier && bezier->num_data > 4)
    {
      ClayScanConvert *scan_convert = clay_scan_convert_new ();

      clay_scan_convert_add_bezier (scan_convert, bezier);
      clay_drawable_fill_scan_convert (drawable, options,
                                       scan_convert, push_undo);

      clay_scan_convert_free (scan_convert);

      return TRUE;
    }

  g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                       _("Not enough points to fill"));

  return FALSE;
}

void
clay_drawable_fill_scan_convert (ClayDrawable    *drawable,
                                 ClayFillOptions *options,
                                 ClayScanConvert *scan_convert,
                                 gboolean         push_undo)
{
  ClayContext *context;
  GeglBuffer  *buffer;
  GeglBuffer  *mask_buffer;
  gint         x, y, w, h;
  gint         off_x;
  gint         off_y;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));
  g_return_if_fail (scan_convert != NULL);
  g_return_if_fail (clay_fill_options_get_style (options) !=
                    CLAY_FILL_STYLE_PATTERN ||
                    clay_context_get_pattern (CLAY_CONTEXT (options)) != NULL);

  context = CLAY_CONTEXT (options);

  if (! clay_item_mask_intersect (CLAY_ITEM (drawable), &x, &y, &w, &h))
    return;

  /* fill a 1-bpp GeglBuffer with black, this will describe the shape
   * of the stroke.
   */
  mask_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, w, h),
                                 babl_format ("Y u8"));

  /* render the stroke into it */
  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

  clay_scan_convert_render (scan_convert, mask_buffer,
                            x + off_x, y + off_y,
                            clay_fill_options_get_antialias (options));

  buffer = clay_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0, w, h),
                                            -x, -y);

  clay_gegl_apply_opacity (buffer, NULL, NULL, buffer,
                           mask_buffer, 0, 0, 1.0);
  g_object_unref (mask_buffer);

  /* Apply to drawable */
  clay_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (0, 0, w, h),
                              push_undo, C_("undo-type", "Render Stroke"),
                              clay_context_get_opacity (context),
                              clay_context_get_paint_mode (context),
                              CLAY_LAYER_COLOR_SPACE_AUTO,
                              CLAY_LAYER_COLOR_SPACE_AUTO,
                              clay_layer_mode_get_paint_composite_mode (
                                clay_context_get_paint_mode (context)),
                              NULL, x, y);

  g_object_unref (buffer);

  clay_drawable_update (drawable, x, y, w, h);
}
