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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clay-gegl-loops.h"

#include "path/claypath.h"

#include "clay.h"
#include "claychannel.h"
#include "clayguide.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-colormap.h"
#include "clayimage-duplicate.h"
#include "clayimage-grid.h"
#include "clayimage-guides.h"
#include "clayimage-metadata.h"
#include "clayimage-private.h"
#include "clayimage-undo.h"
#include "clayimage-sample-points.h"
#include "clayitemstack.h"
#include "claylayer.h"
#include "claylayermask.h"
#include "claylayer-floating-selection.h"
#include "clayparasitelist.h"
#include "claysamplepoint.h"


static void          clay_image_duplicate_resolution       (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_save_source_file (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_colormap         (ClayImage *image,
                                                            ClayImage *new_image);
static ClayItem    * clay_image_duplicate_item             (ClayItem  *item,
                                                            ClayImage *new_image);
static GList       * clay_image_duplicate_layers           (ClayImage *image,
                                                            ClayImage *new_image);
static GList       * clay_image_duplicate_channels         (ClayImage *image,
                                                            ClayImage *new_image);
static GList       * clay_image_duplicate_paths            (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_floating_sel     (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_mask             (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_components       (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_guides           (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_sample_points    (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_grid             (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_metadata         (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_quick_mask       (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_parasites        (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_color_profile    (ClayImage *image,
                                                            ClayImage *new_image);
static void          clay_image_duplicate_simulation_profile
                                                           (ClayImage *image,
                                                            ClayImage *new_image);


ClayImage *
clay_image_duplicate (ClayImage *image)
{
  ClayImage    *new_image;
  GList        *active_layers;
  GList        *active_channels;
  GList        *active_path;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  clay_set_busy_until_idle (image->clay);

  /*  Create a new image  */
  new_image = clay_create_image (image->clay,
                                 clay_image_get_width  (image),
                                 clay_image_get_height (image),
                                 clay_image_get_base_type (image),
                                 clay_image_get_precision (image),
                                 FALSE);
  clay_image_undo_disable (new_image);

  /*  Store the source uri to be used by the save dialog  */
  clay_image_duplicate_save_source_file (image, new_image);

  /*  Copy resolution information  */
  clay_image_duplicate_resolution (image, new_image);

  /*  Copy parasites first so we have a color profile  */
  clay_image_duplicate_parasites (image, new_image);
  clay_image_duplicate_color_profile (image, new_image);

  /*  Copy the simulation profile settings */
  clay_image_duplicate_simulation_profile (image, new_image);

  /*  Copy the colormap if necessary  */
  clay_image_duplicate_colormap (image, new_image);

  /*  Copy the layers  */
  active_layers = clay_image_duplicate_layers (image, new_image);

  /*  Copy the channels  */
  active_channels = clay_image_duplicate_channels (image, new_image);

  /*  Copy any path  */
  active_path = clay_image_duplicate_paths (image, new_image);

  /*  Copy floating layer  */
  clay_image_duplicate_floating_sel (image, new_image);

  /*  Copy the selection mask  */
  clay_image_duplicate_mask (image, new_image);

  /*  Set active layer, active channel, active path  */
  if (active_layers)
    clay_image_set_selected_layers (new_image, active_layers);

  if (active_channels)
    clay_image_set_selected_channels (new_image, active_channels);

  if (active_path)
    clay_image_set_selected_paths (new_image, active_path);

  /*  Copy state of all color components  */
  clay_image_duplicate_components (image, new_image);

  /*  Copy any guides  */
  clay_image_duplicate_guides (image, new_image);

  /*  Copy any sample points  */
  clay_image_duplicate_sample_points (image, new_image);

  /*  Copy the grid  */
  clay_image_duplicate_grid (image, new_image);

  /*  Copy the metadata  */
  clay_image_duplicate_metadata (image, new_image);

  /*  Copy the quick mask info  */
  clay_image_duplicate_quick_mask (image, new_image);

  clay_image_undo_enable (new_image);

  /*  Explicitly mark image as dirty, so that its dirty time is set  */
  clay_image_dirty (new_image, CLAY_DIRTY_ALL);

  /* XXX Without flushing the duplicated image, we had at least one case
   * where it wouldn't properly render the image (with empty
   * pass-through groups with layer effects, which I think is because we
   * have code believing the group is smaller that it really is, because
   * of the specificity of pass-through groups). See #13057.
   * So I'm not entirely happy of calling this here, which feels more
   * like a workaround than a real fix. But it will do for now.
   */
  clay_image_flush (new_image);

  return new_image;
}

static void
clay_image_duplicate_resolution (ClayImage *image,
                                 ClayImage *new_image)
{
  gdouble xres;
  gdouble yres;

  clay_image_get_resolution (image, &xres, &yres);
  clay_image_set_resolution (new_image, xres, yres);
  clay_image_set_unit (new_image, clay_image_get_unit (image));
}

static void
clay_image_duplicate_save_source_file (ClayImage *image,
                                       ClayImage *new_image)
{
  GFile *file = clay_image_get_file (image);

  if (file)
    g_object_set_data_full (G_OBJECT (new_image), "clay-image-source-file",
                            g_object_ref (file),
                            (GDestroyNotify) g_object_unref);
}

static void
clay_image_duplicate_colormap (ClayImage *image,
                               ClayImage *new_image)
{
  if (clay_image_get_base_type (new_image) == CLAY_INDEXED)
    clay_image_set_colormap_palette (new_image,
                                     clay_image_get_colormap_palette (image),
                                     FALSE);
}

static ClayItem *
clay_image_duplicate_item (ClayItem  *item,
                           ClayImage *new_image)
{
  ClayItem *new_item;

  new_item = clay_item_convert (item, new_image,
                                G_TYPE_FROM_INSTANCE (item));

  /*  Make sure the copied item doesn't say: "<old item> copy"  */
  clay_object_set_name (CLAY_OBJECT (new_item),
                        clay_object_get_name (item));

  return new_item;
}

static GList *
clay_image_duplicate_layers (ClayImage *image,
                             ClayImage *new_image)
{
  GList         *new_selected_layers = NULL;
  GList         *selected_paths      = NULL;
  GList         *selected_layers;
  ClayItemStack *new_item_stack;
  GList         *list;
  gint           count;

  selected_layers = clay_image_get_selected_layers (image);

  for (list = selected_layers; list; list = list->next)
    selected_paths = g_list_prepend (selected_paths,
                                     clay_item_get_path (list->data));

  for (list = clay_image_get_layer_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      ClayLayer *layer = list->data;
      ClayLayer *new_layer;

      if (clay_layer_is_floating_sel (layer))
        continue;

      new_layer = CLAY_LAYER (clay_image_duplicate_item (CLAY_ITEM (layer),
                                                         new_image));

      /*  Make sure that if the layer has a layer mask,
       *  its name isn't screwed up
       */
      if (new_layer->mask)
        clay_object_set_name (CLAY_OBJECT (new_layer->mask),
                              clay_object_get_name (layer->mask));

      clay_image_add_layer (new_image, new_layer,
                            NULL, count++, FALSE);
    }

  new_item_stack = CLAY_ITEM_STACK (clay_image_get_layers (new_image));
  for (list = selected_paths; list; list = list->next)
    new_selected_layers = g_list_prepend (new_selected_layers,
                                          clay_item_stack_get_item_by_path (new_item_stack, list->data));

  g_list_free_full (selected_paths, (GDestroyNotify) g_list_free);

  return new_selected_layers;
}

static GList *
clay_image_duplicate_channels (ClayImage *image,
                               ClayImage *new_image)
{
  GList *new_selected_channels = NULL;
  GList *selected_channels;
  GList *list;
  gint   count;

  selected_channels = clay_image_get_selected_channels (image);

  for (list = clay_image_get_channel_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      ClayChannel  *channel = list->data;
      ClayChannel  *new_channel;

      new_channel = CLAY_CHANNEL (clay_image_duplicate_item (CLAY_ITEM (channel),
                                                             new_image));

      if (g_list_find (selected_channels, channel))
        new_selected_channels = g_list_prepend (new_selected_channels, new_channel);

      clay_image_add_channel (new_image, new_channel,
                              NULL, count++, FALSE);
    }

  return new_selected_channels;
}

static GList *
clay_image_duplicate_paths (ClayImage *image,
                            ClayImage *new_image)
{
  GList *new_selected_path = NULL;
  GList *selected_path;
  GList *list;
  gint   count;

  selected_path = clay_image_get_selected_paths (image);

  for (list = clay_image_get_path_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      ClayPath *path = list->data;
      ClayPath *new_path;

      /* If the path is attached to a vector layer, it will be
       * duplicated already */
      if (clay_path_attached_to_vector_layer (path, image))
        continue;

      new_path = CLAY_PATH (clay_image_duplicate_item (CLAY_ITEM (path),
                                                       new_image));

      if (g_list_find (selected_path, path))
        new_selected_path = g_list_prepend (new_selected_path, new_path);


      clay_image_add_path (new_image, new_path,
                           NULL, count++, FALSE);
    }

  return new_selected_path;
}

static void
clay_image_duplicate_floating_sel (ClayImage *image,
                                   ClayImage *new_image)
{
  ClayLayer     *floating_sel;
  ClayDrawable  *floating_sel_drawable;
  GList         *floating_sel_path;
  ClayItemStack *new_item_stack;
  ClayLayer     *new_floating_sel;
  ClayDrawable  *new_floating_sel_drawable;

  floating_sel = clay_image_get_floating_selection (image);

  if (! floating_sel)
    return;

  floating_sel_drawable = clay_layer_get_floating_sel_drawable (floating_sel);

  if (CLAY_IS_LAYER_MASK (floating_sel_drawable))
    {
      ClayLayer *layer;

      layer = clay_layer_mask_get_layer (CLAY_LAYER_MASK (floating_sel_drawable));

      floating_sel_path = clay_item_get_path (CLAY_ITEM (layer));

      new_item_stack = CLAY_ITEM_STACK (clay_image_get_layers (new_image));
    }
  else
    {
      floating_sel_path = clay_item_get_path (CLAY_ITEM (floating_sel_drawable));

      if (CLAY_IS_LAYER (floating_sel_drawable))
        new_item_stack = CLAY_ITEM_STACK (clay_image_get_layers (new_image));
      else
        new_item_stack = CLAY_ITEM_STACK (clay_image_get_channels (new_image));
    }

  /*  adjust path[0] for the floating layer missing in new_image  */
  floating_sel_path->data =
    GUINT_TO_POINTER (GPOINTER_TO_UINT (floating_sel_path->data) - 1);

  if (CLAY_IS_LAYER (floating_sel_drawable))
    {
      new_floating_sel =
        CLAY_LAYER (clay_image_duplicate_item (CLAY_ITEM (floating_sel),
                                               new_image));
    }
  else
    {
      /*  can't use clay_item_convert() for floating selections of channels
       *  or layer masks because they maybe don't have a normal layer's type
       */
      new_floating_sel =
        CLAY_LAYER (clay_item_duplicate (CLAY_ITEM (floating_sel),
                                         G_TYPE_FROM_INSTANCE (floating_sel)));
      clay_item_set_image (CLAY_ITEM (new_floating_sel), new_image);

      clay_object_set_name (CLAY_OBJECT (new_floating_sel),
                            clay_object_get_name (floating_sel));
    }

  /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
  clay_object_set_name (CLAY_OBJECT (new_floating_sel),
                        clay_object_get_name (floating_sel));

  new_floating_sel_drawable =
    CLAY_DRAWABLE (clay_item_stack_get_item_by_path (new_item_stack,
                                                     floating_sel_path));

  if (CLAY_IS_LAYER_MASK (floating_sel_drawable))
    new_floating_sel_drawable =
      CLAY_DRAWABLE (clay_layer_get_mask (CLAY_LAYER (new_floating_sel_drawable)));

  floating_sel_attach (new_floating_sel, new_floating_sel_drawable);

  g_list_free (floating_sel_path);
}

static void
clay_image_duplicate_mask (ClayImage *image,
                           ClayImage *new_image)
{
  ClayDrawable *mask;
  ClayDrawable *new_mask;

  mask     = CLAY_DRAWABLE (clay_image_get_mask (image));
  new_mask = CLAY_DRAWABLE (clay_image_get_mask (new_image));

  clay_gegl_buffer_copy (clay_drawable_get_buffer (mask), NULL, GEGL_ABYSS_NONE,
                         clay_drawable_get_buffer (new_mask), NULL);

  CLAY_CHANNEL (new_mask)->bounds_known   = FALSE;
  CLAY_CHANNEL (new_mask)->boundary_known = FALSE;
}

static void
clay_image_duplicate_components (ClayImage *image,
                                 ClayImage *new_image)
{
  ClayImagePrivate *private     = CLAY_IMAGE_GET_PRIVATE (image);
  ClayImagePrivate *new_private = CLAY_IMAGE_GET_PRIVATE (new_image);
  gint              count;

  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_private->visible[count] = private->visible[count];
      new_private->active[count]  = private->active[count];
    }
}

static void
clay_image_duplicate_guides (ClayImage *image,
                             ClayImage *new_image)
{
  GList *list;

  for (list = clay_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      ClayGuide *guide    = list->data;
      gint       position = clay_guide_get_position (guide);

      switch (clay_guide_get_orientation (guide))
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          clay_image_add_hguide (new_image, position, FALSE);
          break;

        case CLAY_ORIENTATION_VERTICAL:
          clay_image_add_vguide (new_image, position, FALSE);
          break;

        default:
          g_error ("Unknown guide orientation.\n");
        }
    }
}

static void
clay_image_duplicate_sample_points (ClayImage *image,
                                    ClayImage *new_image)
{
  GList *list;

  for (list = clay_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      ClaySamplePoint   *sample_point = list->data;
      ClaySamplePoint   *new_sample_point;
      ClayColorPickMode  pick_mode;
      gint               x;
      gint               y;

      clay_sample_point_get_position (sample_point, &x, &y);
      pick_mode = clay_sample_point_get_pick_mode (sample_point);

      new_sample_point = clay_image_add_sample_point_at_pos (new_image, x, y,
                                                             FALSE);
      clay_image_set_sample_point_pick_mode (new_image, new_sample_point,
                                             pick_mode, FALSE);
    }
}

static void
clay_image_duplicate_grid (ClayImage *image,
                           ClayImage *new_image)
{
  if (clay_image_get_grid (image))
    clay_image_set_grid (new_image, clay_image_get_grid (image), FALSE);
}

static void
clay_image_duplicate_metadata (ClayImage *image,
                               ClayImage *new_image)
{
  ClayMetadata *metadata = clay_image_get_metadata (image);

  if (metadata)
    {
      metadata = clay_metadata_duplicate (metadata);
      clay_image_set_metadata (new_image, metadata, FALSE);
      g_object_unref (metadata);
    }
}

static void
clay_image_duplicate_quick_mask (ClayImage *image,
                                 ClayImage *new_image)
{
  ClayImagePrivate *private     = CLAY_IMAGE_GET_PRIVATE (image);
  ClayImagePrivate *new_private = CLAY_IMAGE_GET_PRIVATE (new_image);

  new_private->quick_mask_state    = private->quick_mask_state;
  new_private->quick_mask_inverted = private->quick_mask_inverted;
  new_private->quick_mask_color    = gegl_color_duplicate (private->quick_mask_color);
}

static void
clay_image_duplicate_parasites (ClayImage *image,
                                ClayImage *new_image)
{
  ClayImagePrivate *private     = CLAY_IMAGE_GET_PRIVATE (image);
  ClayImagePrivate *new_private = CLAY_IMAGE_GET_PRIVATE (new_image);

  if (private->parasites)
    {
      g_object_unref (new_private->parasites);
      new_private->parasites = clay_parasite_list_copy (private->parasites);
    }
}

static void
clay_image_duplicate_color_profile (ClayImage *image,
                                    ClayImage *new_image)
{
  ClayColorProfile *profile = clay_image_get_color_profile (image);
  ClayColorProfile *hidden  = _clay_image_get_hidden_profile (image);

  clay_image_set_color_profile (new_image, profile, NULL);
  _clay_image_set_hidden_profile (new_image, hidden, FALSE);
}

static void
clay_image_duplicate_simulation_profile (ClayImage *image,
                                         ClayImage *new_image)
{
  ClayColorProfile         *profile;
  ClayColorRenderingIntent  intent;
  gboolean                  bpc;

  profile = clay_image_get_simulation_profile (image);
  intent  = clay_image_get_simulation_intent (image);
  bpc     = clay_image_get_simulation_bpc (image);

  clay_image_set_simulation_profile (new_image, profile);
  clay_image_set_simulation_intent (new_image, intent);
  clay_image_set_simulation_bpc (new_image, bpc);
}
