/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawable-filters.c
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
#include <cairo.h>

#include "core-types.h"

#include "gegl/clayapplicator.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "clay.h"
#include "clay-utils.h"
#include "claychannel.h"
#include "claydrawable.h"
#include "claydrawable-filters.h"
#include "claydrawable-private.h"
#include "claydrawablefilter.h"
#include "claydrawablefilterundo.h"
#include "clayfilter.h"
#include "clayfilterstack.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "claylayer.h"
#include "clayprogress.h"
#include "clayprojection.h"

#include "clay-intl.h"


void
_clay_drawable_filters_init (ClayDrawable *drawable)
{
  drawable->private->filter_stack = clay_filter_stack_new (CLAY_TYPE_FILTER);
}

void
_clay_drawable_filters_finalize (ClayDrawable *drawable)
{
  g_clear_object (&drawable->private->filter_stack);
}

ClayContainer *
clay_drawable_get_filters (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  return drawable->private->filter_stack;
}

gboolean
clay_drawable_has_visible_filters (ClayDrawable *drawable)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  for (list = CLAY_LIST (drawable->private->filter_stack)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayFilter *filter = list->data;

      if (clay_filter_get_active (filter))
        return TRUE;
    }

  return FALSE;
}

gboolean
clay_drawable_n_editable_filters (ClayDrawable *drawable,
                                  gint         *n_editable,
                                  gint         *first,
                                  gint         *last)
{
  GList    *list;
  gboolean  editing_blocked = FALSE;
  gint      index           = 0;
  gint      n               = 0;
  gint      first_editable  = -1;
  gint      last_editable   = -1;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  for (list = CLAY_LIST (drawable->private->filter_stack)->queue->head;
       list;
       list = g_list_next (list), index++)
    {
      ClayFilter *filter   = list->data;
      gboolean    editable = FALSE;

      if (CLAY_IS_DRAWABLE_FILTER (filter))
        {
          if (clay_drawable_filter_get_temporary (CLAY_DRAWABLE_FILTER (filter)))
            {
              editing_blocked = TRUE;
            }
          else
            {
              editable = TRUE;
            }
        }

      if (editable)
        {
          n++;

          if (first_editable == -1)
            first_editable = index;

          last_editable = index;
        }
    }

  if (n_editable) *n_editable = n;
  if (first)      *first      = first_editable;
  if (last)       *last       = last_editable;

  return ! editing_blocked;
}

void
clay_drawable_add_filter (ClayDrawable *drawable,
                          ClayFilter   *filter)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (CLAY_IS_FILTER (filter));
  g_return_if_fail (clay_drawable_has_filter (drawable, filter) == FALSE);

  clay_container_add (drawable->private->filter_stack,
                      CLAY_OBJECT (filter));

  clay_drawable_filters_changed (drawable);
}

void
clay_drawable_remove_filter (ClayDrawable *drawable,
                             ClayFilter   *filter)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (CLAY_IS_FILTER (filter));
  g_return_if_fail (clay_drawable_has_filter (drawable, filter) == TRUE);

  clay_container_remove (drawable->private->filter_stack,
                         CLAY_OBJECT (filter));

  clay_drawable_filters_changed (drawable);
}

void
clay_drawable_clear_filters (ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  clay_container_clear (drawable->private->filter_stack);

  clay_drawable_filters_changed (drawable);
}

gboolean
clay_drawable_has_filter (ClayDrawable *drawable,
                          ClayFilter   *filter)
{
  gboolean filter_exists = FALSE;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (CLAY_IS_FILTER (filter), FALSE);

  filter_exists = clay_container_have (drawable->private->filter_stack,
                                       CLAY_OBJECT (filter));

  return filter_exists;
}

