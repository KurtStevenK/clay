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

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clay-babl-compat.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-nodes.h"
#include "gegl/clay-gegl-utils.h"

#include "path/claypath.h"

#include "clay.h"
#include "claycontext.h"
#include "claydrawable-filters.h"
#include "clayerror.h"
#include "claygrouplayer.h"
#include "clayimage.h"
#include "clayimage-merge.h"
#include "clayimage-undo.h"
#include "clayitemstack.h"
#include "claylayer-floating-selection.h"
#include "claylayer-new.h"
#include "claylayermask.h"
#include "clayparasitelist.h"
#include "claypickable.h"
#include "clayprogress.h"
#include "clayprojectable.h"
#include "clayundostack.h"

#include "clay-intl.h"


static GSList    * clay_image_trim_merge_list (ClayImage     *image,
                                               GSList        *merge_list);
static ClayLayer * clay_image_merge_layers    (ClayImage     *image,
                                               ClayContainer *container,
                                               GSList        *merge_list,
                                               ClayContext   *context,
                                               ClayMergeType  merge_type,
                                               const gchar   *undo_desc,
                                               ClayProgress  *progress);


/*  public functions  */

GList *
clay_image_merge_visible_layers (ClayImage     *image,
                                 ClayContext   *context,
                                 ClayMergeType  merge_type,
                                 gboolean       merge_active_group,
                                 gboolean       discard_invisible,
                                 ClayProgress  *progress)
{
  const gchar *undo_desc  = C_("undo-type", "Merge Visible Layers");
  GList       *containers = NULL;
  GList       *new_layers = NULL;
  GList       *iter;
  GList       *iter2;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);

  if (merge_active_group)
    {
      GList *selected_layers = clay_image_get_selected_layers (image);

      /*  if the active layer is the floating selection, get the
       *  underlying drawable, but only if it is a layer
       */
      if (g_list_length (selected_layers) == 1 && clay_layer_is_floating_sel (selected_layers->data))
        {
          ClayDrawable *fs_drawable;

          fs_drawable = clay_layer_get_floating_sel_drawable (selected_layers->data);

          if (CLAY_IS_LAYER (fs_drawable))
            containers = g_list_prepend (containers,
                                         clay_item_get_container (CLAY_ITEM (fs_drawable)));
        }
      else
        {
          for (iter = selected_layers; iter; iter = iter->next)
            if (! clay_item_get_parent (iter->data))
              break;

          /* No need to list selected groups if any selected layer is
           * top-level.
           */
          if (iter == NULL)
            {
              for (iter = selected_layers; iter; iter = iter->next)
                {
                  for (iter2 = selected_layers; iter2; iter2 = iter2->next)
                    {
                      /* Only retain a selected layer's container if no
                       * other selected layers are its parents.
                       */
                      if (iter->data != iter2->data &&
                          clay_viewable_is_ancestor (iter2->data, iter->data))
                        break;
                    }
                  if (iter2 == NULL &&
                      ! g_list_find (containers, clay_item_get_container (CLAY_ITEM (iter->data))))
                    containers = g_list_prepend (containers,
                                                 clay_item_get_container (CLAY_ITEM (iter->data)));
                }
            }
        }
    }
  if (! containers)
    containers = g_list_prepend (NULL, clay_image_get_layers (image));

  clay_set_busy (image->clay);
  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  for (iter = containers; iter; iter = iter->next)
    {
      ClayContainer *container      = iter->data;
      GSList        *merge_list     = NULL;
      GSList        *invisible_list = NULL;

      for (iter2 = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (container));
           iter2;
           iter2 = g_list_next (iter2))
        {
          ClayLayer *layer = iter2->data;

          if (clay_layer_is_floating_sel (layer))
            continue;

          if (clay_item_get_visible (CLAY_ITEM (layer)))
            {
              merge_list = g_slist_append (merge_list, layer);
            }
          else if (discard_invisible)
            {
              invisible_list = g_slist_append (invisible_list, layer);
            }
        }

      if (merge_list)
        {
          ClayLayer *layer;

          /* if there's a floating selection, anchor it */
          if (clay_image_get_floating_selection (image))
            floating_sel_anchor (clay_image_get_floating_selection (image));

          layer = clay_image_merge_layers (image,
                                           container,
                                           merge_list, context, merge_type,
                                           undo_desc, progress);
          g_slist_free (merge_list);

          if (invisible_list)
            {
              GSList *list;

              for (list = invisible_list; list; list = g_slist_next (list))
                clay_image_remove_layer (image, list->data, TRUE, NULL);

              g_slist_free (invisible_list);
            }

          new_layers = g_list_prepend (new_layers, layer);
        }
    }

  clay_image_set_selected_layers (image, new_layers);
  clay_image_undo_group_end (image);
  clay_unset_busy (image->clay);

  g_list_free (new_layers);
  g_list_free (containers);

  return clay_image_get_selected_layers (image);
}

