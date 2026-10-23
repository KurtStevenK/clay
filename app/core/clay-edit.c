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

#include "core-types.h"

#include "clay.h"
#include "clay-edit.h"
#include "claybuffer.h"
#include "claycontext.h"
#include "claydrawable-edit.h"
#include "clayerror.h"
#include "claygrouplayer.h"
#include "clayimage.h"
#include "clayimage-duplicate.h"
#include "clayimage-merge.h"
#include "clayimage-new.h"
#include "clayimage-resize.h"
#include "clayimage-undo.h"
#include "claylayer-floating-selection.h"
#include "claylayer-new.h"
#include "claylayermask.h"
#include "claylist.h"
#include "claypickable.h"
#include "clayselection.h"

#include "clay-intl.h"


/*  local function protypes  */

static ClayBuffer   * clay_edit_extract            (ClayImage     *image,
                                                    GList         *pickables,
                                                    ClayContext   *context,
                                                    gboolean       cut_pixels,
                                                    GError       **error);
static ClayDrawable * clay_edit_paste_get_top_item (GList         *drawables);


/*  public functions  */

ClayObject *
clay_edit_cut (ClayImage     *image,
               GList         *drawables,
               ClayContext   *context,
               GError       **error)
{
  GList    *iter;
  gboolean  layers_only = TRUE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    if (! CLAY_IS_LAYER (iter->data))
      {
        layers_only = FALSE;
        break;
      }

  if (layers_only)
    {
      gchar    *undo_label;
      gboolean  success = TRUE;

      undo_label = g_strdup_printf (ngettext ("Cut Layer", "Cut %d Layers",
                                              g_list_length (drawables)),
                                    g_list_length (drawables));
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_CUT,
                                   undo_label);
      g_free (undo_label);

      /* With a layer, a 'cut' is the same as a 'copy' and
       * then deleting layers/clearing selections
       */
      if (clay_edit_copy (image, drawables, context, TRUE, error))
        {
          for (iter = drawables; iter; iter = iter->next)
            {
              if (! clay_channel_is_empty (clay_image_get_mask (image)))
                {
                  clay_drawable_edit_clear (CLAY_DRAWABLE (iter->data), context);
                }
              else
                {
                  if (! clay_layer_is_floating_sel (iter->data))
                    clay_image_remove_layer (image, CLAY_LAYER (iter->data),
                                             TRUE, NULL);
                }
            }
        }
      else
        {
          success = FALSE;
        }

      clay_image_undo_group_end (image);

      return success ? CLAY_OBJECT (clay_get_clipboard_image (image->clay)) : NULL;
    }
  else
    {
      ClayBuffer *buffer;

      buffer = clay_edit_extract (image, drawables, context, TRUE, error);

      if (buffer)
        {
          clay_set_clipboard_buffer (image->clay, buffer);
          g_object_unref (buffer);

          return CLAY_OBJECT (clay_get_clipboard_buffer (image->clay));
        }
    }

  return NULL;
}

