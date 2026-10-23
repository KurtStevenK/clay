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

#include "clay.h"
#include "claychannelpropundo.h"
#include "claychannelundo.h"
#include "claydrawablefilter.h"
#include "claydrawablefilterundo.h"
#include "claydrawablemodundo.h"
#include "claydrawablepropundo.h"
#include "claydrawableundo.h"
#include "clayfloatingselectionundo.h"
#include "claygrid.h"
#include "claygrouplayer.h"
#include "claygrouplayerundo.h"
#include "clayguide.h"
#include "clayguideundo.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayimageundo.h"
#include "clayitempropundo.h"
#include "claylayermask.h"
#include "claylayermaskpropundo.h"
#include "claylayermaskundo.h"
#include "claylayerpropundo.h"
#include "claylayerundo.h"
#include "claylinklayer.h"
#include "claylinklayerundo.h"
#include "claymaskundo.h"
#include "clayrasterizable.h"
#include "clayrasterizableundo.h"
#include "claysamplepoint.h"
#include "claysamplepointundo.h"
#include "clayselection.h"

#include "path/claypath.h"
#include "path/claypathmodundo.h"
#include "path/claypathpropundo.h"
#include "path/claypathundo.h"
#include "path/clayvectorlayer.h"
#include "path/clayvectorlayerundo.h"

#include "text/claytextlayer.h"
#include "text/claytextundo.h"

#include "clay-intl.h"


/**************************/
/*  Image Property Undos  */
/**************************/

ClayUndo *
clay_image_undo_push_image_type (ClayImage   *image,
                                 const gchar *undo_desc)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_TYPE, undo_desc,
                               CLAY_DIRTY_IMAGE,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_precision (ClayImage   *image,
                                      const gchar *undo_desc)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_PRECISION, undo_desc,
                               CLAY_DIRTY_IMAGE,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_size (ClayImage   *image,
                                 const gchar *undo_desc,
                                 gint         previous_origin_x,
                                 gint         previous_origin_y,
                                 gint         previous_width,
                                 gint         previous_height)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_SIZE, undo_desc,
                               CLAY_DIRTY_IMAGE | CLAY_DIRTY_IMAGE_SIZE,
                               "previous-origin-x", previous_origin_x,
                               "previous-origin-y", previous_origin_y,
                               "previous-width",    previous_width,
                               "previous-height",   previous_height,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_resolution (ClayImage   *image,
                                       const gchar *undo_desc)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_RESOLUTION, undo_desc,
                               CLAY_DIRTY_IMAGE,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_grid (ClayImage   *image,
                                 const gchar *undo_desc,
                                 ClayGrid    *grid)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GRID (grid), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_GRID, undo_desc,
                               CLAY_DIRTY_IMAGE_META,
                               "grid", grid,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_colormap (ClayImage   *image,
                                     const gchar *undo_desc)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_COLORMAP, undo_desc,
                               CLAY_DIRTY_IMAGE,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_hidden_profile (ClayImage   *image,
                                           const gchar *undo_desc)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_HIDDEN_PROFILE, undo_desc,
                               CLAY_DIRTY_IMAGE,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_metadata (ClayImage   *image,
                                     const gchar *undo_desc)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_IMAGE_METADATA, undo_desc,
                               CLAY_DIRTY_IMAGE_META,
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_parasite (ClayImage          *image,
                                     const gchar        *undo_desc,
                                     const ClayParasite *parasite)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_PARASITE_ATTACH, undo_desc,
                               CLAY_DIRTY_IMAGE_META,
                               "parasite-name", clay_parasite_get_name (parasite),
                               NULL);
}

ClayUndo *
clay_image_undo_push_image_parasite_remove (ClayImage   *image,
                                            const gchar *undo_desc,
                                            const gchar *name)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return clay_image_undo_push (image, CLAY_TYPE_IMAGE_UNDO,
                               CLAY_UNDO_PARASITE_REMOVE, undo_desc,
                               CLAY_DIRTY_IMAGE_META,
                               "parasite-name", name,
                               NULL);
}


/********************************/
/*  Guide & Sample Point Undos  */
/********************************/