ClayLayer *
clay_image_flatten (ClayImage     *image,
                    ClayContext   *context,
                    ClayProgress  *progress,
                    GError       **error)
{
  GList     *list;
  GSList    *merge_list = NULL;
  ClayLayer *layer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = clay_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (clay_layer_is_floating_sel (layer))
        continue;

      if (clay_item_get_visible (CLAY_ITEM (layer)))
        merge_list = g_slist_append (merge_list, layer);
    }

  if (merge_list)
    {
      const gchar *undo_desc = C_("undo-type", "Flatten Image");

      clay_set_busy (image->clay);

      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_desc);

      /* if there's a floating selection, anchor it */
      if (clay_image_get_floating_selection (image))
        floating_sel_anchor (clay_image_get_floating_selection (image));

      layer = clay_image_merge_layers (image,
                                       clay_image_get_layers (image),
                                       merge_list, context,
                                       CLAY_FLATTEN_IMAGE,
                                       undo_desc, progress);
      g_slist_free (merge_list);

      clay_image_alpha_changed (image);

      clay_image_undo_group_end (image);

      clay_unset_busy (image->clay);

      return layer;
    }

  g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                       _("Cannot flatten an image without any visible layer."));
  return NULL;
}

GList *
clay_image_merge_down (ClayImage      *image,
                       GList          *layers,
                       ClayContext    *context,
                       ClayMergeType   merge_type,
                       ClayProgress   *progress,
                       ClayItem      **blink_item,
                       GError        **error)
{
  GList       *merged_layers = NULL;
  GList       *merge_lists   = NULL;
  GSList      *merge_list    = NULL;
  ClayLayer   *layer;
  GList       *list;
  const gchar *undo_desc;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  layers = g_list_copy (layers);
  while ((list = layers))
    {
      GList *list2;

      g_return_val_if_fail (CLAY_IS_LAYER (list->data), NULL);
      g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (list->data)), NULL);

      if (clay_layer_is_floating_sel (list->data))
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               _("Cannot merge down a floating selection."));
          if (blink_item)
            *blink_item = CLAY_ITEM (list->data);
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }
      if (clay_layer_get_mode (list->data) == CLAY_LAYER_MODE_PASS_THROUGH)
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               _("Cannot merge down a pass through layer group."));
          if (blink_item)
            *blink_item = CLAY_ITEM (list->data);
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      if (! clay_item_get_visible (CLAY_ITEM (list->data)))
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               _("Cannot merge down an invisible layer."));
          if (blink_item)
            *blink_item = CLAY_ITEM (list->data);
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      for (list2 = clay_item_get_container_iter (CLAY_ITEM (list->data));
           list2;
           list2 = g_list_next (list2))
        {
          if (list2->data == list->data)
            break;
        }

      layer = NULL;
      for (list2 = g_list_next (list2);
           list2;
           list2 = g_list_next (list2))
        {
          layer = list2->data;

          if (clay_item_get_visible (CLAY_ITEM (layer)))
            {
              if (clay_viewable_get_children (CLAY_VIEWABLE (layer)))
                {
                  g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                       _("Cannot merge down to a layer group."));
                  if (blink_item)
                    *blink_item = CLAY_ITEM (layer);
                  g_list_free (layers);
                  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
                  return NULL;
                }

              if (clay_item_is_content_locked (CLAY_ITEM (layer), NULL))
                {
                  g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                       _("The layer to merge down to is locked."));
                  if (blink_item)
                    *blink_item = CLAY_ITEM (layer);
                  g_list_free (layers);
                  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
                  return NULL;
                }

              break;
            }

          layer = NULL;
        }

      if (! layer)
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               _("There is no visible layer to merge down to."));
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      merge_list = g_slist_append (merge_list, list->data);
      layers     = g_list_delete_link (layers, layers);

      if ((list = g_list_find (layers, layer)))
        {
          /* The next item is itself in the merge down list. Continue
           * filling the same merge list instead of starting a new one.
           */
          layers = g_list_delete_link (layers, list);
          layers = g_list_prepend (layers, layer);

          continue;
        }

      merge_list  = g_slist_append (merge_list, layer);
      merge_lists = g_list_prepend (merge_lists, merge_list);
      merge_list  = NULL;
    }

  undo_desc = C_("undo-type", "Merge Down");

  clay_set_busy (image->clay);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  for (list = merge_lists; list; list = list->next)
    {
      merge_list = list->data;
      layer = clay_image_merge_layers (image,
                                       clay_item_get_container (merge_list->data),
                                       merge_list, context, merge_type,
                                       undo_desc, progress);
      merged_layers = g_list_prepend (merged_layers, layer);
    }

  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);

  clay_image_undo_group_end (image);

  clay_unset_busy (image->clay);

  return merged_layers;
}