ClayObject *
clay_edit_copy (ClayImage     *image,
                GList         *drawables,
                ClayContext   *context,
                gboolean       copy_for_cut,
                GError       **error)
{
  GList    *iter;
  gboolean  drawables_are_layers = TRUE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (drawables != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (clay_item_is_attached (iter->data), NULL);

      if (! CLAY_IS_LAYER (iter->data))
        drawables_are_layers = FALSE;
    }

  /* Only accept multiple drawables for layers. */
  if (g_list_length (drawables) > 1 && ! drawables_are_layers)
    {
      if (error)
        {
          if (copy_for_cut)
            g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                 _("Cannot cut because multiple channels "
                                   "are selected."));
          else
            g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                 _("Cannot copy because multiple channels "
                                   "are selected."));
        }
      return NULL;
    }

  if (drawables_are_layers)
    {
      /* Special-casing the 1 layer with no selection case.
       * It allows us to save the whole layer with all pixels as stored,
       * not the rendered version of it.
       */
      ClayImage     *clip_image;
      ClayChannel   *clip_selection;
      GList         *remove = NULL;
      GeglRectangle  selection_bounds;
      gboolean       has_selection = FALSE;

      for (iter = drawables; iter; iter = iter->next)
        {
          gint x1;
          gint y1;
          gint x2;
          gint y2;

          if (! clay_item_mask_bounds (iter->data, &x1, &y1, &x2, &y2) ||
              (x1 != x2 && y1 != y2))
            break;
        }

      if (iter == NULL)
        {
          if (error)
            {
              if (copy_for_cut)
                g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                     _("Cannot cut because the selected region is empty."));
              else
                g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                     _("Cannot copy because the selected region is empty."));
            }
          return NULL;
        }

      drawables = g_list_copy (drawables);
      for (iter = drawables; iter; iter = iter->next)
        {
          GList *iter2;

          for (iter2 = drawables; iter2; iter2 = iter2->next)
            {
              if (iter2 == iter)
                continue;

              if (clay_viewable_is_ancestor (iter2->data, iter->data))
                {
                  /* When copying a layer group, all its children come
                   * with anyway.
                   */
                  remove = g_list_prepend (remove, iter);
                  break;
                }
            }
        }
      for (iter = remove; iter; iter = iter->next)
        drawables = g_list_delete_link (drawables, iter->data);

      g_list_free (remove);

      clip_image = clay_image_new_from_drawables (image->clay, drawables, TRUE, TRUE);
      clay_container_remove (image->clay->images, CLAY_OBJECT (clip_image));
      clay_set_clipboard_image (image->clay, clip_image);

      clip_selection = clay_image_get_mask (clip_image);
      if (! clay_channel_is_empty (clip_selection))
        {
          clay_item_bounds (CLAY_ITEM (clip_selection),
                            &selection_bounds.x, &selection_bounds.y,
                            &selection_bounds.width, &selection_bounds.height);

          /* Invert the selection. */
          clay_channel_invert (clip_selection, FALSE);

          /* Empty selection work the same as full selection for the
           * clay_drawable_edit_*() APIs. So as a special case, we check again
           * after inverting the selection (if the inverted selection is empty,
           * it meant we were in "all" selection case).
           * Otherwise we were ending up clearing everything with
           * clay_drawable_edit_clear() on the inverted "all" selection.
           */
          has_selection = (! clay_channel_is_empty (clip_selection));
        }

      if (has_selection)
        {
          GList *all_items;

          all_items = clay_image_get_layer_list (clip_image);

          for (iter = all_items; iter; iter = g_list_next (iter))
            {
              gint item_x;
              gint item_y;

              if (CLAY_IS_GROUP_LAYER (iter->data))
                /* Various destructive actions won't do a thing to a
                 * group layer anyway; except for clay_item_resize()
                 * which will resize children layers outside the
                 * selection (and will therefore break the loop).
                 */
                continue;

              clay_item_get_offset (CLAY_ITEM (iter->data), &item_x, &item_y);

              /* Even if the original layer may not have an alpha channel, the
               * selected data must always have one. First because a selection
               * is in some way an alpha channel when we copy (we may copy part
               * of a pixel, i.e. with transparency). Second because the
               * selection is not necessary rectangular, unlike layers. So when
               * we will clear, if we hadn't added an alpha channel, we'd end up
               * with background color all over the place.
               */
              clay_layer_add_alpha (CLAY_LAYER (iter->data));
              clay_drawable_edit_clear (CLAY_DRAWABLE (iter->data), context);

              /* Finally shrink the copied layer to selection bounds. */
              clay_item_resize (iter->data, context, CLAY_FILL_TRANSPARENT,
                                selection_bounds.width, selection_bounds.height,
                                item_x - selection_bounds.x, item_y - selection_bounds.y);
            }
          g_list_free (all_items);

          /* We need to store the original offsets before the image was
           * resized, in order to move it into the correct location for
           * in-place pasting.
           */
          g_object_set_data (G_OBJECT (clip_image),
                             "clay-edit-new-image-x",
                             GINT_TO_POINTER (selection_bounds.x));
          g_object_set_data (G_OBJECT (clip_image),
                             "clay-edit-new-image-y",
                             GINT_TO_POINTER (selection_bounds.y));
        }
      else
        {
          gint offset_x;
          gint offset_y;

          /* We need to store the minimum offset from the selected layers
           * in order to move it into the correct location for
           * in-place pasting.
           */
          clay_item_get_offset (CLAY_ITEM (drawables->data), &offset_x, &offset_y);

          for (iter = g_list_next (drawables); iter; iter = iter->next)
            {
              gint item_x;
              gint item_y;

              clay_item_get_offset (CLAY_ITEM (iter->data), &item_x, &item_y);

              offset_x = MIN (item_x, offset_x);
              offset_y = MIN (item_y, offset_y);
            }

          g_object_set_data (G_OBJECT (clip_image),
                             "clay-edit-new-image-x",
                             GINT_TO_POINTER (offset_x));
          g_object_set_data (G_OBJECT (clip_image),
                             "clay-edit-new-image-y",
                             GINT_TO_POINTER (offset_y));
        }
      g_list_free (drawables);

      clay_image_undo_disable (clip_image);
      clay_image_resize_to_layers (clip_image, context, NULL, NULL, NULL,
                                   NULL, NULL);
      clay_image_undo_enable (clip_image);

      /* Remove selection from the clipboard image. */
      clay_channel_clear (clip_selection, NULL, FALSE);
      g_object_unref (clip_image);

      return CLAY_OBJECT (clay_get_clipboard_image (image->clay));
    }
  else
    {
      ClayBuffer *buffer;

      buffer = clay_edit_extract (image, drawables, context, FALSE, error);

      if (buffer)
        {
          clay_set_clipboard_buffer (image->clay, buffer);
          g_object_unref (buffer);

          return CLAY_OBJECT (clay_get_clipboard_buffer (image->clay));
        }
    }

  return NULL;
}

