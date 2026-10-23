/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimage-convert-precision.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "claychannel.h"
#include "claydrawable.h"
#include "claydrawable-operation.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-convert-precision.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"

#include "text/claytextlayer.h"

#include "clay-intl.h"


void
clay_image_convert_precision (ClayImage        *image,
                              ClayPrecision     precision,
                              GeglDitherMethod  layer_dither_type,
                              GeglDitherMethod  text_layer_dither_type,
                              GeglDitherMethod  mask_dither_type,
                              ClayProgress     *progress)
{
  ClayColorProfile *profile;
  ClayObjectQueue  *queue;
  ClayProgress     *sub_progress;
  GList            *layers;
  ClayDrawable     *drawable;
  const gchar      *enum_desc;
  gchar            *undo_desc = NULL;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (precision != clay_image_get_precision (image));
  g_return_if_fail (clay_babl_is_valid (clay_image_get_base_type (image),
                                        precision));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  clay_enum_get_value (CLAY_TYPE_PRECISION, precision,
                       NULL, NULL, &enum_desc, NULL);

  undo_desc = g_strdup_printf (C_("undo-type", "Convert Image to %s"),
                               enum_desc);

  if (progress)
    clay_progress_start (progress, FALSE, "%s", undo_desc);

  queue        = clay_object_queue_new (progress);
  sub_progress = CLAY_PROGRESS (queue);

  layers = clay_image_get_layer_list (image);
  clay_object_queue_push_list (queue, layers);
  g_list_free (layers);

  clay_object_queue_push (queue, clay_image_get_mask (image));
  clay_object_queue_push_container (queue, clay_image_get_channels (image));

  g_object_freeze_notify (G_OBJECT (image));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_CONVERT,
                               undo_desc);
  g_free (undo_desc);

  /*  Push the image precision to the stack  */
  clay_image_undo_push_image_precision (image, NULL);

  /* Use the actual image's profile, and in particular don't be clever
   * by using a builtin profile. I.e. don't use clay_color_managed_get_color_profile().
   * We don't want a profile change, and the format is enough to take
   * care of any TRC override if needed.
   */
  profile = clay_image_get_color_profile (image);

  clay_image_set_converting (image, TRUE);

  /*  Set the new precision  */
  g_object_set (image, "precision", precision, NULL);

  while ((drawable = clay_object_queue_pop (queue)))
    {
      if (drawable == CLAY_DRAWABLE (clay_image_get_mask (image)))
        {
          GeglBuffer *buffer;

          clay_image_undo_push_mask_precision (image, NULL,
                                               CLAY_CHANNEL (drawable));

          buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                         clay_image_get_width  (image),
                                                         clay_image_get_height (image)),
                                         clay_image_get_mask_format (image),
                                         drawable);

          clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), NULL,
                                 GEGL_ABYSS_NONE,
                                 buffer, NULL);

          clay_drawable_set_buffer (drawable, FALSE, NULL, buffer);
          g_object_unref (buffer);

          clay_progress_set_value (sub_progress, 1.0);
        }
      else
        {
          GeglDitherMethod dither_type;

          if (clay_item_is_text_layer (CLAY_ITEM (drawable)))
            dither_type = text_layer_dither_type;
          else
            dither_type = layer_dither_type;

          clay_drawable_convert_type (drawable, image,
                                      clay_drawable_get_base_type (drawable),
                                      precision,
                                      clay_drawable_has_alpha (drawable),
                                      profile, profile,
                                      dither_type,
                                      mask_dither_type,
                                      TRUE, sub_progress);
        }
    }

  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (image));

  clay_image_set_converting (image, FALSE);

  clay_image_undo_group_end (image);

  clay_image_precision_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_object_unref (queue);

  if (progress)
    clay_progress_end (progress);
}

void
clay_image_convert_dither_u8 (ClayImage    *image,
                              ClayProgress *progress)
{
  GeglNode *dither;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  dither = gegl_node_new_child (NULL,
                                "operation", "gegl:noise-rgb",
                                "red",       1.0 / 256.0,
                                "green",     1.0 / 256.0,
                                "blue",      1.0 / 256.0,
                                "linear",    FALSE,
                                "gaussian",  FALSE,
                                NULL);

  if (dither)
    {
      ClayObjectQueue *queue;
      ClayProgress    *sub_progress;
      GList           *layers;
      GList           *list;
      ClayDrawable    *drawable;

      if (progress)
        clay_progress_start (progress, FALSE, "%s", _("Dithering"));

      queue        = clay_object_queue_new (progress);
      sub_progress = CLAY_PROGRESS (queue);

      layers = clay_image_get_layer_list (image);

      for (list = layers; list; list = g_list_next (list))
        {
          if (! clay_viewable_get_children (list->data) &&
              ! clay_item_is_text_layer (list->data))
            {
              clay_object_queue_push (queue, list->data);
            }
        }

      g_list_free (layers);

      while ((drawable = clay_object_queue_pop (queue)))
        {
          clay_drawable_apply_operation (drawable, sub_progress,
                                         _("Dithering"),
                                         dither);
        }

      g_object_unref (queue);

      if (progress)
        clay_progress_end (progress);

      g_object_unref (dither);
    }
}