gboolean
clay_drawable_raise_filter (ClayDrawable *drawable,
                            ClayFilter   *filter)
{
  gint index;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (clay_drawable_has_filter (drawable, filter), FALSE);

  if (clay_drawable_filter_get_mask (CLAY_DRAWABLE_FILTER (filter)) == NULL)
    return FALSE;

  index = clay_container_get_child_index (drawable->private->filter_stack,
                                          CLAY_OBJECT (filter));
  index--;

  if (index >= 0)
    {
      clay_image_undo_push_filter_reorder (clay_item_get_image (CLAY_ITEM (drawable)),
                                           _("Reorder filter"),
                                           drawable,
                                           CLAY_DRAWABLE_FILTER (filter));

      clay_container_reorder (drawable->private->filter_stack,
                              CLAY_OBJECT (filter), index);

      clay_drawable_update (drawable, 0, 0, -1, -1);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_drawable_lower_filter (ClayDrawable *drawable,
                            ClayFilter   *filter)
{
  gint index;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (clay_drawable_has_filter (drawable, filter), FALSE);

  if (clay_drawable_filter_get_mask (CLAY_DRAWABLE_FILTER (filter)) == NULL)
    return FALSE;

  index = clay_container_get_child_index (drawable->private->filter_stack,
                                          CLAY_OBJECT (filter));
  index++;

  if (index < clay_container_get_n_children (drawable->private->filter_stack))
    {
      /* Don't rearrange filters with floating selection */
      if (! CLAY_IS_DRAWABLE_FILTER (clay_container_get_child_by_index (drawable->private->filter_stack,
                                                                        index)))
        return FALSE;

      clay_image_undo_push_filter_reorder (clay_item_get_image (CLAY_ITEM (drawable)),
                                           _("Reorder filter"),
                                           drawable,
                                           CLAY_DRAWABLE_FILTER (filter));

      clay_container_reorder (drawable->private->filter_stack,
                              CLAY_OBJECT (filter), index);

      clay_drawable_update (drawable, 0, 0, -1, -1);

      return TRUE;
    }

  return FALSE;
}

void
clay_drawable_merge_filters (ClayDrawable *drawable)
{
  ClayImage   *image;
  ClayChannel *selection;
  GeglBuffer  *buffer;
  GList       *list;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  image = clay_item_get_image (CLAY_ITEM (drawable));

  /* Temporarily remove selection so filters can be merged down correctly */
  selection =
    CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (clay_image_get_mask (image)),
                                       CLAY_TYPE_CHANNEL));
  clay_channel_clear (clay_image_get_mask (image), NULL, TRUE);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE,
                               _("Rasterize filters"));

  /* We do not commit filters individually because we'd end up in the
   * situation where we'd force cast back to the image's precision after
   * every effect. See #12614.
   * Instead we get the result of the top filter and change the
   * drawable's buffer.
   */
  if (CLAY_LIST (drawable->private->filter_stack)->queue->head)
    {
      ClayDrawableFilter *filter    = NULL;
      gboolean            add_alpha = FALSE;

      for (list = CLAY_LIST (drawable->private->filter_stack)->queue->tail;
           list;
           list = list->prev)
        {
          if (CLAY_IS_DRAWABLE_FILTER (list->data) &&
              clay_filter_get_active (CLAY_FILTER (list->data)))
            {
              filter = list->data;

              if (! add_alpha)
                add_alpha = clay_drawable_filter_get_add_alpha (filter);
            }
        }

      if (filter)
        {
          ClayApplicator *applicator;
          GeglNode       *graph;
          GeglNode       *output;
          GeglRectangle   output_rect;

          if (CLAY_IS_LAYER (drawable) && add_alpha)
            clay_layer_add_alpha (CLAY_LAYER (drawable));

          graph       = clay_filter_stack_get_graph (CLAY_FILTER_STACK (drawable->private->filter_stack));
          output      = gegl_node_get_output_proxy (graph, "output");
          output_rect = gegl_node_get_bounding_box (output);
          buffer      = clay_gegl_buffer_new (&output_rect, clay_drawable_get_format (drawable), drawable);

          applicator  = clay_filter_get_applicator (CLAY_FILTER (filter));
          clay_applicator_set_dest_buffer (applicator, buffer);
          clay_applicator_blit (applicator, gegl_buffer_get_extent (buffer));
          clay_drawable_set_buffer (drawable, TRUE, NULL, buffer);
          g_clear_object (&buffer);
        }

      /*  don't use a for() loop because we are deleting the list
       *  under our feet
       */
      list = CLAY_LIST (drawable->private->filter_stack)->queue->head;
      while (list)
        {
          filter = list->data;

          list = list->next;

          if (CLAY_IS_DRAWABLE_FILTER (filter)              &&
              clay_filter_get_active (CLAY_FILTER (filter)) &&
              ! clay_drawable_filter_get_temporary (filter))

            {
              clay_image_undo_push_filter_remove (clay_item_get_image (CLAY_ITEM (drawable)),
                                                  _("Merge filter"),
                                                  drawable, filter);
              clay_drawable_remove_filter (drawable, CLAY_FILTER (filter));
            }
        }
    }

  clay_image_undo_group_end (clay_item_get_image (CLAY_ITEM (drawable)));

  /* Restore selection after merging down */
  buffer = clay_gegl_buffer_dup (clay_drawable_get_buffer (CLAY_DRAWABLE (selection)),
                                 CLAY_DRAWABLE (clay_image_get_mask (image)));
  clay_drawable_set_buffer (CLAY_DRAWABLE (clay_image_get_mask (image)),
                            FALSE, NULL, buffer);
  g_object_unref (buffer);
  g_object_ref_sink (selection);
  g_object_unref (selection);

  clay_drawable_filters_changed (drawable);
}

