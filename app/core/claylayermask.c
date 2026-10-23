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

#include "core-types.h"

#include "gegl/clay-babl.h"

#include "clayerror.h"
#include "clayimage.h"
#include "claylayer.h"
#include "claylayermask.h"

#include "clay-intl.h"


static void            clay_layer_mask_preview_freeze       (ClayViewable      *viewable);
static void            clay_layer_mask_preview_thaw         (ClayViewable      *viewable);

static gboolean        clay_layer_mask_is_attached          (ClayItem          *item);
static gboolean        clay_layer_mask_is_content_locked    (ClayItem          *item,
                                                             ClayItem         **locked_item,
                                                             gboolean           check_children);
static gboolean        clay_layer_mask_is_position_locked   (ClayItem          *item,
                                                             ClayItem         **locked_item,
                                                             gboolean           check_children);
static ClayItemTree  * clay_layer_mask_get_tree             (ClayItem          *item);
static ClayItem      * clay_layer_mask_duplicate            (ClayItem          *item,
                                                             GType              new_type);
static gboolean        clay_layer_mask_rename               (ClayItem          *item,
                                                             const gchar       *new_name,
                                                             const gchar       *undo_desc,
                                                             GError           **error);

static void            clay_layer_mask_bounding_box_changed (ClayDrawable      *drawable);
static void            clay_layer_mask_convert_type         (ClayDrawable      *drawable,
                                                             ClayImage         *dest_image,
                                                             const Babl        *new_format,
                                                             ClayColorProfile  *src_profile,
                                                             ClayColorProfile  *dest_profile,
                                                             GeglDitherMethod   layer_dither_type,
                                                             GeglDitherMethod   mask_dither_type,
                                                             gboolean           push_undo,
                                                             ClayProgress      *progress);
static void            clay_layer_mask_filters_changed      (ClayDrawable      *drawable);


G_DEFINE_TYPE (ClayLayerMask, clay_layer_mask, CLAY_TYPE_CHANNEL)

#define parent_class clay_layer_mask_parent_class


static void
clay_layer_mask_class_init (ClayLayerMaskClass *klass)
{
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class     = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class = CLAY_DRAWABLE_CLASS (klass);

  viewable_class->default_icon_name = "clay-layer-mask";

  viewable_class->preview_freeze       = clay_layer_mask_preview_freeze;
  viewable_class->preview_thaw         = clay_layer_mask_preview_thaw;

  item_class->is_attached              = clay_layer_mask_is_attached;
  item_class->is_content_locked        = clay_layer_mask_is_content_locked;
  item_class->is_position_locked       = clay_layer_mask_is_position_locked;
  item_class->get_tree                 = clay_layer_mask_get_tree;
  item_class->duplicate                = clay_layer_mask_duplicate;
  item_class->rename                   = clay_layer_mask_rename;
  item_class->translate_desc           = C_("undo-type", "Move Layer Mask");
  item_class->to_selection_desc        = C_("undo-type", "Layer Mask to Selection");

  drawable_class->bounding_box_changed = clay_layer_mask_bounding_box_changed;
  drawable_class->convert_type         = clay_layer_mask_convert_type;
  drawable_class->filters_changed      = clay_layer_mask_filters_changed;
}

static void
clay_layer_mask_init (ClayLayerMask *layer_mask)
{
  layer_mask->layer = NULL;
}

static void
clay_layer_mask_preview_freeze (ClayViewable *viewable)
{
  ClayLayerMask *mask  = CLAY_LAYER_MASK (viewable);
  ClayLayer     *layer = clay_layer_mask_get_layer (mask);

  if (layer)
    {
      ClayViewable *parent = clay_viewable_get_parent (CLAY_VIEWABLE (layer));

      if (! parent && clay_item_is_attached (CLAY_ITEM (layer)))
        parent = CLAY_VIEWABLE (clay_item_get_image (CLAY_ITEM (layer)));

      if (parent)
        clay_viewable_preview_freeze (parent);
    }
}

static void
clay_layer_mask_preview_thaw (ClayViewable *viewable)
{
  ClayLayerMask *mask  = CLAY_LAYER_MASK (viewable);
  ClayLayer     *layer = clay_layer_mask_get_layer (mask);

  if (layer)
    {
      ClayViewable *parent = clay_viewable_get_parent (CLAY_VIEWABLE (layer));

      if (! parent && clay_item_is_attached (CLAY_ITEM (layer)))
        parent = CLAY_VIEWABLE (clay_item_get_image (CLAY_ITEM (layer)));

      if (parent)
        clay_viewable_preview_thaw (parent);
    }
}

static gboolean
clay_layer_mask_is_content_locked (ClayItem  *item,
                                   ClayItem **locked_item,
                                   gboolean   check_children)
{
  ClayLayerMask *mask  = CLAY_LAYER_MASK (item);
  ClayLayer     *layer = clay_layer_mask_get_layer (mask);

  if (layer)
    return clay_item_is_content_locked (CLAY_ITEM (layer), locked_item);

  return FALSE;
}