ClayBuffer *
clay_edit_copy_visible (ClayImage    *image,
                        ClayContext  *context,
                        GError      **error)
{
  ClayBuffer *buffer;
  GList      *pickables;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pickables = g_list_prepend (NULL, image);
  buffer = clay_edit_extract (image, pickables, context, FALSE, error);
  g_list_free (pickables);

  if (buffer)
    {
      clay_set_clipboard_buffer (image->clay, buffer);
      g_object_unref (buffer);

      return clay_get_clipboard_buffer (image->clay);
    }

  return NULL;
}

static gboolean
clay_edit_paste_is_in_place (ClayPasteType paste_type)
{
  switch (paste_type)
    {
    case CLAY_PASTE_TYPE_FLOATING:
    case CLAY_PASTE_TYPE_FLOATING_INTO:
    case CLAY_PASTE_TYPE_NEW_LAYER:
    case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
      return FALSE;

    case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
    case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
    case CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE:
    case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
    case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      return TRUE;
    }

  g_return_val_if_reached (FALSE);
}

static gboolean
clay_edit_paste_is_floating (ClayPasteType  paste_type,
                             ClayDrawable  *drawable)
{
  switch (paste_type)
    {
    case CLAY_PASTE_TYPE_FLOATING:
    case CLAY_PASTE_TYPE_FLOATING_INTO:
    case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
    case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
      return TRUE;

    case CLAY_PASTE_TYPE_NEW_LAYER:
    case CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE:
      return FALSE;

    case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
    case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
    case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      if (CLAY_IS_LAYER_MASK (drawable))
        return TRUE;
      else
        return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

static GList *
clay_edit_paste_get_tagged_layers (ClayImage         *image,
                                   GList             *layers,
                                   GList             *returned_layers,
                                   const Babl        *floating_format,
                                   ClayImageBaseType  base_type,
                                   ClayPrecision      precision,
                                   ClayPasteType      paste_type)
{
  GList *iter;

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *layer;
      GType      layer_type;
      gboolean   copied = TRUE;

      switch (paste_type)
        {
        case CLAY_PASTE_TYPE_FLOATING:
        case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
        case CLAY_PASTE_TYPE_FLOATING_INTO:
        case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          /*  when pasting as floating make sure clay_item_convert()
           *  will turn group layers into normal layers, otherwise use
           *  the same layer type so e.g. text information gets
           *  preserved. See issue #2667.
           */
          if (CLAY_IS_GROUP_LAYER (iter->data))
            layer_type = CLAY_TYPE_LAYER;
          else
            layer_type = G_TYPE_FROM_INSTANCE (iter->data);
          break;

        case CLAY_PASTE_TYPE_NEW_LAYER:
        case CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE:
          layer_type = G_TYPE_FROM_INSTANCE (iter->data);
          break;

        default:
          g_return_val_if_reached (NULL);
        }

      if (CLAY_IS_GROUP_LAYER (iter->data))
        copied = (gboolean) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (iter->data),
                                                                "clay-image-copied-layer"));
      if (copied)
        {
          layer = CLAY_LAYER (clay_item_convert (CLAY_ITEM (iter->data),
                                                 image, layer_type));

          returned_layers = g_list_prepend (returned_layers, layer);

          switch (paste_type)
            {
            case CLAY_PASTE_TYPE_FLOATING:
            case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
            case CLAY_PASTE_TYPE_FLOATING_INTO:
            case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
              /*  when pasting as floating selection, get rid of the layer mask,
               *  and make sure the layer has the right format
               */
              if (clay_layer_get_mask (layer))
                clay_layer_apply_mask (layer, CLAY_MASK_DISCARD, FALSE);

              if (clay_drawable_get_format (CLAY_DRAWABLE (layer)) !=
                  floating_format)
                {
                  clay_drawable_convert_type (CLAY_DRAWABLE (layer), image,
                                              base_type, precision,
                                              TRUE, NULL, NULL,
                                              GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                              FALSE, NULL);
                }
              break;

            default:
              break;
            }
        }
      else
        {
          ClayContainer *container;

          container = clay_viewable_get_children (iter->data);
          returned_layers = clay_edit_paste_get_tagged_layers (image,
                                                               CLAY_LIST (container)->queue->head,
                                                               returned_layers,
                                                               floating_format,
                                                               base_type, precision, paste_type);
        }
    }

  return returned_layers;
}