ClayUndo *
clay_image_undo_push_guide (ClayImage   *image,
                            const gchar *undo_desc,
                            ClayGuide   *guide)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GUIDE (guide), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GUIDE_UNDO,
                               CLAY_UNDO_GUIDE, undo_desc,
                               CLAY_DIRTY_IMAGE_META,
                               "aux-item", guide,
                               NULL);
}

ClayUndo *
clay_image_undo_push_sample_point (ClayImage       *image,
                                   const gchar     *undo_desc,
                                   ClaySamplePoint *sample_point)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_SAMPLE_POINT (sample_point), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_SAMPLE_POINT_UNDO,
                               CLAY_UNDO_SAMPLE_POINT, undo_desc,
                               CLAY_DIRTY_IMAGE_META,
                               "aux-item", sample_point,
                               NULL);
}


/********************/
/*  Drawable Undos  */
/********************/

ClayUndo *
clay_image_undo_push_drawable (ClayImage    *image,
                               const gchar  *undo_desc,
                               ClayDrawable *drawable,
                               GeglBuffer   *buffer,
                               gint          x,
                               gint          y)
{
  ClayItem *item;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  item = CLAY_ITEM (drawable);

  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_UNDO,
                               CLAY_UNDO_DRAWABLE, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",   item,
                               "buffer", buffer,
                               "x",      x,
                               "y",      y,
                               NULL);
}

ClayUndo *
clay_image_undo_push_drawable_mod (ClayImage    *image,
                                   const gchar  *undo_desc,
                                   ClayDrawable *drawable,
                                   gboolean      copy_buffer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_MOD_UNDO,
                               CLAY_UNDO_DRAWABLE_MOD, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",        drawable,
                               "copy-buffer", copy_buffer,
                               NULL);
}

ClayUndo *
clay_image_undo_push_drawable_format (ClayImage    *image,
                                      const gchar  *undo_desc,
                                      ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_PROP_UNDO,
                               CLAY_UNDO_DRAWABLE_FORMAT, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item", drawable,
                               NULL);
}


/***************************/
/*  Drawable Filter Undos  */
/***************************/

ClayUndo *
clay_image_undo_push_filter_add (ClayImage          *image,
                                 const gchar        *undo_desc,
                                 ClayDrawable       *drawable,
                                 ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);
  g_return_val_if_fail (clay_drawable_filter_get_temporary (filter) == FALSE,
                        NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_FILTER_UNDO,
                               CLAY_UNDO_FILTER_ADD, undo_desc,
                               CLAY_DIRTY_DRAWABLE,
                               "filter", filter,
                               NULL);
}

ClayUndo *
clay_image_undo_push_filter_remove (ClayImage          *image,
                                    const gchar        *undo_desc,
                                    ClayDrawable       *drawable,
                                    ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);
  g_return_val_if_fail (clay_drawable_filter_get_temporary (filter) == FALSE,
                        NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_FILTER_UNDO,
                               CLAY_UNDO_FILTER_REMOVE, undo_desc,
                               CLAY_DIRTY_DRAWABLE,
                               "filter", filter,
                               NULL);
}

ClayUndo *
clay_image_undo_push_filter_reorder (ClayImage          *image,
                                     const gchar        *undo_desc,
                                     ClayDrawable       *drawable,
                                     ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);
  g_return_val_if_fail (clay_drawable_filter_get_temporary (filter) == FALSE,
                        NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_FILTER_UNDO,
                               CLAY_UNDO_FILTER_REORDER, undo_desc,
                               CLAY_DIRTY_DRAWABLE,
                               "filter", filter,
                               NULL);
}

ClayUndo *
clay_image_undo_push_filter_modified (ClayImage          *image,
                                      const gchar        *undo_desc,
                                      ClayDrawable       *drawable,
                                      ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);
  g_return_val_if_fail (clay_drawable_filter_get_temporary (filter) == FALSE,
                        NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_FILTER_UNDO,
                               CLAY_UNDO_FILTER_MODIFIED, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "filter", filter,
                               NULL);
}