gboolean
clay_drawable_merge_filter (ClayDrawable *drawable,
                            ClayFilter   *filter,
                            ClayProgress *progress,
                            const gchar  *undo_desc,
                            const Babl   *format,
                            gboolean      clip,
                            gboolean      cancellable,
                            gboolean      update)
{
  ClayImage      *image;
  ClayApplicator *applicator;
  gboolean        applicator_cache         = FALSE;
  const Babl     *applicator_output_format = NULL;
  GeglBuffer     *buffer                   = NULL;
  GeglBuffer     *dest_buffer;
  GeglBuffer     *undo_buffer              = NULL;
  GeglRectangle   undo_rect;
  GeglBuffer     *cache                    = NULL;
  GeglRectangle  *rects                    = NULL;
  gint            n_rects                  = 0;
  GeglRectangle   rect;
  gboolean        success                  = TRUE;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (CLAY_IS_FILTER (filter), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);

  image       = clay_item_get_image (CLAY_ITEM (drawable));
  applicator  = clay_filter_get_applicator (filter);
  dest_buffer = clay_drawable_get_buffer (drawable);

  if (! format)
    format = clay_drawable_get_format (drawable);

  rect = gegl_node_get_bounding_box (clay_filter_get_node (filter));

  if (! clip && gegl_rectangle_equal (&rect,
                                      gegl_buffer_get_extent (dest_buffer)))
    {
      clip = TRUE;
    }

  if (clip)
    {
      if (! clay_item_mask_intersect (CLAY_ITEM (drawable),
                                      &rect.x, &rect.y,
                                      &rect.width, &rect.height))
        {
          return TRUE;
        }

      if (format != clay_drawable_get_format (drawable))
        {
          buffer = clay_gegl_buffer_new (gegl_buffer_get_extent (dest_buffer), format, drawable);

          dest_buffer = buffer;
        }
    }
  else
    {
      buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                     format, drawable);

      dest_buffer = g_object_new (GEGL_TYPE_BUFFER,
                                  "source",  buffer,
                                  "shift-x", -rect.x,
                                  "shift-y", -rect.y,
                                  NULL);
    }

  if (applicator)
    {
      const GeglRectangle *crop_rect;

      crop_rect = clay_applicator_get_crop (applicator);

      if (crop_rect && ! gegl_rectangle_intersect (&rect, &rect, crop_rect))
        return TRUE;

      /*  the cache and its valid rectangles are the region that
       *  has already been processed by this applicator.
       */
      cache = clay_applicator_get_cache_buffer (applicator,
                                                &rects, &n_rects);

      /*  skip the cache and output-format conversion while processing
       *  the remaining area, so that the result is written directly to
       *  the drawable's buffer.
       */
      applicator_cache         = clay_applicator_get_cache (applicator);
      applicator_output_format = clay_applicator_get_output_format (applicator);

      clay_applicator_set_cache (applicator, FALSE);
      if (applicator_output_format == format)
        clay_applicator_set_output_format (applicator, NULL);
    }

  if (! buffer)
    {
      gegl_rectangle_align_to_buffer (
        &undo_rect,
        &rect,
        clay_drawable_get_buffer (drawable),
        GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      undo_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                     undo_rect.width,
                                                     undo_rect.height),
                                     clay_drawable_get_format (drawable));

      clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable),
                             &undo_rect,
                             GEGL_ABYSS_NONE,
                             undo_buffer,
                             GEGL_RECTANGLE (0, 0, 0, 0));
    }

  clay_projection_stop_rendering (clay_image_get_projection (image));

  /* make sure we have a source node - this connects the filter stack to the
   * underlying source node
   */
  (void) clay_drawable_get_source_node (drawable);

  if (clay_gegl_apply_cached_operation (clay_drawable_get_buffer (drawable),
                                        progress, undo_desc,
                                        clay_filter_get_node (filter), FALSE,
                                        dest_buffer, &rect, FALSE,
                                        cache, rects, n_rects,
                                        cancellable))
    {
      /*  finished successfully  */

      if (clip)
        {
          if (buffer)
            {
              clay_drawable_set_buffer_full (drawable,
                                             TRUE, undo_desc,
                                             buffer, NULL,
                                             FALSE);
            }
          else
            {
              clay_drawable_push_undo (drawable, undo_desc, undo_buffer,
                                       undo_rect.x, undo_rect.y,
                                       undo_rect.width, undo_rect.height);
            }
        }
      else
        {
          ClayLayerMask *mask = NULL;
          gint           offset_x;
          gint           offset_y;

          clay_item_get_offset (CLAY_ITEM (drawable), &offset_x, &offset_y);

          if (CLAY_IS_LAYER (drawable))
            mask = clay_layer_get_mask (CLAY_LAYER (drawable));

          if (mask)
            {
              clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                           undo_desc);
            }

          clay_drawable_set_buffer_full (
            drawable, TRUE, undo_desc, buffer,
            GEGL_RECTANGLE (offset_x + rect.x, offset_y + rect.y, 0, 0),
            FALSE);

          if (mask)
            {
              clay_item_resize (CLAY_ITEM (mask),
                                clay_get_default_context (image->clay),
                                CLAY_FILL_TRANSPARENT,
                                rect.width, rect.height,
                                -rect.x, -rect.y);

              clay_image_undo_group_end (image);
            }
        }
    }
  else
    {
      /*  canceled by the user  */

      if (clip)
        {
          clay_gegl_buffer_copy (undo_buffer,
                                 GEGL_RECTANGLE (0, 0,
                                                 undo_rect.width,
                                                 undo_rect.height),
                                 GEGL_ABYSS_NONE,
                                 clay_drawable_get_buffer (drawable),
                                 &undo_rect);
        }

      success = FALSE;
    }

  if (clip)
    {
      g_clear_object (&undo_buffer);
      g_clear_object (&buffer);
    }
  else
    {
      g_object_unref (buffer);
      g_object_unref (dest_buffer);
    }

  if (cache)
    {
      g_object_unref (cache);
      g_free (rects);
    }

  if (applicator)
    {
      clay_applicator_set_cache (applicator, applicator_cache);
      clay_applicator_set_output_format (applicator, applicator_output_format);
    }

  if (update)
    {
      clay_drawable_update (drawable,
                            rect.x, rect.y,
                            rect.width, rect.height);
    }

  if (success)
    clay_drawable_filters_changed (drawable);

  return success;
}
