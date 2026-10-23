/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayexportoptions.c
 * Copyright (C) 2024 Alx Sa.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"

#include "clayexportoptions.h"

#include "libclay-intl.h"


/* export helper functions */

typedef void (* ExportFunc) (ClayImage    *image,
                             GList       **drawables);

/* the export action structure */
typedef struct
{
  ExportFunc   default_action;
  ExportFunc   alt_action;
  const gchar *reason;
  const gchar *possibilities[2];
  gint         choice;
} ExportAction;


/* the functions that do the actual export */

static void
export_merge (ClayImage  *image,
              GList     **drawables)
{
  GList     *layers;
  GList     *iter;
  ClayLayer *floating_sel;
  gint32     nvisible = 0;

  /* Convert floating selection to layer if it exists */
  floating_sel = clay_image_get_floating_sel (image);
  if (floating_sel)
    clay_floating_sel_to_layer (floating_sel);

  layers = clay_image_list_layers (image);

  for (iter = layers; iter; iter = g_list_next (iter))
    {
      if (clay_item_get_visible (CLAY_ITEM (iter->data)))
        nvisible++;
    }

  if (nvisible <= 1)
    {
      ClayLayer     *transp;
      ClayImageType  layer_type;

      /* if there is only one (or zero) visible layer, add a new
       * transparent layer that has the same size as the canvas.  The
       * merge that follows will ensure that the offset, opacity and
       * size are correct
       */
      switch (clay_image_get_base_type (image))
        {
        case CLAY_RGB:
          layer_type = CLAY_RGBA_IMAGE;
          break;
        case CLAY_GRAY:
          layer_type = CLAY_GRAYA_IMAGE;
          break;
        case CLAY_INDEXED:
          layer_type = CLAY_INDEXEDA_IMAGE;
          break;
        default:
          /* In case we add a new type in future. */
          g_return_if_reached ();
        }
      transp = clay_layer_new (image, "-",
                               clay_image_get_width (image),
                               clay_image_get_height (image),
                               layer_type,
                               100.0, CLAY_LAYER_MODE_NORMAL);
      clay_image_insert_layer (image, transp, NULL, 1);
      clay_selection_none (image);
      clay_drawable_edit_clear (CLAY_DRAWABLE (transp));
      nvisible++;
    }

  if (nvisible > 1)
    {
      ClayLayer *merged;

      merged = clay_image_merge_visible_layers (image, CLAY_CLIP_TO_IMAGE);

      g_return_if_fail (merged != NULL);

      *drawables = g_list_prepend (NULL, merged);

      g_list_free (layers);

      layers = clay_image_list_layers (image);

      /*  make sure that the merged drawable matches the image size  */
      if (clay_drawable_get_width   (CLAY_DRAWABLE (merged)) !=
          clay_image_get_width  (image) ||
          clay_drawable_get_height  (CLAY_DRAWABLE (merged)) !=
          clay_image_get_height (image))
        {
          gint off_x, off_y;

          clay_drawable_get_offsets (CLAY_DRAWABLE (merged), &off_x, &off_y);
          clay_layer_resize (merged,
                             clay_image_get_width (image),
                             clay_image_get_height (image),
                             off_x, off_y);
        }
    }

  /* remove any remaining (invisible) layers */
  for (iter = layers; iter; iter = iter->next)
    {
      if (! g_list_find (*drawables, iter->data))
        clay_image_remove_layer (image, iter->data);
    }

  g_list_free (layers);
}

static void
export_flatten (ClayImage  *image,
                GList     **drawables)
{
  ClayLayer *flattened;

  flattened = clay_image_flatten (image);

  if (flattened != NULL)
    *drawables = g_list_prepend (NULL, flattened);
}

static void
export_merge_layer_effects_rec (GList *layers)
{
  GList *iter;

  for (iter = layers; iter; iter = g_list_next (iter))
    if (clay_item_is_group (iter->data))
      {
        GList *children = clay_item_list_children (iter->data);

        export_merge_layer_effects_rec (children);

        g_list_free (children);
      }
    else
      {
        clay_drawable_merge_filters (CLAY_DRAWABLE (iter->data));
      }
}

static void
export_merge_layer_effects (ClayImage  *image,
                            GList     **drawables)
{
  GList *layers;

  layers = clay_image_list_layers (image);
  export_merge_layer_effects_rec (layers);
  g_list_free (layers);
}