ClayUndo *
clay_image_undo_push_filter_visibility (ClayImage          *image,
                                        const gchar        *undo_desc,
                                        ClayDrawable       *drawable,
                                        ClayDrawableFilter *filter)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_DRAWABLE_FILTER_UNDO,
                               CLAY_UNDO_FILTER_VISIBILITY, undo_desc,
                               CLAY_DIRTY_ITEM_META, "filter", filter, NULL);
}

ClayUndo *
clay_image_undo_push_filter_visibility_compressible (ClayDrawableFilter *filter,
                                                     ClayContext        *context)
{
  ClayDrawable *drawable;
  ClayImage    *image;
  ClayUndo     *undo;
  gboolean      push_undo = TRUE;

  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  drawable = clay_drawable_filter_get_drawable (filter);
  image    = clay_item_get_image (CLAY_ITEM (drawable));

  undo = clay_image_undo_can_compress (image, CLAY_TYPE_DRAWABLE_FILTER_UNDO,
                                       CLAY_UNDO_FILTER_VISIBILITY);

  if (undo != NULL && CLAY_DRAWABLE_FILTER_UNDO (undo)->filter == filter)
    {
      push_undo = FALSE;
    }
  else
    {
      undo = clay_image_undo_push_filter_visibility (image,
                                                     _("Filter visibility"),
                                                     drawable,
                                                     filter);
    }

  if (! push_undo)
    clay_undo_refresh_preview (undo, context);

  return undo;
}


/****************/
/*  Mask Undos  */
/****************/

ClayUndo *
clay_image_undo_push_mask (ClayImage   *image,
                           const gchar *undo_desc,
                           ClayChannel *mask)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CHANNEL (mask), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (mask)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_MASK_UNDO,
                               CLAY_UNDO_MASK, undo_desc,
                               CLAY_IS_SELECTION (mask) ?
                               CLAY_DIRTY_SELECTION :
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item", mask,
                               NULL);
}

ClayUndo *
clay_image_undo_push_mask_precision (ClayImage   *image,
                                     const gchar *undo_desc,
                                     ClayChannel *mask)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CHANNEL (mask), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (mask)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_MASK_UNDO,
                               CLAY_UNDO_MASK, undo_desc,
                               CLAY_IS_SELECTION (mask) ?
                               CLAY_DIRTY_SELECTION :
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",           mask,
                               "convert-format", TRUE,
                               NULL);
}


/****************/
/*  Item Undos  */
/****************/