static GList *
clay_edit_paste_get_layers (ClayImage     *image,
                            GList         *drawables,
                            ClayObject    *paste,
                            ClayPasteType *paste_type)
{
  GList      *layers = NULL;
  const Babl *floating_format;

  /*  change paste type to NEW_LAYER for cases where we can't attach a
   *  floating selection
   */
  if (g_list_length (drawables) != 1               ||
      clay_viewable_get_children (drawables->data) ||
      clay_item_is_content_locked (drawables->data, NULL))
    {
      if (clay_edit_paste_is_in_place (*paste_type))
        *paste_type = CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE;
      else
        *paste_type = CLAY_PASTE_TYPE_NEW_LAYER;
    }

  /*  floating pastes always have the pasted-to drawable's format with
   *  alpha; if drawable == NULL, user is pasting into an empty image
   */
  if (drawables && clay_edit_paste_is_floating (*paste_type, drawables->data))
    floating_format = clay_drawable_get_format_with_alpha (drawables->data);
  else
    floating_format = clay_image_get_layer_format (image, TRUE);

  if (CLAY_IS_IMAGE (paste))
    {
      ClayImageBaseType base_type;
      ClayPrecision     precision;

      layers = clay_image_get_layer_iter (CLAY_IMAGE (paste));

      /* If pasting into an empty image, use the image information.
       * Otherwise, get it from the selected drawables */
      if (drawables && drawables->data)
        {
          base_type = clay_drawable_get_base_type (drawables->data);
          precision = clay_drawable_get_precision (drawables->data);
        }
      else
        {
          base_type = clay_image_get_base_type (image);
          precision = clay_image_get_precision (image);
        }

      if (g_list_length (layers) > 1)
        {
          if (clay_edit_paste_is_in_place (*paste_type))
            *paste_type = CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE;
          else
            *paste_type = CLAY_PASTE_TYPE_NEW_LAYER;
        }

      layers = clay_edit_paste_get_tagged_layers (image, layers, NULL, floating_format,
                                                  base_type, precision,
                                                  *paste_type);
      layers = g_list_reverse (layers);
    }
  else if (CLAY_IS_BUFFER (paste))
    {
      ClayLayer *layer;

      layer = clay_layer_new_from_buffer (CLAY_BUFFER (paste), image,
                                          floating_format,
                                          _("Pasted Layer"),
                                          CLAY_OPACITY_OPAQUE,
                                          clay_image_get_default_new_layer_mode (image));

      layers = g_list_prepend (layers, layer);
    }

  return layers;
}