ClayLayer *
clay_image_merge_group_layer (ClayImage      *image,
                              ClayGroupLayer *group)
{
  ClayLayer     *parent;
  ClayLayer     *layer;
  GeglBuffer    *pass_through_buffer = NULL;
  gboolean       is_pass_through     = FALSE;
  gint           index;
  GeglRectangle  rect;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);
  g_return_val_if_fail (clay_item_get_image (CLAY_ITEM (group)) == image, NULL);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               C_("undo-type", "Merge Layer Group"));

  parent = clay_layer_get_parent (CLAY_LAYER (group));
  index  = clay_item_get_index (CLAY_ITEM (group));

  is_pass_through = (clay_layer_get_mode (CLAY_LAYER (group)) == CLAY_LAYER_MODE_PASS_THROUGH &&
                     clay_item_get_visible (CLAY_ITEM (group)));
  if (is_pass_through &&
      (clay_layer_get_opacity (CLAY_LAYER (group)) < 1.0 ||
       ! clay_drawable_has_visible_filters (CLAY_DRAWABLE (group))))
    {
      ClayDrawable  *drawable  = CLAY_DRAWABLE (group);
      GeglNode      *mode_node = clay_drawable_get_mode_node (drawable);

      rect = gegl_node_get_bounding_box (mode_node);
      pass_through_buffer = clay_gegl_buffer_new (&rect, clay_drawable_get_format (drawable), NULL);
      gegl_node_blit_buffer (mode_node, pass_through_buffer, &rect, 0, GEGL_ABYSS_NONE);
    }

  /* Merge down filter effects */
  clay_drawable_merge_filters (CLAY_DRAWABLE (group));
  clay_drawable_clear_filters (CLAY_DRAWABLE (group));

  layer = CLAY_LAYER (clay_item_duplicate (CLAY_ITEM (group),
                                           CLAY_TYPE_LAYER));

  clay_object_set_name (CLAY_OBJECT (layer), clay_object_get_name (group));

  clay_image_remove_layer (image, CLAY_LAYER (group), TRUE, NULL);
  clay_image_add_layer (image, layer, parent, index, TRUE);

  /* Pass-through groups are a very special case. The duplicate works
   * only if both these points are true:
   * - The group is at full opacity: with lower opacity, what we want is
   *   in fact the output of the "clay:pass-through" mode (similar to
   *   "clay:replace") because we can't reproduce the same render
   *   otherwise. This works well, since anyway the merged layer is
   *   ensured to be the bottomest one on its own level.
   * - The group has filters: clay_drawable_merge_filters() will
   *   actually set the end-rendering to the drawable (kinda rasterizing
   *   the group layer).
   */
   if (pass_through_buffer)
    {
      /* The group duplicate may be smaller than actual render in case
       * of pass-through groups.
       */
      clay_item_set_size (CLAY_ITEM (layer), rect.width, rect.height);

      if (rect.x != 0 || rect.y != 0)
        {
          GeglBuffer *buffer;

          buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                         clay_drawable_get_format (CLAY_DRAWABLE (layer)),
                                         CLAY_DRAWABLE (layer));
          gegl_buffer_copy (pass_through_buffer, &rect, GEGL_ABYSS_NONE,
                            buffer, GEGL_RECTANGLE (0, 0, rect.width, rect.height));
          g_object_unref (pass_through_buffer);
          pass_through_buffer = buffer;
        }
      clay_drawable_set_buffer_full (CLAY_DRAWABLE (layer), FALSE, NULL,
                                     pass_through_buffer, &rect, TRUE);
      g_object_unref (pass_through_buffer);
    }

  /* For pass-through group layers, we must remove all "big sister"
   * layers, i.e. all layers on the same level below the newly merged
   * layer, because their render is already integrated in the merged
   * layer. Therefore keeping them would change the whole image's
   * rendering.
   */
  if (is_pass_through)
    {
      ClayContainer *stack;
      GList         *iter;
      GList         *new_selected = g_list_prepend (NULL, layer);
      GList         *to_remove    = NULL;
      gboolean       remove       = FALSE;

      if (parent)
        stack = clay_viewable_get_children (CLAY_VIEWABLE (parent));
      else
        stack = clay_image_get_layers (image);

      for (iter = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (stack)); iter; iter = iter->next)
        {
          if (iter->data == layer)
            remove = TRUE;
          else if (remove && clay_item_get_visible (iter->data))
            to_remove = g_list_prepend (to_remove, iter->data);
        }

      for (iter = to_remove; iter; iter = iter->next)
        clay_image_remove_layer (image, CLAY_LAYER (iter->data), TRUE, new_selected);

      g_list_free (new_selected);
    }

  clay_image_undo_group_end (image);

  return layer;
}