ClayUndo *
clay_image_undo_push_item_reorder (ClayImage   *image,
                                   const gchar *undo_desc,
                                   ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_REORDER, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_rename (ClayImage   *image,
                                  const gchar *undo_desc,
                                  ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_RENAME, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_displace (ClayImage   *image,
                                    const gchar *undo_desc,
                                    ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_DISPLACE, undo_desc,
                               CLAY_IS_DRAWABLE (item) ?
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE :
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_PATH,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_visibility (ClayImage   *image,
                                      const gchar *undo_desc,
                                      ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_VISIBILITY, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_color_tag (ClayImage   *image,
                                     const gchar *undo_desc,
                                     ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_COLOR_TAG, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_lock_content (ClayImage   *image,
                                        const gchar *undo_desc,
                                        ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_LOCK_CONTENT, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_lock_position (ClayImage   *image,
                                         const gchar *undo_desc,
                                         ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_LOCK_POSITION, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_lock_visibility (ClayImage   *image,
                                           const gchar *undo_desc,
                                           ClayItem    *item)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_ITEM_LOCK_VISIBILITY, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", item,
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_parasite (ClayImage          *image,
                                    const gchar        *undo_desc,
                                    ClayItem           *item,
                                    const ClayParasite *parasite)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);
  g_return_val_if_fail (parasite != NULL, NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_PARASITE_ATTACH, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item",          item,
                               "parasite-name", clay_parasite_get_name (parasite),
                               NULL);
}

ClayUndo *
clay_image_undo_push_item_parasite_remove (ClayImage   *image,
                                           const gchar *undo_desc,
                                           ClayItem    *item,
                                           const gchar *name)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return clay_image_undo_push (image, CLAY_TYPE_ITEM_PROP_UNDO,
                               CLAY_UNDO_PARASITE_REMOVE, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item",          item,
                               "parasite-name", name,
                               NULL);
}


/*****************/
/*  Layer Undos  */
/*****************/

ClayUndo *
clay_image_undo_push_layer_add (ClayImage   *image,
                                const gchar *undo_desc,
                                ClayLayer   *layer,
                                GList       *prev_layers)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (! clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  for (iter = prev_layers; iter; iter = iter->next)
    g_return_val_if_fail (CLAY_IS_LAYER (iter->data), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_UNDO,
                               CLAY_UNDO_LAYER_ADD, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",        layer,
                               "prev-layers", prev_layers,
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_remove (ClayImage   *image,
                                   const gchar *undo_desc,
                                   ClayLayer   *layer,
                                   ClayLayer   *prev_parent,
                                   gint         prev_position,
                                   GList       *prev_layers)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);
  g_return_val_if_fail (prev_parent == NULL || CLAY_IS_LAYER (prev_parent),
                        NULL);

  for (iter = prev_layers; iter; iter = iter->next)
    g_return_val_if_fail (CLAY_IS_LAYER (iter->data), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_UNDO,
                               CLAY_UNDO_LAYER_REMOVE, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",          layer,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-layers",   prev_layers,
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_mode (ClayImage   *image,
                                 const gchar *undo_desc,
                                 ClayLayer   *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_PROP_UNDO,
                               CLAY_UNDO_LAYER_MODE, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_opacity (ClayImage   *image,
                                    const gchar *undo_desc,
                                    ClayLayer   *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_PROP_UNDO,
                               CLAY_UNDO_LAYER_OPACITY, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_lock_alpha (ClayImage   *image,
                                       const gchar *undo_desc,
                                       ClayLayer   *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_PROP_UNDO,
                               CLAY_UNDO_LAYER_LOCK_ALPHA, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}


/***********************/
/*  Group Layer Undos  */
/***********************/

ClayUndo *
clay_image_undo_push_group_layer_suspend_resize (ClayImage      *image,
                                                 const gchar    *undo_desc,
                                                 ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

ClayUndo *
clay_image_undo_push_group_layer_resume_resize (ClayImage      *image,
                                                const gchar    *undo_desc,
                                                ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

ClayUndo *
clay_image_undo_push_group_layer_suspend_mask (ClayImage      *image,
                                               const gchar    *undo_desc,
                                               ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

ClayUndo *
clay_image_undo_push_group_layer_resume_mask (ClayImage      *image,
                                              const gchar    *undo_desc,
                                              ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_RESUME_MASK, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

ClayUndo *
clay_image_undo_push_group_layer_start_transform (ClayImage      *image,
                                                  const gchar    *undo_desc,
                                                  ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_START_TRANSFORM, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

ClayUndo *
clay_image_undo_push_group_layer_end_transform (ClayImage      *image,
                                                const gchar    *undo_desc,
                                                ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_END_TRANSFORM, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  group,
                               NULL);
}

ClayUndo *
clay_image_undo_push_group_layer_convert (ClayImage      *image,
                                          const gchar    *undo_desc,
                                          ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (group)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_GROUP_LAYER_UNDO,
                               CLAY_UNDO_GROUP_LAYER_CONVERT, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item", group,
                               NULL);
}


/************************/
/*  Rasterizable Undos  */
/************************/

ClayUndo *
clay_image_undo_push_rasterizable (ClayImage        *image,
                                   const gchar      *undo_desc,
                                   ClayRasterizable *rasterizable)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_RASTERIZABLE (rasterizable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (rasterizable)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_RASTERIZABLE_UNDO,
                               CLAY_UNDO_RASTERIZABLE, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item", rasterizable,
                               NULL);
}


/**********************/
/*  Text Layer Undos  */
/**********************/

ClayUndo *
clay_image_undo_push_text_layer (ClayImage        *image,
                                 const gchar      *undo_desc,
                                 ClayTextLayer    *layer,
                                 const GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_TEXT_UNDO,
                               CLAY_UNDO_TEXT_LAYER, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  layer,
                               "param", pspec,
                               NULL);
}

ClayUndo *
clay_image_undo_push_text_layer_convert (ClayImage     *image,
                                         const gchar   *undo_desc,
                                         ClayTextLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_TEXT_UNDO,
                               CLAY_UNDO_TEXT_LAYER_CONVERT, undo_desc,
                               CLAY_DIRTY_ITEM,
                               "item", layer,
                               NULL);
}


/**********************/
/*  Link Layer Undos  */
/**********************/

ClayUndo *
clay_image_undo_push_link_layer (ClayImage     *image,
                                 const gchar   *undo_desc,
                                 ClayLinkLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LINK_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LINK_LAYER_UNDO,
                               CLAY_UNDO_LINK_LAYER, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  layer,
                               NULL);
}

/************************/
/*  Vector Layer Undos  */
/************************/

ClayUndo *
clay_image_undo_push_vector_layer (ClayImage        *image,
                                   const gchar      *undo_desc,
                                   ClayVectorLayer  *layer,
                                   const GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_VECTOR_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_VECTOR_LAYER_UNDO,
                               CLAY_UNDO_VECTOR_LAYER, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item",  layer,
                               "param", pspec,
                               NULL);
}


/**********************/
/*  Layer Mask Undos  */
/**********************/

ClayUndo *
clay_image_undo_push_layer_mask_add (ClayImage     *image,
                                     const gchar   *undo_desc,
                                     ClayLayer     *layer,
                                     ClayLayerMask *mask)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (! clay_item_is_attached (CLAY_ITEM (mask)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_MASK_UNDO,
                               CLAY_UNDO_LAYER_MASK_ADD, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "layer-mask", mask,
                               "edit-mask",  FALSE,
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_mask_remove (ClayImage     *image,
                                        const gchar   *undo_desc,
                                        ClayLayer     *layer,
                                        ClayLayerMask *mask)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER_MASK (mask), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (mask)), NULL);
  g_return_val_if_fail (clay_layer_mask_get_layer (mask) == layer, NULL);
  g_return_val_if_fail (clay_layer_get_mask (layer) == mask, NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_MASK_UNDO,
                               CLAY_UNDO_LAYER_MASK_REMOVE, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",       layer,
                               "layer-mask", mask,
                               "edit-mask",  clay_layer_get_edit_mask (layer),
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_mask_apply (ClayImage   *image,
                                       const gchar *undo_desc,
                                       ClayLayer   *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_MASK_PROP_UNDO,
                               CLAY_UNDO_LAYER_MASK_APPLY, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}

ClayUndo *
clay_image_undo_push_layer_mask_show (ClayImage   *image,
                                      const gchar *undo_desc,
                                      ClayLayer   *layer)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (layer), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (layer)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_LAYER_MASK_PROP_UNDO,
                               CLAY_UNDO_LAYER_MASK_SHOW, undo_desc,
                               CLAY_DIRTY_ITEM_META,
                               "item", layer,
                               NULL);
}


/*******************/
/*  Channel Undos  */
/*******************/

ClayUndo *
clay_image_undo_push_channel_add (ClayImage   *image,
                                  const gchar *undo_desc,
                                  ClayChannel *channel,
                                  GList       *prev_channels)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (! clay_item_is_attached (CLAY_ITEM (channel)), NULL);

  for (iter = prev_channels; iter; iter = iter->next)
    g_return_val_if_fail (CLAY_IS_CHANNEL (iter->data), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_CHANNEL_UNDO,
                               CLAY_UNDO_CHANNEL_ADD, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",          channel,
                               "prev-channels", prev_channels,
                               NULL);
}

ClayUndo *
clay_image_undo_push_channel_remove (ClayImage   *image,
                                     const gchar *undo_desc,
                                     ClayChannel *channel,
                                     ClayChannel *prev_parent,
                                     gint         prev_position,
                                     GList       *prev_channels)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (channel)), NULL);
  g_return_val_if_fail (prev_parent == NULL || CLAY_IS_CHANNEL (prev_parent),
                        NULL);

  for (iter = prev_channels; iter; iter = iter->next)
    g_return_val_if_fail (CLAY_IS_CHANNEL (iter->data), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_CHANNEL_UNDO,
                               CLAY_UNDO_CHANNEL_REMOVE, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",          channel,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-channels", prev_channels,
                               NULL);
}

ClayUndo *
clay_image_undo_push_channel_color (ClayImage   *image,
                                    const gchar *undo_desc,
                                    ClayChannel *channel)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (channel)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_CHANNEL_PROP_UNDO,
                               CLAY_UNDO_CHANNEL_COLOR, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_DRAWABLE,
                               "item", channel,
                               NULL);
}