static void
clay_edit_paste_get_viewport_offset (ClayImage *image,
                                     GList     *drawables,
                                     GList     *pasted_layers,
                                     gint       viewport_x,
                                     gint       viewport_y,
                                     gint       viewport_width,
                                     gint       viewport_height,
                                     gint      *pasted_bbox_x,
                                     gint      *pasted_bbox_y,
                                     gint      *offset_x,
                                     gint      *offset_y)
{
  GList   *iter;
  gint     image_width;
  gint     image_height;
  /* Source: pasted layers */
  gint     source_width;
  gint     source_height;

  gint     x1             = G_MAXINT;
  gint     y1             = G_MAXINT;
  gint     x2             = G_MININT;
  gint     y2             = G_MININT;
  gboolean clamp_to_image = TRUE;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (pasted_layers != NULL);
  g_return_if_fail (offset_x != NULL);
  g_return_if_fail (offset_y != NULL);

  image_width  = clay_image_get_width  (image);
  image_height = clay_image_get_height (image);

  for (iter = pasted_layers; iter; iter = iter->next)
    {
      gint layer_off_x;
      gint layer_off_y;
      gint layer_width;
      gint layer_height;

      g_return_if_fail (CLAY_IS_VIEWABLE (iter->data));

      clay_item_get_offset (CLAY_ITEM (iter->data), &layer_off_x, &layer_off_y);
      clay_viewable_get_size (iter->data, &layer_width, &layer_height);

      x1 = MIN (layer_off_x, x1);
      y1 = MIN (layer_off_y, y1);
      x2 = MAX (layer_off_x + layer_width, x2);
      y2 = MAX (layer_off_y + layer_height, y2);
    }

  /* Source offset and dimensions: this is the bounding box for all layers which
   * we want to paste together, keeping their relative position to each others.
   */
  *pasted_bbox_x = x1;
  *pasted_bbox_y = y1;
  source_width = x2 - x1;
  source_height = y2 - y1;

  if (viewport_width  == image_width &&
      viewport_height == image_height)
    {
      /* if the whole image is visible, act as if there was no viewport */

      viewport_x      = 0;
      viewport_y      = 0;
      viewport_width  = 0;
      viewport_height = 0;
    }

  if (drawables)
    {
      /*  if pasting while 1 or more drawables are selected  */
      gboolean empty_target = TRUE;
      gint     target_x;
      gint     target_y;
      gint     target_width;
      gint     target_height;
      gint     paste_x, paste_y;
      gint     paste_width;
      gint     paste_height;
      gboolean have_mask;

      have_mask = ! clay_channel_is_empty (clay_image_get_mask (image));

      for (iter = drawables; iter; iter = iter->next)
        {
          ClayContainer *children;

          children = clay_viewable_get_children (iter->data);
          if (! children || clay_container_get_n_children (children) > 0)
            {
              empty_target = FALSE;
              break;
            }
        }

      if (empty_target)
        {
          /* treat empty layer groups as image-sized, use the selection
           * as target
           */
          clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                            &target_x, &target_y,
                            &target_width, &target_height);
        }
      else
        {
          gint x1 = G_MAXINT;
          gint y1 = G_MAXINT;
          gint x2 = G_MININT;
          gint y2 = G_MININT;

          for (iter = drawables; iter; iter = iter->next)
            {
              gint layer_off_x;
              gint layer_off_y;
              gint mask_off_x;
              gint mask_off_y;
              gint mask_width;
              gint mask_height;

              clay_item_get_offset (CLAY_ITEM (iter->data), &layer_off_x, &layer_off_y);
              /* This is the bounds relatively to the drawable. */
              clay_item_mask_intersect (CLAY_ITEM (iter->data),
                                        &mask_off_x, &mask_off_y,
                                        &mask_width, &mask_height);

              /* The bounds relatively to the image. */
              x1 = MIN (layer_off_x + mask_off_x, x1);
              y1 = MIN (layer_off_y + mask_off_y, y1);
              x2 = MAX (layer_off_x + mask_off_x + mask_width, x2);
              y2 = MAX (layer_off_y + mask_off_y + mask_height, y2);
            }

          target_x      = x1;
          target_y      = y1;
          target_width  = x2 - x1;
          target_height = y2 - y1;
        }

      if (! have_mask         &&    /* if we have no mask */
          viewport_width  > 0 &&    /* and we have a viewport */
          viewport_height > 0 &&
          (source_width  < target_width || /* and the paste is smaller than the target */
           source_height < target_height) &&

          /* and the viewport intersects with the target */
          clay_rectangle_intersect (viewport_x, viewport_y,
                                    viewport_width, viewport_height,
                                    target_x, target_x,
                                    target_width, target_height,
                                    &paste_x, &paste_y,
                                    &paste_width, &paste_height))
        {
          /*  center on the viewport  */
          *offset_x = viewport_x + (viewport_width - source_width)  / 2;
          *offset_y = viewport_y + (viewport_height- source_height) / 2;
        }
      else
        {
          /*  otherwise center on the target  */
          *offset_x = target_x + (target_width  - source_width)  / 2;
          *offset_y = target_y + (target_height - source_height) / 2;

          /*  and keep it that way  */
          clamp_to_image = FALSE;
        }
    }
  else if (viewport_width  > 0 &&  /* if we have a viewport */
           viewport_height > 0 &&
           (source_width  < image_width || /* and the paste is       */
            source_height < image_height)) /* smaller than the image */
    {
      /*  center on the viewport  */

      *offset_x = viewport_x + (viewport_width  - source_width)  / 2;
      *offset_y = viewport_y + (viewport_height - source_height) / 2;
    }
  else
    {
      /*  otherwise center on the image  */

      *offset_x = (image_width  - source_width)  / 2;
      *offset_y = (image_height - source_height) / 2;

      /*  and keep it that way  */
      clamp_to_image = FALSE;
    }

  if (clamp_to_image)
    {
      /*  Ensure that the pasted layer is always within the image, if it
       *  fits and aligned at top left if it doesn't. (See bug #142944).
       */
      *offset_x = MIN (*offset_x, image_width  - source_width);
      *offset_y = MIN (*offset_y, image_height - source_height);
      *offset_x = MAX (*offset_x, 0);
      *offset_y = MAX (*offset_y, 0);
    }
}