static void
export_remove_alpha (ClayImage  *image,
                     GList     **drawables)
{
  GList  *layers;
  GList  *iter;

  layers = clay_image_list_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_drawable_has_alpha (CLAY_DRAWABLE (iter->data)))
        clay_layer_flatten (iter->data);
    }

  g_list_free (layers);
}

static void
export_apply_masks (ClayImage  *image,
                    GList     **drawables)
{
  GList  *layers;
  GList  *iter;

  layers = clay_image_list_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayerMask *mask;

      mask = clay_layer_get_mask (iter->data);

      if (mask)
        {
          /* we can't apply the mask directly to a layer group, so merge it
           * first
           */
          if (clay_item_is_group (iter->data))
            iter->data = clay_group_layer_merge (iter->data);

          clay_layer_remove_mask (iter->data, CLAY_MASK_APPLY);
        }
    }

  g_list_free (layers);
}

static void
export_convert_rgb (ClayImage  *image,
                    GList     **drawables)
{
  clay_image_convert_rgb (image);
}

static void
export_convert_grayscale (ClayImage  *image,
                          GList     **drawables)
{
  clay_image_convert_grayscale (image);
}

static void
export_convert_indexed (ClayImage  *image,
                        GList     **drawables)
{
  GList    *layers;
  GList    *iter;
  gboolean  has_alpha = FALSE;

  /* check alpha */
  layers = clay_image_list_layers (image);

  for (iter = *drawables; iter; iter = iter->next)
    {
      if (clay_drawable_has_alpha (iter->data))
        {
          has_alpha = TRUE;
          break;
        }
    }

  if (layers || has_alpha)
    clay_image_convert_indexed (image,
                                CLAY_CONVERT_DITHER_NONE,
                                CLAY_CONVERT_PALETTE_GENERATE,
                                255, FALSE, FALSE, "");
  else
    clay_image_convert_indexed (image,
                                CLAY_CONVERT_DITHER_NONE,
                                CLAY_CONVERT_PALETTE_GENERATE,
                                256, FALSE, FALSE, "");
  g_list_free (layers);
}

static void
export_convert_bitmap (ClayImage  *image,
                       GList     **drawables)
{
  if (clay_image_get_base_type (image) == CLAY_INDEXED)
    clay_image_convert_rgb (image);

  clay_image_convert_indexed (image,
                              CLAY_CONVERT_DITHER_FS,
                              CLAY_CONVERT_PALETTE_GENERATE,
                              2, FALSE, FALSE, "");
}

static void
export_add_alpha (ClayImage  *image,
                  GList     **drawables)
{
  GList  *layers;
  GList  *iter;

  layers = clay_image_list_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (! clay_drawable_has_alpha (CLAY_DRAWABLE (iter->data)))
        clay_layer_add_alpha (CLAY_LAYER (iter->data));
    }

  g_list_free (layers);
}

static void
export_crop_image (ClayImage  *image,
                   GList     **drawables)
{
  clay_image_crop (image,
                   clay_image_get_width  (image),
                   clay_image_get_height (image),
                   0, 0);
}

static void
export_resize_image (ClayImage  *image,
                     GList     **drawables)
{
  clay_image_resize_to_layers (image);
}

static void
export_void (ClayImage  *image,
             GList     **drawables)
{
  /* do nothing */
}


/* a set of predefined actions */