/* merging paths */

ClayPath *
clay_image_merge_visible_paths (ClayImage  *image,
                                GError    **error)
{
  GList       *list;
  GList       *merge_list = NULL;
  ClayPath    *path;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = clay_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      path = list->data;

      if (clay_item_get_visible (CLAY_ITEM (path)))
        merge_list = g_list_prepend (merge_list, path);
    }

  merge_list = g_list_reverse (merge_list);

  if (merge_list && merge_list->next)
    {
      ClayPath *target_path;
      gchar    *name;
      gint      pos;

      clay_set_busy (image->clay);

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_PATHS_MERGE,
                                   C_("undo-type", "Merge Visible Paths"));

      path = CLAY_PATH (merge_list->data);

      name = g_strdup (clay_object_get_name (path));
      pos = clay_item_get_index (CLAY_ITEM (path));

      target_path = CLAY_PATH (clay_item_duplicate (CLAY_ITEM (path),
                                                    CLAY_TYPE_PATH));
      clay_image_remove_path (image, path, TRUE, NULL);

      for (list = g_list_next (merge_list);
           list;
           list = g_list_next (list))
        {
          path = list->data;

          clay_path_add_strokes (path, target_path);
          clay_image_remove_path (image, path, TRUE, NULL);
        }

      clay_object_take_name (CLAY_OBJECT (target_path), name);

      g_list_free (merge_list);

      /* FIXME tree */
      clay_image_add_path (image, target_path, NULL, pos, TRUE);
      clay_unset_busy (image->clay);

      clay_image_undo_group_end (image);

      return target_path;
    }
  else
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Not enough visible paths for a merge. "
                             "There must be at least two."));
      return NULL;
    }
}