static GList *
clay_edit_paste_paste (ClayImage     *image,
                       GList         *drawables,
                       GList         *layers,
                       ClayPasteType  paste_type,
                       gboolean       use_offset,
                       gint           layers_bbox_x,
                       gint           layers_bbox_y,
                       gint           offset_x,
                       gint           offset_y)
{
  GList *iter;

  g_return_val_if_fail (paste_type > CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE ||
                        g_list_length (drawables) == 1, NULL);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                               C_("undo-type", "Paste"));

  for (iter = layers; iter; iter = iter->next)
    {
      /* Layers for in-place paste are already translated. */
      if (use_offset)
        clay_item_translate (CLAY_ITEM (iter->data),
                             offset_x - layers_bbox_x,
                             offset_y - layers_bbox_y, FALSE);

      switch (paste_type)
        {
        case CLAY_PASTE_TYPE_FLOATING:
        case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
          /*  if there is a selection mask clear it - this might not
           *  always be desired, but in general, it seems like the correct
           *  behavior
           */
          if (! clay_channel_is_empty (clay_image_get_mask (image)))
            clay_channel_clear (clay_image_get_mask (image), NULL, TRUE);

          /* fall thru */

        case CLAY_PASTE_TYPE_FLOATING_INTO:
        case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          floating_sel_attach (iter->data, drawables->data);
          break;

        case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
        case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
          if (g_list_length (drawables) == 1 && CLAY_IS_LAYER_MASK (drawables->data))
            {
              if (! clay_channel_is_empty (clay_image_get_mask (image)))
                clay_channel_clear (clay_image_get_mask (image), NULL, TRUE);
              floating_sel_attach (iter->data, drawables->data);
              break;
            }

          /* fall thru if not a layer mask */

        case CLAY_PASTE_TYPE_NEW_LAYER:
        case CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE:
            {
              ClayLayer *parent   = NULL;
              gint       position = 0;

              /*  always add on top of a passed layer, where we would attach
               *  a floating selection
               */
              if (g_list_length (drawables) > 0 && CLAY_IS_LAYER (drawables->data))
                {
                  ClayDrawable *top_drawable;

                  top_drawable = clay_edit_paste_get_top_item (drawables);
                  parent   = clay_layer_get_parent (CLAY_LAYER (top_drawable));
                  position = clay_item_get_index (CLAY_ITEM (top_drawable));
                }

              clay_image_add_layer (image, iter->data, parent, position, TRUE);
            }
          break;

        case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
        case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
          g_return_val_if_reached (NULL);
        }
    }

  clay_image_undo_group_end (image);

  return layers;
}