/*******************/
/*  Path Undos  */
/*******************/

ClayUndo *
clay_image_undo_push_path_add (ClayImage   *image,
                               const gchar *undo_desc,
                               ClayPath    *path,
                               GList       *prev_paths)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (! clay_item_is_attached (CLAY_ITEM (path)), NULL);

  for (iter = prev_paths; iter; iter = iter->next)
    g_return_val_if_fail (CLAY_IS_PATH (iter->data), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_PATH_UNDO,
                               CLAY_UNDO_PATH_ADD, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",         path,
                               "prev-paths", prev_paths,
                               NULL);
}

ClayUndo *
clay_image_undo_push_path_remove (ClayImage   *image,
                                  const gchar *undo_desc,
                                  ClayPath    *path,
                                  ClayPath    *prev_parent,
                                  gint         prev_position,
                                  GList       *prev_paths)
{
  GList *iter;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (path)), NULL);
  g_return_val_if_fail (prev_parent == NULL || CLAY_IS_PATH (prev_parent),
                        NULL);

  for (iter = prev_paths; iter; iter = iter->next)
    g_return_val_if_fail (CLAY_IS_PATH (iter->data), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_PATH_UNDO,
                               CLAY_UNDO_PATH_REMOVE, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item",          path,
                               "prev-parent",   prev_parent,
                               "prev-position", prev_position,
                               "prev-paths",    prev_paths,
                               NULL);
}