static gboolean
clay_layer_mask_is_position_locked (ClayItem  *item,
                                    ClayItem **locked_item,
                                    gboolean   check_children)
{
  ClayLayerMask *mask  = CLAY_LAYER_MASK (item);
  ClayLayer     *layer = clay_layer_mask_get_layer (mask);

  if (layer)
    return clay_item_is_position_locked (CLAY_ITEM (layer), locked_item);

  return FALSE;
}

static gboolean
clay_layer_mask_is_attached (ClayItem *item)
{
  ClayLayerMask *mask  = CLAY_LAYER_MASK (item);
  ClayLayer     *layer = clay_layer_mask_get_layer (mask);

  return (CLAY_IS_IMAGE (clay_item_get_image (item)) &&
          CLAY_IS_LAYER (layer)                      &&
          clay_layer_get_mask (layer) == mask        &&
          clay_item_is_attached (CLAY_ITEM (layer)));
}

static ClayItemTree *
clay_layer_mask_get_tree (ClayItem *item)
{
  return NULL;
}

static ClayItem *
clay_layer_mask_duplicate (ClayItem *item,
                           GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  return new_item;
}

static gboolean
clay_layer_mask_rename (ClayItem     *item,
                        const gchar  *new_name,
                        const gchar  *undo_desc,
                        GError      **error)
{
  /* reject renaming, layer masks are always named "<layer name> mask"  */

  g_set_error (error, CLAY_ERROR, CLAY_FAILED,
               _("Cannot rename layer masks."));

  return FALSE;
}

static void
clay_layer_mask_bounding_box_changed (ClayDrawable *drawable)
{
  ClayLayerMask *mask  = CLAY_LAYER_MASK (drawable);
  ClayLayer     *layer = clay_layer_mask_get_layer (mask);

  if (CLAY_DRAWABLE_CLASS (parent_class)->bounding_box_changed)
    CLAY_DRAWABLE_CLASS (parent_class)->bounding_box_changed (drawable);

  if (layer)
    clay_drawable_update_bounding_box (CLAY_DRAWABLE (layer));
}

static void
clay_layer_mask_convert_type (ClayDrawable      *drawable,
                              ClayImage         *dest_image,
                              const Babl        *new_format,
                              ClayColorProfile  *src_profile,
                              ClayColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              ClayProgress      *progress)
{
  new_format =
    clay_babl_mask_format (clay_babl_format_get_precision (new_format));

  CLAY_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);
}

static void
clay_layer_mask_filters_changed (ClayDrawable *drawable)
{
  ClayLayerMask *layer_mask = CLAY_LAYER_MASK (drawable);

  clay_drawable_filters_changed (CLAY_DRAWABLE (layer_mask->layer));
}

ClayLayerMask *
clay_layer_mask_new (ClayImage   *image,
                     gint         width,
                     gint         height,
                     const gchar *name,
                     GeglColor   *color)
{
  ClayLayerMask *layer_mask;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  layer_mask =
    CLAY_LAYER_MASK (clay_drawable_new (CLAY_TYPE_LAYER_MASK,
                                        image, name,
                                        0, 0, width, height,
                                        clay_image_get_mask_format (image)));

  /*  set the layer_mask color and opacity  */
  clay_channel_set_color (CLAY_CHANNEL (layer_mask), color, FALSE);
  clay_channel_set_show_masked (CLAY_CHANNEL (layer_mask), TRUE);

  /*  selection mask variables  */
  CLAY_CHANNEL (layer_mask)->x2 = width;
  CLAY_CHANNEL (layer_mask)->y2 = height;

  return layer_mask;
}

void
clay_layer_mask_set_layer (ClayLayerMask *layer_mask,
                           ClayLayer     *layer)
{
  g_return_if_fail (CLAY_IS_LAYER_MASK (layer_mask));
  g_return_if_fail (layer == NULL || CLAY_IS_LAYER (layer));

  layer_mask->layer = layer;

  if (layer)
    {
      gchar *mask_name;
      gint   offset_x;
      gint   offset_y;

      clay_item_get_offset (CLAY_ITEM (layer), &offset_x, &offset_y);
      clay_item_set_offset (CLAY_ITEM (layer_mask), offset_x, offset_y);

      mask_name = g_strdup_printf (_("%s mask"), clay_object_get_name (layer));

      clay_object_take_name (CLAY_OBJECT (layer_mask), mask_name);
    }
}

ClayLayer *
clay_layer_mask_get_layer (ClayLayerMask *layer_mask)
{
  g_return_val_if_fail (CLAY_IS_LAYER_MASK (layer_mask), NULL);

  return layer_mask->layer;
}
