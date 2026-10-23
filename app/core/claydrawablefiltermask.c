/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawablefiltermask.c
 * Copyright (C) 2024 Jehan
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

#include "claydrawablefilter.h"
#include "claydrawablefiltermask.h"
#include "clayerror.h"
#include "clayimage.h"

#include "clay-intl.h"


static gboolean       clay_drawable_filter_mask_is_attached   (ClayItem            *item);
static ClayItemTree * clay_drawable_filter_mask_get_tree      (ClayItem            *item);
static gboolean       clay_drawable_filter_mask_rename        (ClayItem            *item,
                                                               const gchar         *new_name,
                                                               const gchar         *undo_desc,
                                                               GError             **error);

static void           clay_drawable_filter_mask_convert_type  (ClayDrawable        *drawable,
                                                               ClayImage           *dest_image,
                                                               const Babl          *new_format,
                                                               ClayColorProfile    *src_profile,
                                                               ClayColorProfile    *dest_profile,
                                                               GeglDitherMethod     layer_dither_type,
                                                               GeglDitherMethod     mask_dither_type,
                                                               gboolean             push_undo,
                                                               ClayProgress        *progress);


G_DEFINE_TYPE (ClayDrawableFilterMask, clay_drawable_filter_mask, CLAY_TYPE_CHANNEL)

#define parent_class clay_drawable_filter_mask_parent_class


static void
clay_drawable_filter_mask_class_init (ClayDrawableFilterMaskClass *klass)
{
  ClayItemClass     *item_class     = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class = CLAY_DRAWABLE_CLASS (klass);

  item_class->is_attached           = clay_drawable_filter_mask_is_attached;
  item_class->get_tree              = clay_drawable_filter_mask_get_tree;
  item_class->rename                = clay_drawable_filter_mask_rename;

  drawable_class->convert_type      = clay_drawable_filter_mask_convert_type;
}

static void
clay_drawable_filter_mask_init (ClayDrawableFilterMask *drawable_filter_mask)
{
}

static gboolean
clay_drawable_filter_mask_is_attached (ClayItem *item)
{
  ClayDrawableFilterMask *mask = CLAY_DRAWABLE_FILTER_MASK (item);
  ClayDrawable           *drawable;

  if (! mask->filter)
    return FALSE;

  drawable = clay_drawable_filter_get_drawable (mask->filter);

  return (drawable                                     &&
          clay_item_is_attached (CLAY_ITEM (drawable)) &&
          clay_drawable_filter_get_mask (mask->filter) == mask);
}

static ClayItemTree *
clay_drawable_filter_mask_get_tree (ClayItem *item)
{
  return NULL;
}

static gboolean
clay_drawable_filter_mask_rename (ClayItem     *item,
                                  const gchar  *new_name,
                                  const gchar  *undo_desc,
                                  GError      **error)
{
  g_set_error (error, CLAY_ERROR, CLAY_FAILED,
               _("Cannot rename effect masks."));

  return FALSE;
}

static void
clay_drawable_filter_mask_convert_type (ClayDrawable      *drawable,
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


/*  public functions  */

ClayDrawableFilterMask *
clay_drawable_filter_mask_new (ClayImage *image,
                               gint       width,
                               gint       height)
{
  GeglColor   *black = gegl_color_new ("black");
  ClayChannel *channel;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  clay_color_set_alpha (black, 0.5);
  channel = CLAY_CHANNEL (clay_drawable_new (CLAY_TYPE_DRAWABLE_FILTER_MASK,
                                             image, NULL,
                                             0, 0, width, height,
                                             clay_image_get_mask_format (image)));

  clay_channel_set_color (channel, black, FALSE);
  clay_channel_set_show_masked (channel, TRUE);

  channel->x2 = width;
  channel->y2 = height;

  g_object_unref (black);

  return CLAY_DRAWABLE_FILTER_MASK (channel);
}

void
clay_drawable_filter_mask_set_filter (ClayDrawableFilterMask *mask,
                                      ClayDrawableFilter     *filter)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER_MASK (mask));
  g_return_if_fail (filter == NULL || CLAY_IS_DRAWABLE_FILTER (filter));

  mask->filter = filter;

  if (filter)
    {
      ClayDrawable *drawable;
      gchar        *mask_name;
      gint          offset_x;
      gint          offset_y;

      drawable = clay_drawable_filter_get_drawable (mask->filter);

      if (drawable)
        {
          clay_item_get_offset (CLAY_ITEM (drawable), &offset_x, &offset_y);
          clay_item_set_offset (CLAY_ITEM (mask), offset_x, offset_y);
        }

      mask_name = g_strdup_printf (_("%s mask"), clay_object_get_name (filter));

      clay_object_take_name (CLAY_OBJECT (mask), mask_name);
    }
}

ClayDrawableFilter *
clay_drawable_filter_mask_get_filter (ClayDrawableFilterMask *mask)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER_MASK (mask), NULL);

  return mask->filter;
}