GList *
clay_edit_paste (ClayImage     *image,
                 GList         *drawables,
                 ClayObject    *paste,
                 ClayPasteType  paste_type,
                 ClayContext   *context,
                 gboolean       merged,
                 gint           viewport_x,
                 gint           viewport_y,
                 gint           viewport_width,
                 gint           viewport_height)
{
  GList    *layers;
  gboolean  use_offset     = FALSE;
  gint      layers_bbox_x  = 0;
  gint      layers_bbox_y  = 0;
  gint      offset_y       = 0;
  gint      offset_x       = 0;
  GType     drawables_type = G_TYPE_NONE;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (paste) || CLAY_IS_BUFFER (paste), NULL);

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (iter->data)), NULL);
      g_return_val_if_fail (clay_item_get_image (CLAY_ITEM (iter->data)) == image, NULL);
      g_return_val_if_fail (drawables_type == G_TYPE_NONE ||
                            g_type_is_a (G_OBJECT_TYPE (iter->data), drawables_type), NULL);

      if (drawables_type == G_TYPE_NONE)
        {
          if (CLAY_IS_CHANNEL (iter->data))
            drawables_type = CLAY_TYPE_CHANNEL;
          else
            drawables_type = CLAY_TYPE_LAYER;
        }

      /* Currently we can only paste to channels (named channels, layer
       * masks, selection, etc.) as floating.
       */
      if (CLAY_IS_CHANNEL (iter->data))
        {
          switch (paste_type)
            {
            case CLAY_PASTE_TYPE_NEW_LAYER:
            case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
            case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
              paste_type = CLAY_PASTE_TYPE_FLOATING;
              break;
            case CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE:
            case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
            case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
              paste_type = CLAY_PASTE_TYPE_FLOATING_IN_PLACE;
              break;
            default:
              break;
            }
        }
    }

  if (merged && CLAY_IS_IMAGE (paste))
    {
      ClayImage *tmp_image;

      tmp_image = clay_image_duplicate (CLAY_IMAGE (paste));
      clay_container_remove (image->clay->images, CLAY_OBJECT (tmp_image));
      clay_image_merge_visible_layers (tmp_image, context, CLAY_EXPAND_AS_NECESSARY,
                                       FALSE, FALSE, NULL);
      layers = g_list_copy (clay_image_get_layer_iter (tmp_image));

      /* The merge process should ensure that we get a single non-group and
       * no-mask layer.
       */
      g_return_val_if_fail (g_list_length (layers) == 1, NULL);

      layers->data = clay_item_convert (CLAY_ITEM (layers->data), image,
                                        G_TYPE_FROM_INSTANCE (layers->data));

      switch (paste_type)
        {
        case CLAY_PASTE_TYPE_FLOATING:
        case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
        case CLAY_PASTE_TYPE_FLOATING_INTO:
        case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          if (clay_drawable_get_format (CLAY_DRAWABLE (layers->data)) !=
              clay_drawable_get_format_with_alpha (CLAY_DRAWABLE (layers->data)))
            {
              clay_drawable_convert_type (CLAY_DRAWABLE (layers->data), image,
                                          clay_drawable_get_base_type (layers->data),
                                          clay_drawable_get_precision (layers->data),
                                          TRUE, NULL, NULL,
                                          GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                          FALSE, NULL);
            }
          break;

        default:
          break;
        }
      g_object_unref (tmp_image);
    }
  else
    {
      layers = clay_edit_paste_get_layers (image, drawables, paste, &paste_type);
    }

  if (! layers)
    return NULL;

  if (clay_edit_paste_is_in_place (paste_type))
    {
      offset_x = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paste),
                                                     "clay-edit-new-image-x"));
      offset_y = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paste),
                                                     "clay-edit-new-image-y"));

      if (offset_x != 0 || offset_y != 0)
        {
          use_offset = TRUE;
        }
      else if (CLAY_IS_BUFFER (paste))
        {
          ClayBuffer *buffer = CLAY_BUFFER (paste);

          use_offset = TRUE;
          offset_x   = buffer->offset_x;
          offset_y   = buffer->offset_y;
        }
    }
  else
    {
      use_offset = TRUE;
      clay_edit_paste_get_viewport_offset (image, drawables, layers,
                                           viewport_x,
                                           viewport_y,
                                           viewport_width,
                                           viewport_height,
                                           &layers_bbox_x,
                                           &layers_bbox_y,
                                           &offset_x,
                                           &offset_y);
    }

  return clay_edit_paste_paste (image, drawables, layers, paste_type,
                                use_offset, layers_bbox_x, layers_bbox_y,
                                offset_x, offset_y);
}

ClayImage *
clay_edit_paste_as_new_image (Clay        *clay,
                              ClayObject  *paste,
                              ClayContext *context)
{
  ClayImage *image = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (paste) || CLAY_IS_BUFFER (paste), NULL);

  if (CLAY_IS_IMAGE (paste))
    image = clay_image_duplicate (CLAY_IMAGE (paste));
  else if (CLAY_IS_BUFFER (paste))
    image = clay_image_new_from_buffer (clay, CLAY_BUFFER (paste));

  return image;
}