static ExportAction export_action_merge =
{
  export_merge,
  NULL,
  N_("%s plug-in can't handle layers"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_merge_single =
{
  export_merge,
  NULL,
  N_("%s plug-in can't handle layer offsets, size or opacity"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_animate_or_merge =
{
  NULL,
  export_merge,
  N_("%s plug-in can only handle layers as animation frames"),
  { N_("Save as Animation"), N_("Merge Visible Layers") },
  0
};

static ExportAction export_action_animate_or_flatten =
{
  NULL,
  export_flatten,
  N_("%s plug-in can only handle layers as animation frames"),
  { N_("Save as Animation"), N_("Flatten Image") },
  0
};

static ExportAction export_action_merge_or_flatten =
{
  export_flatten,
  export_merge,
  N_("%s plug-in can't handle layers"),
  { N_("Flatten Image"), N_("Merge Visible Layers") },
  1
};

static ExportAction export_action_flatten =
{
  export_flatten,
  NULL,
  N_("%s plug-in can't handle transparency"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_merge_layer_effects =
{
  export_merge_layer_effects,
  NULL,
  N_("%s plug-in can't handle layer effects"),
  { N_("Merge Layer Effects"), NULL },
  0
};

static ExportAction export_action_remove_alpha =
{
  export_remove_alpha,
  NULL,
  N_("%s plug-in can't handle transparent layers"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_apply_masks =
{
  export_apply_masks,
  NULL,
  N_("%s plug-in can't handle layer masks"),
  { N_("Apply Layer Masks"), NULL },
  0
};

static ExportAction export_action_convert_rgb =
{
  export_convert_rgb,
  NULL,
  N_("%s plug-in can only handle RGB images"),
  { N_("Convert to RGB"), NULL },
  0
};

static ExportAction export_action_convert_grayscale =
{
  export_convert_grayscale,
  NULL,
  N_("%s plug-in can only handle grayscale images"),
  { N_("Convert to Grayscale"), NULL },
  0
};

static ExportAction export_action_convert_indexed =
{
  export_convert_indexed,
  NULL,
  N_("%s plug-in can only handle indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_bitmap =
{
  export_convert_bitmap,
  NULL,
  N_("%s plug-in can only handle bitmap (two color) indexed images"),
  { N_("Convert to Indexed using bitmap default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_rgb_or_grayscale =
{
  export_convert_rgb,
  export_convert_grayscale,
  N_("%s plug-in can only handle RGB or grayscale images"),
  { N_("Convert to RGB"), N_("Convert to Grayscale")},
  0
};

static ExportAction export_action_convert_rgb_or_indexed =
{
  export_convert_rgb,
  export_convert_indexed,
  N_("%s plug-in can only handle RGB or indexed images"),
  { N_("Convert to RGB"), N_("Convert to Indexed using default settings\n"
                             "(Do it manually to tune the result)")},
  0
};

static ExportAction export_action_convert_indexed_or_grayscale =
{
  export_convert_indexed,
  export_convert_grayscale,
  N_("%s plug-in can only handle grayscale or indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"),
    N_("Convert to Grayscale") },
  0
};

static ExportAction export_action_add_alpha =
{
  export_add_alpha,
  NULL,
  N_("%s plug-in needs an alpha channel"),
  { N_("Add Alpha Channel"), NULL},
  0
};

static ExportAction export_action_crop_or_resize =
{
  export_crop_image,
  export_resize_image,
  N_("%s plug-in needs to crop the layers to the image bounds"),
  { N_("Crop Layers"), N_("Resize Image to Layers")},
  0
};


static ExportFunc
export_action_get_func (const ExportAction *action)
{
  if (action->choice == 0 && action->default_action)
    {
      return action->default_action;
    }

  if (action->choice == 1 && action->alt_action)
    {
      return action->alt_action;
    }

  return export_void;
}

static void
export_action_perform (const ExportAction *action,
                       ClayImage          *image,
                       GList             **drawables)
{
  export_action_get_func (action) (image, drawables);
}

/**
 * clay_export_options_get_image:
 * @options: (transfer none): The #ClayExportOptions object.
 * @image: (inout) (transfer none): the image.
 *
 * Takes an image to be exported, possibly creating a temporary copy
 * modified according to export settings in @options (such as the
 * capabilities of the export format).
 *
 * If necessary, a copy is created, converted and modified, @image
 * changed to point to the new image and the procedure returns
 * [enum@Clay.ExportReturn.EXPORT].
 * In this case, you must take care of deleting the created image using
 * [method@Image.delete] once the image has been exported, unless you
 * were planning to display it with [ctor@Display.new], or you will leak
 * memory.
 *
 * If [enum@Clay.ExportReturn.IGNORE] is returned, then @image is still the
 * original image. You should neither modify it, nor should you delete
 * it in the end. If you wish to temporarily modify the image before
 * export anyway, call [method@Image.duplicate] when
 * [enum@Clay.ExportReturn.IGNORE] was returned.
 *
 * Returns: An enum of #ClayExportReturn.
 *
 * Since: 3.0
 **/
ClayExportReturn
clay_export_options_get_image (ClayExportOptions  *options,
                               ClayImage         **image)
{
  GSList                 *actions = NULL;
  ClayImageBaseType       type;
  GList                  *layers;
  gint                    n_layers;
  GList                  *iter;
  ClayExportCapabilities  capabilities         = 0;
  gboolean                added_flatten        = FALSE;
  gboolean                has_layer_masks      = FALSE;
  gboolean                background_has_alpha = TRUE;
  ClayExportReturn        retval               = CLAY_EXPORT_IGNORE;

  g_return_val_if_fail (image && clay_image_is_valid (*image), CLAY_EXPORT_IGNORE);
  g_return_val_if_fail (CLAY_IS_EXPORT_OPTIONS (options), CLAY_EXPORT_IGNORE);

  /* Get capabilities from ExportOptions */
  g_object_get (options, "capabilities", &capabilities, NULL);

  g_return_val_if_fail (capabilities & (CLAY_EXPORT_CAN_HANDLE_RGB |
                                        CLAY_EXPORT_CAN_HANDLE_GRAY |
                                        CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                        CLAY_EXPORT_CAN_HANDLE_BITMAP),
                        CLAY_EXPORT_IGNORE);


  /* do some sanity checks */
  if (capabilities & CLAY_EXPORT_NEEDS_ALPHA)
    capabilities |= CLAY_EXPORT_CAN_HANDLE_ALPHA;

  if (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
    capabilities |= CLAY_EXPORT_CAN_HANDLE_LAYERS;

  if (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS)
    capabilities |= CLAY_EXPORT_CAN_HANDLE_LAYERS;

  /* Merge down layer effects for non-project file formats */
  if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS))
    actions = g_slist_prepend (actions, &export_action_merge_layer_effects);

  /* check alpha and layer masks */
  layers   = clay_image_list_layers (*image);
  n_layers = g_list_length (layers);

  if (n_layers < 1)
    {
      g_list_free (layers);
      return FALSE;
    }

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *layer = CLAY_LAYER (iter->data);

      if (clay_drawable_has_alpha (CLAY_DRAWABLE (layer)))
        {
          if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_ALPHA))
            {
              if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS))
                {
                  actions = g_slist_prepend (actions, &export_action_flatten);
                  added_flatten = TRUE;
                  break;
                }
              else
                {
                  actions = g_slist_prepend (actions, &export_action_remove_alpha);
                  break;
                }
            }
        }
      else
        {
          /*  If this is the last layer, it's visible and has no alpha
           *  channel, then the image has a "flat" background
           */
          if (iter->next == NULL && clay_item_get_visible (CLAY_ITEM (layer)))
            background_has_alpha = FALSE;

          if (capabilities & CLAY_EXPORT_NEEDS_ALPHA)
            {
              actions = g_slist_prepend (actions, &export_action_add_alpha);
              break;
            }
        }
    }

  if (! added_flatten)
    {
      for (iter = layers; iter; iter = iter->next)
        {
          if (clay_layer_get_mask (iter->data))
            has_layer_masks = TRUE;
        }
    }

  if (! added_flatten)
    {
      ClayLayer *layer = CLAY_LAYER (layers->data);
      GList     *children;

      children = clay_item_list_children (CLAY_ITEM (layer));

      if ((capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS) &&
          (capabilities & CLAY_EXPORT_NEEDS_CROP))
        {
          GeglRectangle image_bounds;
          gboolean      needs_crop = FALSE;

          image_bounds.x      = 0;
          image_bounds.y      = 0;
          image_bounds.width  = clay_image_get_width  (*image);
          image_bounds.height = clay_image_get_height (*image);

          for (iter = layers; iter; iter = iter->next)
            {
              ClayDrawable  *drawable = iter->data;
              GeglRectangle  layer_bounds;

              clay_drawable_get_offsets (drawable,
                                     &layer_bounds.x, &layer_bounds.y);

              layer_bounds.width  = clay_drawable_get_width  (drawable);
              layer_bounds.height = clay_drawable_get_height (drawable);

              if (! gegl_rectangle_contains (&image_bounds, &layer_bounds))
                {
                  needs_crop = TRUE;

                  break;
                }
            }

          if (needs_crop)
            {
              actions = g_slist_prepend (actions,
                                         &export_action_crop_or_resize);
            }
        }

      /* check if layer size != canvas size, opacity != 100%, or offsets != 0 */
      if (g_list_length (layers) == 1       &&
          ! children                        &&
          ! (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS))
        {
          ClayDrawable *drawable = layers->data;
          gint          offset_x;
          gint          offset_y;

          clay_drawable_get_offsets (drawable, &offset_x, &offset_y);

          if ((clay_layer_get_opacity (CLAY_LAYER (drawable)) < 100.0) ||
              (clay_image_get_width (*image) !=
               clay_drawable_get_width (drawable))            ||
              (clay_image_get_height (*image) !=
               clay_drawable_get_height (drawable))           ||
              offset_x || offset_y)
            {
              if (capabilities & CLAY_EXPORT_CAN_HANDLE_ALPHA)
                {
                  actions = g_slist_prepend (actions,
                                             &export_action_merge_single);
                }
              else
                {
                  actions = g_slist_prepend (actions,
                                             &export_action_flatten);
                }
            }
        }
      /* check multiple layers */
      else if (layers && layers->next != NULL)
        {
          if (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
            {
              if (background_has_alpha ||
                  capabilities & CLAY_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_animate_or_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_animate_or_flatten);
            }
          else if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS))
            {
              if (capabilities & CLAY_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_merge_or_flatten);
            }
        }
      /* check for a single toplevel layer group */
      else if (children)
        {
          if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYERS))
            {
              if (capabilities & CLAY_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_merge_or_flatten);
            }
        }

      g_list_free (children);

      /* check layer masks */
      if (has_layer_masks &&
          ! (capabilities & CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS))
        actions = g_slist_prepend (actions, &export_action_apply_masks);
    }

  g_list_free (layers);

  /* check the image type */
  type = clay_image_get_base_type (*image);
  switch (type)
    {
    case CLAY_RGB:
      if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_RGB))
        {
          if ((capabilities & CLAY_EXPORT_CAN_HANDLE_INDEXED) &&
              (capabilities & CLAY_EXPORT_CAN_HANDLE_GRAY))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed_or_grayscale);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_INDEXED)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_GRAY)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_grayscale);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_BITMAP)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_bitmap);
        }
      break;

    case CLAY_GRAY:
      if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_GRAY))
        {
          if ((capabilities & CLAY_EXPORT_CAN_HANDLE_RGB) &&
              (capabilities & CLAY_EXPORT_CAN_HANDLE_INDEXED))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb_or_indexed);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_RGB)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_INDEXED)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_BITMAP)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_bitmap);
        }
      break;

    case CLAY_INDEXED:
      if (! (capabilities & CLAY_EXPORT_CAN_HANDLE_INDEXED))
        {
          if ((capabilities & CLAY_EXPORT_CAN_HANDLE_RGB) &&
              (capabilities & CLAY_EXPORT_CAN_HANDLE_GRAY))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb_or_grayscale);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_RGB)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_GRAY)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_grayscale);
          else if (capabilities & CLAY_EXPORT_CAN_HANDLE_BITMAP)
            {
              gint n_colors;

              n_colors = clay_palette_get_color_count (clay_image_get_palette (*image));

              if (n_colors > 2)
                actions = g_slist_prepend (actions,
                                           &export_action_convert_bitmap);
            }
        }
      break;
    }

  if (actions)
    {
      actions = g_slist_reverse (actions);

      retval = CLAY_EXPORT_EXPORT;
    }
  else
    {
      retval = CLAY_EXPORT_IGNORE;
    }

  if (retval == CLAY_EXPORT_EXPORT)
    {
      GSList      *list;
      GList       *drawables_in;
      GList       *drawables_out;
      ClayChannel *selection_copy;

      *image = clay_image_duplicate (*image);
      drawables_in  = clay_image_list_selected_layers (*image);
      drawables_out = drawables_in;

      /* Temporarily remove selection so that any merged filters
       * aren't affected by it */
      selection_copy = CLAY_CHANNEL (clay_selection_save (*image));
      clay_selection_none (*image);

      clay_image_undo_disable (*image);

      for (list = actions; list; list = list->next)
        {
          export_action_perform (list->data, *image, &drawables_out);

          if (drawables_in != drawables_out)
            {
              g_list_free (drawables_in);
              drawables_in = drawables_out;
            }
        }

      g_list_free (drawables_out);

      clay_image_select_item (*image, CLAY_CHANNEL_OP_REPLACE,
                              CLAY_ITEM (selection_copy));
      clay_image_remove_channel (*image, selection_copy);
    }

  g_slist_free (actions);

  return retval;
}