/*  private functions  */

static GSList *
clay_image_trim_merge_list (ClayImage *image,
                            GSList    *merge_list)
{
  GSList *trimmed_list = NULL;
  GSList *pass_through = NULL;

  for (GSList *iter = merge_list; iter; iter = iter->next)
    {
      ClayLayer *layer  = iter->data;
      gboolean   ignore = FALSE;

      for (GSList *iter2 = pass_through; iter2; iter2 = iter2->next)
        {
          ClayLayer *pass_through_parent = clay_layer_get_parent (iter2->data);
          ClayLayer *cousin              = layer;

          do
            {
              ClayLayer *cousin_parent = clay_layer_get_parent (cousin);

              if (pass_through_parent == cousin_parent &&
                  clay_item_get_index (CLAY_ITEM (iter2->data)) < clay_item_get_index (CLAY_ITEM (cousin)))
                {
                  /* The "cousin" layer is in the same group layer as a
                   * pass-through group and below it. We don't merge it
                   * because it will be rendered already through the
                   * merged pass-through by definition.
                   */
                  ignore = TRUE;
                  break;
                }

              cousin = cousin_parent;
            }
          while (cousin != NULL);

          if (ignore)
            break;
        }

      if (! ignore)
        {
          trimmed_list = g_slist_append (trimmed_list, layer);

          if (clay_viewable_get_children (CLAY_VIEWABLE (layer)) &&
              clay_layer_get_mode (layer) == CLAY_LAYER_MODE_PASS_THROUGH)
            pass_through = g_slist_append (pass_through, layer);
        }
    }

  g_slist_free (pass_through);

  return trimmed_list;
}

