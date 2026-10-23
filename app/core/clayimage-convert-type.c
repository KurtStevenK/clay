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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"

#include "clay.h"
#include "claydrawable.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-colormap.h"
#include "clayimage-convert-type.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"

#include "clay-intl.h"


gboolean
clay_image_convert_type (ClayImage          *image,
                         ClayImageBaseType   new_type,
                         ClayColorProfile   *dest_profile,
                         ClayProgress       *progress,
                         GError            **error)
{
  ClayColorProfile  *src_profile;
  ClayImageBaseType  old_type;
  const Babl        *new_layer_format;
  ClayObjectQueue   *queue;
  GList             *all_layers;
  ClayDrawable      *drawable;
  const gchar       *undo_desc = NULL;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (new_type != clay_image_get_base_type (image), FALSE);
  g_return_val_if_fail (new_type != CLAY_INDEXED, FALSE);
  g_return_val_if_fail (clay_babl_is_valid (new_type,
                                            clay_image_get_precision (image)),
                        FALSE);
  g_return_val_if_fail (dest_profile == NULL || CLAY_IS_COLOR_PROFILE (dest_profile),
                        FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  new_layer_format = clay_babl_format (new_type,
                                       clay_image_get_precision (image),
                                       TRUE,
                                       clay_image_get_layer_space (image));

  if (dest_profile &&
      ! clay_image_validate_color_profile_by_format (new_layer_format,
                                                     dest_profile,
                                                     NULL, error))
    {
      return FALSE;
    }

  switch (new_type)
    {
    case CLAY_RGB:
      undo_desc = C_("undo-type", "Convert Image to RGB");
      break;

    case CLAY_GRAY:
      undo_desc = C_("undo-type", "Convert Image to Grayscale");
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  clay_set_busy (image->clay);

  queue    = clay_object_queue_new (progress);
  progress = CLAY_PROGRESS (queue);

  all_layers = clay_image_get_layer_list (image);
  clay_object_queue_push_list (queue, all_layers);
  g_list_free (all_layers);

  g_object_freeze_notify (G_OBJECT (image));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_CONVERT,
                               undo_desc);

  src_profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));

  /*  Push the image type to the stack  */
  clay_image_undo_push_image_type (image, NULL);

  /*  Set the new base type  */
  old_type = clay_image_get_base_type (image);

  g_object_set (image, "base-type", new_type, NULL);

  /*  When converting to/from GRAY, always convert to the new type's
   *  builtin profile as a fallback, we need one for convert_type on
   *  the same image
   */
  if (old_type == CLAY_GRAY ||
      new_type == CLAY_GRAY)
    {
      if (! dest_profile)
        dest_profile = clay_image_get_builtin_color_profile (image);
    }

  while ((drawable = clay_object_queue_pop (queue)))
    {
      clay_drawable_convert_type (drawable, image,
                                  new_type,
                                  clay_drawable_get_precision (drawable),
                                  clay_drawable_has_alpha (drawable),
                                  src_profile,
                                  dest_profile,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  TRUE, progress);
    }

  if (old_type == CLAY_INDEXED)
    clay_image_unset_colormap (image, TRUE);

  /*  When converting to/from GRAY, set the new profile.
   */
  if (old_type == CLAY_GRAY ||
      new_type == CLAY_GRAY)
    {
      clay_image_set_color_profile (image, dest_profile, NULL);
    }

  clay_image_undo_group_end (image);

  clay_image_mode_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_object_unref (queue);

  clay_unset_busy (image->clay);

  return TRUE;
}