const gchar *
clay_edit_named_cut (ClayImage     *image,
                     const gchar   *name,
                     GList         *drawables,
                     ClayContext   *context,
                     GError       **error)
{
  ClayBuffer *buffer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = clay_edit_extract (image, drawables, context, TRUE, error);

  if (buffer)
    {
      clay_object_set_name (CLAY_OBJECT (buffer), name);
      clay_container_add (image->clay->named_buffers, CLAY_OBJECT (buffer));
      g_object_unref (buffer);

      return clay_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
clay_edit_named_copy (ClayImage     *image,
                      const gchar   *name,
                      GList         *drawables,
                      ClayContext   *context,
                      GError       **error)
{
  ClayBuffer *buffer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = clay_edit_extract (image, drawables, context, FALSE, error);

  if (buffer)
    {
      clay_object_set_name (CLAY_OBJECT (buffer), name);
      clay_container_add (image->clay->named_buffers, CLAY_OBJECT (buffer));
      g_object_unref (buffer);

      return clay_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
clay_edit_named_copy_visible (ClayImage    *image,
                              const gchar  *name,
                              ClayContext  *context,
                              GError      **error)
{
  ClayBuffer *buffer;
  GList      *pickables;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pickables = g_list_prepend (NULL, image);
  buffer = clay_edit_extract (image, pickables, context, FALSE, error);
  g_list_free (pickables);

  if (buffer)
    {
      clay_object_set_name (CLAY_OBJECT (buffer), name);
      clay_container_add (image->clay->named_buffers, CLAY_OBJECT (buffer));
      g_object_unref (buffer);

      return clay_object_get_name (buffer);
    }

  return NULL;
}


/*  private functions  */

/**
 * clay_edit_extract:
 * @image:
 * @pickables:
 * @context:
 * @cut_pixels:
 * @error:
 *
 * Extracts the selected part of @image from the list of @pickables.
 * If @cut_pixels is %TRUE, and there is only one pickable input, and if
 * this pickable is a #ClayDrawable, then the selected pixels will be
 * effectively erased from the input pickable.
 * Otherwise @cut_pixels has no additional effect.
 * Note that all @pickables must belong to the same @image.
 *
 * Returns: a #ClayBuffer of the selected part of @image as if only the
 * selected @pickables were present (composited according to their
 * properties, unless there is only one pickable, in which case direct
 * pixel information is used without composition).
 */
static ClayBuffer *
clay_edit_extract (ClayImage     *image,
                   GList         *pickables,
                   ClayContext   *context,
                   gboolean       cut_pixels,
                   GError       **error)
{
  GeglBuffer *buffer;
  gint        offset_x;
  gint        offset_y;

  g_return_val_if_fail (g_list_length (pickables) > 0, NULL);

  if (g_list_length (pickables) > 1 ||
      ! CLAY_IS_DRAWABLE (pickables->data))
    cut_pixels = FALSE;

  if (cut_pixels)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_CUT,
                                 C_("undo-type", "Cut"));

  /*  Cut/copy the mask portion from the image  */
  buffer = clay_selection_extract (CLAY_SELECTION (clay_image_get_mask (image)),
                                   pickables, context,
                                   cut_pixels, FALSE, FALSE,
                                   &offset_x, &offset_y, error);

  if (cut_pixels)
    clay_image_undo_group_end (image);

  if (buffer)
    {
      ClayBuffer *clay_buffer;
      gdouble     res_x;
      gdouble     res_y;

      clay_buffer = clay_buffer_new (buffer, _("Global Buffer"),
                                     offset_x, offset_y, FALSE);
      g_object_unref (buffer);

      clay_image_get_resolution (image, &res_x, &res_y);
      clay_buffer_set_resolution (clay_buffer, res_x, res_y);
      clay_buffer_set_unit (clay_buffer, clay_image_get_unit (image));

      if (CLAY_IS_COLOR_MANAGED (pickables->data))
        {
          ClayColorProfile *profile =
            clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (pickables->data));

          if (profile)
            clay_buffer_set_color_profile (clay_buffer, profile);
        }

      return clay_buffer;
    }

  return NULL;
}

/* Return the visually top item. */
static ClayDrawable *
clay_edit_paste_get_top_item (GList *drawables)
{
  GList        *iter;
  ClayDrawable *top      = NULL;
  GList        *top_path = NULL;

  for (iter = drawables; iter; iter = iter->next)
    {
      GList *path = clay_item_get_path (iter->data);

      if (top == NULL)
        {
          top      = iter->data;
          top_path = path;
          path     = NULL;
        }
      else
        {
          GList *p_iter;
          GList *tp_iter;

          for (p_iter = path, tp_iter = top_path; p_iter || tp_iter; p_iter = p_iter->next, tp_iter = tp_iter->next)
            {
              if (tp_iter == NULL)
                {
                  break;
                }
              else if (p_iter == NULL ||
                       GPOINTER_TO_UINT (p_iter->data) < GPOINTER_TO_UINT (tp_iter->data))
                {
                  g_list_free (top_path);
                  top_path = path;
                  path = NULL;
                  top = iter->data;
                  break;
                }
              else if (GPOINTER_TO_UINT (p_iter->data) > GPOINTER_TO_UINT (tp_iter->data))
                {
                  break;
                }
            }
        }
      g_list_free (path);
    }

  g_list_free (top_path);
  return top;
}