static ClayLayer *
clay_image_merge_layers (ClayImage     *image,
                         ClayContainer *container,
                         GSList        *merge_list,
                         ClayContext   *context,
                         ClayMergeType  merge_type,
                         const gchar   *undo_desc,
                         ClayProgress  *progress)
{
  ClayLayer        *parent;
  gint              x1, y1;
  gint              x2, y2;
  GSList           *layers;
  ClayLayer        *layer;
  ClayLayer        *top_layer;
  ClayLayer        *bottom_layer;
  ClayLayer        *merge_layer;
  gint              position;
  gint              n_bottom_removed = 0;
  GeglNode         *node;
  GeglNode         *source_node;
  GeglNode         *flatten_node;
  GeglNode         *offset_node;
  GeglNode         *last_node;
  GeglNode         *last_node_source = NULL;
  ClayParasiteList *parasites;
  GSList           *trimmed_list;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);

  trimmed_list = clay_image_trim_merge_list (image, merge_list);

  top_layer = trimmed_list->data;
  parent    = clay_layer_get_parent (top_layer);

  /*  Make sure the image's graph is constructed, so that top-level layers have
   *  a parent node.
   */
  (void) clay_projectable_get_graph (CLAY_PROJECTABLE (image));

  /*  Make sure the parent's graph is constructed, so that the top layer has a
   *  parent node, even if it is the child of a group layer (in particular, of
   *  an invisible group layer, whose graph may not have been constructed as a
   *  result of the above call.  see issue #2095.)
   */
  if (parent)
    (void) clay_filter_get_node (CLAY_FILTER (parent));

  /*  Build our graph inside the top-layer's parent node  */
  source_node = clay_filter_get_node (CLAY_FILTER (top_layer));
  node = gegl_node_get_parent (source_node);

  g_return_val_if_fail (node, NULL);

  /*  Get the layer extents  */
  x1 = y1 = 0;
  x2 = y2 = 0;
  for (layers = trimmed_list; layers; layers = g_slist_next (layers))
    {
      gint off_x, off_y;
      gint width, height;

      layer = layers->data;

      /* Merge down filter effects so they can be restored on undo */
      clay_drawable_merge_filters (CLAY_DRAWABLE (layer));
      clay_drawable_clear_filters (CLAY_DRAWABLE (layer));

      if (clay_layer_get_mode (layer) == CLAY_LAYER_MODE_PASS_THROUGH)
        {
          GeglNode      *mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (layer));
          GeglRectangle  rect;

          rect   = gegl_node_get_bounding_box (mode_node);
          off_x  = rect.x;
          off_y  = rect.y;
          width  = rect.width;
          height = rect.height;
        }
      else
        {
          clay_item_get_offset (CLAY_ITEM (layer), &off_x, &off_y);
          width  = clay_item_get_width (CLAY_ITEM (layer));
          height = clay_item_get_height (CLAY_ITEM (layer));
        }

      switch (merge_type)
        {
        case CLAY_EXPAND_AS_NECESSARY:
        case CLAY_CLIP_TO_IMAGE:
          if (layers == trimmed_list)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + width;
              y2 = off_y + height;
            }
          else
            {
              if (off_x < x1)
                x1 = off_x;
              if (off_y < y1)
                y1 = off_y;
              if ((off_x + width) > x2)
                x2 = (off_x + width);
              if ((off_y + height) > y2)
                y2 = (off_y + height);
            }

          if (merge_type == CLAY_CLIP_TO_IMAGE)
            {
              x1 = CLAMP (x1, 0, clay_image_get_width  (image));
              y1 = CLAMP (y1, 0, clay_image_get_height (image));
              x2 = CLAMP (x2, 0, clay_image_get_width  (image));
              y2 = CLAMP (y2, 0, clay_image_get_height (image));
            }
          break;

        case CLAY_CLIP_TO_BOTTOM_LAYER:
          if (layers->next == NULL)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + width;
              y2 = off_y + height;
            }
          break;

        case CLAY_FLATTEN_IMAGE:
          if (layers->next == NULL)
            {
              x1 = 0;
              y1 = 0;
              x2 = clay_image_get_width  (image);
              y2 = clay_image_get_height (image);
            }
          break;
        }
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    {
      g_slist_free (trimmed_list);
      return NULL;
    }

  bottom_layer = layer;

  flatten_node = NULL;

  if (merge_type == CLAY_FLATTEN_IMAGE ||
      (clay_drawable_is_indexed (CLAY_DRAWABLE (layer)) &&
       ! clay_drawable_has_alpha (CLAY_DRAWABLE (layer))))
    {
      merge_layer = clay_layer_new (image, (x2 - x1), (y2 - y1),
                                    clay_image_get_layer_format (image, FALSE),
                                    clay_object_get_name (bottom_layer),
                                    CLAY_OPACITY_OPAQUE,
                                    clay_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          g_slist_free (trimmed_list);
          return NULL;
        }

      /*  get the background for compositing  */
      flatten_node = clay_gegl_create_flatten_node
        (clay_context_get_background (context),
         clay_layer_get_real_composite_space (bottom_layer));
    }
  else
    {
      /*  The final merged layer inherits the name of the bottom most layer
       *  and the resulting layer has an alpha channel whether or not the
       *  original did. Opacity is set to 100% and the MODE is set to normal.
       */

      merge_layer =
        clay_layer_new (image, (x2 - x1), (y2 - y1),
                        clay_drawable_get_format_with_alpha (CLAY_DRAWABLE (bottom_layer)),
                        clay_object_get_name (bottom_layer),
                        CLAY_OPACITY_OPAQUE,
                        clay_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          g_slist_free (trimmed_list);
          return NULL;
        }
    }

  if (merge_type == CLAY_FLATTEN_IMAGE)
    {
      position = 0;
    }
  else
    {
      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      position =
        clay_container_get_n_children (container) -
        clay_container_get_child_index (container, CLAY_OBJECT (bottom_layer));
    }

  clay_item_set_offset (CLAY_ITEM (merge_layer), x1, y1);

  offset_node = gegl_node_new_child (node,
                                     "operation", "gegl:translate",
                                     "x",         (gdouble) -x1,
                                     "y",         (gdouble) -y1,
                                     NULL);

  if (flatten_node)
    {
      gegl_node_add_child (node, flatten_node);
      g_object_unref (flatten_node);

      gegl_node_link_many (source_node, flatten_node, offset_node, NULL);
    }
  else
    {
      gegl_node_link_many (source_node, offset_node, NULL);
    }

  /*  Disconnect the bottom-layer node's input, unless it's a
   *  pass-through group.
   */
  if (clay_layer_get_mode (bottom_layer) != CLAY_LAYER_MODE_PASS_THROUGH)
    {
      last_node        = clay_filter_get_node (CLAY_FILTER (bottom_layer));
      last_node_source = gegl_node_get_producer (last_node, "input", NULL);

      gegl_node_disconnect (last_node, "input");
    }

  /*  Render the graph into the merge layer  */
  clay_gegl_apply_operation (NULL, progress, undo_desc, offset_node,
                             clay_drawable_get_buffer (
                               CLAY_DRAWABLE (merge_layer)),
                             NULL, FALSE);

  /*  Reconnect the bottom-layer node's input  */
  if (last_node_source)
    gegl_node_link (last_node_source, last_node);

  /*  Clean up the graph  */
  gegl_node_remove_child (node, offset_node);

  if (flatten_node)
    gegl_node_remove_child (node, flatten_node);

  /* Copy the tattoo and parasites of the bottom layer to the new layer */
  clay_item_set_tattoo (CLAY_ITEM (merge_layer),
                        clay_item_get_tattoo (CLAY_ITEM (bottom_layer)));

  parasites = clay_item_get_parasites (CLAY_ITEM (bottom_layer));
  parasites = clay_parasite_list_copy (parasites);
  clay_item_set_parasites (CLAY_ITEM (merge_layer), parasites);
  g_object_unref (parasites);

  for (layers = trimmed_list; layers; layers = g_slist_next (layers))
    {
      /* Remove the sisters below merged pass-through group layers. */
      if (clay_viewable_get_children (CLAY_VIEWABLE (layers->data)) &&
          clay_layer_get_mode (layers->data) == CLAY_LAYER_MODE_PASS_THROUGH)
        {
          ClayLayer     *parent;
          ClayContainer *stack;
          GList         *iter;
          gboolean       remove    = FALSE;
          GList         *to_remove = NULL;

          parent = clay_layer_get_parent (layers->data);
          if (parent)
            stack = clay_viewable_get_children (CLAY_VIEWABLE (parent));
          else
            stack = clay_image_get_layers (image);

          for (iter = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (stack)); iter; iter = iter->next)
            {
              if (iter->data == layers->data)
                remove = TRUE;
              else if (remove && clay_item_get_visible (CLAY_ITEM (iter->data)))
                to_remove = g_list_prepend (to_remove, iter->data);
            }

          if (layers->data == bottom_layer)
            n_bottom_removed = g_list_length (to_remove);

          for (iter = to_remove; iter; iter = iter->next)
            clay_image_remove_layer (image, iter->data, TRUE, NULL);

          g_list_free (to_remove);
        }

      /*  Remove the merged layers from the image  */
      clay_image_remove_layer (image, layers->data, TRUE, NULL);
    }

  clay_item_set_visible (CLAY_ITEM (merge_layer), TRUE, FALSE);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == CLAY_FLATTEN_IMAGE)
    {
      GList *list = clay_image_get_layer_iter (image);

      while (list)
        {
          layer = list->data;

          list = g_list_next (list);
          clay_image_remove_layer (image, layer, TRUE, NULL);
        }

      clay_image_add_layer (image, merge_layer, parent,
                            position, TRUE);
    }
  else
    {
      /*  Add the layer to the image  */
      clay_image_add_layer (image, merge_layer, parent,
                            clay_container_get_n_children (container) +
                            n_bottom_removed - position + 1,
                            TRUE);
    }

  clay_drawable_update (CLAY_DRAWABLE (merge_layer), 0, 0, -1, -1);
  g_slist_free (trimmed_list);

  return merge_layer;
}