ClayUndo *
clay_image_undo_push_path_mod (ClayImage   *image,
                               const gchar *undo_desc,
                               ClayPath    *path)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (path)), NULL);

  return clay_image_undo_push (image, CLAY_TYPE_PATH_MOD_UNDO,
                               CLAY_UNDO_PATH_MOD, undo_desc,
                               CLAY_DIRTY_ITEM | CLAY_DIRTY_PATH,
                               "item", path,
                               NULL);
}


/******************************/
/*  Floating Selection Undos  */
/******************************/

ClayUndo *
clay_image_undo_push_fs_to_layer (ClayImage    *image,
                                  const gchar  *undo_desc,
                                  ClayLayer    *floating_layer)
{
  ClayUndo *undo;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_LAYER (floating_layer), NULL);

  undo = clay_image_undo_push (image, CLAY_TYPE_FLOATING_SELECTION_UNDO,
                               CLAY_UNDO_FS_TO_LAYER, undo_desc,
                               CLAY_DIRTY_IMAGE_STRUCTURE,
                               "item", floating_layer,
                               NULL);

  return undo;
}


/******************************************************************************/
/*  Something for which programmer is too lazy to write an undo function for  */
/******************************************************************************/

static void
undo_pop_cantundo (ClayUndo            *undo,
                   ClayUndoMode         undo_mode,
                   ClayUndoAccumulator *accum)
{
  switch (undo_mode)
    {
    case CLAY_UNDO_MODE_UNDO:
      clay_message (undo->image->clay, NULL, CLAY_MESSAGE_WARNING,
                    _("Can't undo %s"), clay_object_get_name (undo));
      break;

    case CLAY_UNDO_MODE_REDO:
      break;
    }
}

ClayUndo *
clay_image_undo_push_cantundo (ClayImage   *image,
                               const gchar *undo_desc)
{
  ClayUndo *undo;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility.
   */

  undo = clay_image_undo_push (image, CLAY_TYPE_UNDO,
                               CLAY_UNDO_CANT, undo_desc,
                               CLAY_DIRTY_ALL,
                               NULL);

  if (undo)
    g_signal_connect (undo, "pop",
                      G_CALLBACK (undo_pop_cantundo),
                      NULL);

  return undo;
}
