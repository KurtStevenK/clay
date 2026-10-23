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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "gegl/clay-gegl-loops.h"

#include "claychannel.h"
#include "core/claycontainer.h"
#include "claydrawable.h"
#include "claydrawable-edit.h"
#include "core/claydrawable-filters.h"
#include "claydrawablefilter.h"
#include "claycontext.h"
#include "clayfilloptions.h"
#include "clayimage.h"
#include "claypattern.h"
#include "claytempbuf.h"

#include "clay-intl.h"


/*  local function prototypes  */

static gboolean   clay_drawable_edit_can_fill_direct (ClayDrawable    *drawable,
                                                      ClayFillOptions *options);
static void       clay_drawable_edit_fill_direct     (ClayDrawable    *drawable,
                                                      ClayFillOptions *options,
                                                      const gchar     *undo_desc);


/*  private functions  */

static gboolean
clay_drawable_edit_can_fill_direct (ClayDrawable    *drawable,
                                    ClayFillOptions *options)
{
  ClayImage                *image;
  ClayContext              *context;
  gdouble                   opacity;
  ClayComponentMask         affect;
  ClayLayerMode             mode;
  ClayLayerCompositeMode    composite_mode;
  ClayLayerCompositeRegion  composite_region;

  image            = clay_item_get_image (CLAY_ITEM (drawable));
  context          = CLAY_CONTEXT (options);
  opacity          = clay_context_get_opacity (context);
  affect           = clay_drawable_get_active_mask (drawable);
  mode             = clay_context_get_paint_mode (context);
  composite_mode   = clay_layer_mode_get_paint_composite_mode (mode);
  composite_region = clay_layer_mode_get_included_region (mode, composite_mode);

  if (clay_channel_is_empty (clay_image_get_mask (image)) &&
      opacity == CLAY_OPACITY_OPAQUE                      &&
      affect  == CLAY_COMPONENT_MASK_ALL                  &&
      clay_layer_mode_is_trivial (mode)                   &&
      (! clay_layer_mode_is_subtractive (mode)            ^
       ! (composite_region & CLAY_LAYER_COMPOSITE_REGION_SOURCE)))
    {
      switch (clay_fill_options_get_style (options))
        {
        case CLAY_FILL_STYLE_FG_COLOR:
        case CLAY_FILL_STYLE_BG_COLOR:
          return TRUE;

        case CLAY_FILL_STYLE_PATTERN:
          {
            ClayPattern *pattern;
            ClayTempBuf *mask;
            const Babl  *format;

            pattern = clay_context_get_pattern (context);
            mask    = clay_pattern_get_mask (pattern);
            format  = clay_temp_buf_get_format (mask);

            return ! babl_format_has_alpha (format);
          }
        }
    }

  return FALSE;
}

static void
clay_drawable_edit_fill_direct (ClayDrawable    *drawable,
                                ClayFillOptions *options,
                                const gchar     *undo_desc)
{
  GeglBuffer    *buffer;
  ClayContext   *context;
  ClayLayerMode  mode;
  gint           width;
  gint           height;

  buffer  = clay_drawable_get_buffer (drawable);
  context = CLAY_CONTEXT (options);
  mode    = clay_context_get_paint_mode (context);
  width   = clay_item_get_width  (CLAY_ITEM (drawable));
  height  = clay_item_get_height (CLAY_ITEM (drawable));

  clay_drawable_push_undo (drawable, undo_desc,
                           NULL, 0, 0, width, height);

  if (! clay_layer_mode_is_subtractive (mode))
    clay_fill_options_fill_buffer (options, drawable, buffer, 0, 0);
  else
    clay_gegl_clear (buffer, NULL);
}


/*  public functions  */

void
clay_drawable_edit_clear (ClayDrawable *drawable,
                          ClayContext  *context)
{
  ClayFillOptions *options;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  options = clay_fill_options_new (context->clay, NULL, FALSE);

  if (clay_drawable_has_alpha (drawable))
    clay_fill_options_set_by_fill_type (options, context,
                                        CLAY_FILL_TRANSPARENT, NULL);
  else
    clay_fill_options_set_by_fill_type (options, context,
                                        CLAY_FILL_BACKGROUND, NULL);

  clay_drawable_edit_fill (drawable, options, C_("undo-type", "Clear"));

  g_object_unref (options);
}

void
clay_drawable_edit_fill (ClayDrawable    *drawable,
                         ClayFillOptions *options,
                         const gchar     *undo_desc)
{
  ClayContext *context;
  gint         x, y, width, height;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (CLAY_IS_FILL_OPTIONS (options));

  if (! clay_item_mask_intersect (CLAY_ITEM (drawable),
                                  &x, &y, &width, &height))
    {
      return;  /*  nothing to do, but the fill succeeded  */
    }

  context = CLAY_CONTEXT (options);

  if (clay_layer_mode_is_alpha_only (clay_context_get_paint_mode (context)))
    {
      if (! clay_drawable_has_alpha (drawable) ||
          ! (clay_drawable_get_active_mask (drawable) &
             CLAY_COMPONENT_MASK_ALPHA))
        {
          return;  /*  nothing to do, but the fill succeeded  */
        }
    }

  if (! undo_desc)
    undo_desc = clay_fill_options_get_undo_desc (options);

  /* check if we can fill the drawable's buffer directly */
  if (clay_drawable_edit_can_fill_direct (drawable, options))
    {
      clay_drawable_edit_fill_direct (drawable, options, undo_desc);

      if (clay_drawable_has_visible_filters (drawable))
        {
          /* For drawables with filters, update the bounding box then
           * let the drawable update everything, because the filtered
           * render may be bigger than the filled part.
           */
          clay_drawable_update_bounding_box (drawable);
          clay_drawable_update (drawable, 0, 0, -1, -1);
        }
      else
        {
          clay_drawable_update (drawable, x, y, width, height);
        }
    }
  else
    {
      GeglNode               *operation;
      ClayDrawableFilter     *filter;
      gdouble                 opacity;
      ClayLayerMode           mode;
      ClayLayerCompositeMode  composite_mode;
      ClayContainer          *filter_stack;

      opacity        = clay_context_get_opacity (context);
      mode           = clay_context_get_paint_mode (context);
      composite_mode = clay_layer_mode_get_paint_composite_mode (mode);

      operation = gegl_node_new_child (NULL,
                                       "operation",        "clay:fill-source",
                                       "options",          options,
                                       "drawable",         drawable,
                                       "pattern-offset-x", -x,
                                       "pattern-offset-y", -y,
                                       NULL);

      filter = clay_drawable_filter_new (drawable, undo_desc, operation, NULL);

      clay_drawable_filter_set_opacity (filter, opacity);
      clay_drawable_filter_set_mode    (filter,
                                        mode,
                                        CLAY_LAYER_COLOR_SPACE_AUTO,
                                        CLAY_LAYER_COLOR_SPACE_AUTO,
                                        composite_mode);

      clay_drawable_filter_apply  (filter, NULL);
      /* Move to bottom of filter stack */
      filter_stack = clay_drawable_get_filters (drawable);
      if (filter_stack)
        clay_container_reorder (filter_stack, CLAY_OBJECT (filter),
                                clay_container_get_n_children (filter_stack) - 1);

      clay_drawable_filter_commit (filter, FALSE, NULL, FALSE);

      g_object_unref (filter);
      g_object_unref (operation);
    }
}
