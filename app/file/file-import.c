/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-import.c
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

#include "core/core-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-color-profile.h"
#include "core/clayimage-convert-precision.h"
#include "core/clayimage-rotate.h"
#include "core/claylayer.h"
#include "core/clayprogress.h"

#include "text/claytextlayer.h"

#include "file-import.h"


/*  public functions  */

void
file_import_image (ClayImage    *image,
                   ClayContext  *context,
                   GFile        *file,
                   gboolean      interactive,
                   ClayProgress *progress)
{
  ClayCoreConfig *config;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  config = image->clay->config;

  if (interactive && clay_image_get_base_type (image) != CLAY_INDEXED)
    {
      if (config->import_promote_float)
        {
          ClayPrecision old_precision = clay_image_get_precision (image);

          if (old_precision != CLAY_PRECISION_FLOAT_LINEAR)
            {
              clay_image_convert_precision (image,
                                            CLAY_PRECISION_FLOAT_LINEAR,
                                            GEGL_DITHER_NONE,
                                            GEGL_DITHER_NONE,
                                            GEGL_DITHER_NONE,
                                            progress);

              if (config->import_promote_dither &&
                  old_precision == CLAY_PRECISION_U8_NON_LINEAR)
                {
                  clay_image_convert_dither_u8 (image, progress);
                }
            }
        }

      if (config->import_add_alpha)
        {
          GList *layers = clay_image_get_layer_list (image);
          GList *list;

          for (list = layers; list; list = g_list_next (list))
            {
              if (! clay_viewable_get_children (list->data) &&
                  ! clay_item_is_text_layer (list->data)    &&
                  ! clay_drawable_has_alpha (list->data))
                {
                  clay_layer_add_alpha (list->data);
                }
            }

          g_list_free (layers);
        }
    }

  clay_image_import_color_profile (image, context, progress, interactive);
  clay_image_import_rotation_metadata (image, context, progress, interactive);

  /* Remember the import source */
  clay_image_set_imported_file (image, file);

  /* We shall treat this file as an Untitled file */
  clay_image_set_file (image, NULL);
}
