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
#include "libclayconfig/clayconfig.h"

#include "core/core-types.h"

#include "gegl/clay-babl.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayimage-new.h"
#include "core/clayimage-resize.h"
#include "core/claylayer-new.h"
#include "core/clayparamspecs.h"
#include "core/claypattern.h"
#include "core/claypattern-load.h"
#include "core/claypickable.h"
#include "core/claytempbuf.h"

#include "pdb/clayprocedure.h"

#include "file-data-pat.h"

#include "clay-intl.h"


/*  local function prototypes  */

static ClayImage   * file_pat_pattern_to_image (Clay          *clay,
                                                ClayPattern   *pattern);
static ClayPattern * file_pat_image_to_pattern (ClayImage     *image,
                                                ClayContext   *context,
                                                gint           n_drawables,
                                                ClayDrawable **drawables,
                                                const gchar   *name);


/*  public functions  */

ClayValueArray *
file_pat_load_invoker (ClayProcedure         *procedure,
                       Clay                  *clay,
                       ClayContext           *context,
                       ClayProgress          *progress,
                       const ClayValueArray  *args,
                       GError               **error)
{
  ClayValueArray *return_vals;
  ClayImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  clay_set_busy (clay);

  file = g_value_get_object (clay_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      GList *list = clay_pattern_load (context, file, input, error);

      if (list)
        {
          ClayPattern *pattern = list->data;

          g_list_free (list);

          image = file_pat_pattern_to_image (clay, pattern);
          g_object_unref (pattern);
        }

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  clay_file_get_utf8_name (file));
    }

  return_vals = clay_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (clay_value_array_index (return_vals, 1), image);

  clay_unset_busy (clay);

  return return_vals;
}

ClayValueArray *
file_pat_save_invoker (ClayProcedure         *procedure,
                       Clay                  *clay,
                       ClayContext           *context,
                       ClayProgress          *progress,
                       const ClayValueArray  *args,
                       GError               **error)
{
  ClayValueArray  *return_vals;
  ClayImage       *image;
  ClayPattern     *pattern;
  const gchar     *name;
  GFile           *file;
  ClayDrawable   **drawables;
  gint             n_drawables;
  gboolean         success;

  clay_set_busy (clay);

  image       = g_value_get_object (clay_value_array_index (args, 1));
  drawables   = (ClayDrawable **) g_value_get_boxed (clay_value_array_index (args, 2));
  n_drawables = clay_core_object_array_get_length ((GObject **) drawables);
  file        = g_value_get_object (clay_value_array_index (args, 3));
  name        = g_value_get_string (clay_value_array_index (args, 4));

  pattern = file_pat_image_to_pattern (image, context, n_drawables, drawables, name);

  clay_data_set_file (CLAY_DATA (pattern), file, TRUE, TRUE);

  success = clay_data_save (CLAY_DATA (pattern), error);

  g_object_unref (pattern);

  return_vals = clay_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  clay_unset_busy (clay);

  return return_vals;
}


/*  private functions  */

static ClayImage *
file_pat_pattern_to_image (Clay        *clay,
                           ClayPattern *pattern)
{
  ClayImage         *image;
  ClayLayer         *layer;
  const Babl        *format;
  ClayImageBaseType  base_type;
  gboolean           alpha;
  gint               width;
  gint               height;
  ClayTempBuf       *mask   = clay_pattern_get_mask (pattern);
  GeglBuffer        *buffer;
  GString           *string;
  ClayConfigWriter  *writer;
  ClayParasite      *parasite;

  format = clay_temp_buf_get_format (mask);

  switch (babl_format_get_bytes_per_pixel (format))
    {
    case 1:
      base_type = CLAY_GRAY;
      alpha     = FALSE;
      break;

    case 2:
      base_type = CLAY_GRAY;
      alpha     = TRUE;
      break;

    case 3:
      base_type = CLAY_RGB;
      alpha     = FALSE;
      break;

    case 4:
      base_type = CLAY_RGB;
      alpha     = TRUE;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  width  = clay_temp_buf_get_width  (mask);
  height = clay_temp_buf_get_height (mask);

  image = clay_image_new (clay, width, height, base_type,
                          CLAY_PRECISION_U8_NON_LINEAR);

  string = g_string_new (NULL);
  writer = clay_config_writer_new_from_string (string);

  clay_config_writer_open (writer, "description");
  clay_config_writer_string (writer, clay_object_get_name (pattern));
  clay_config_writer_close (writer);

  clay_config_writer_finish (writer, NULL, NULL);

  parasite = clay_parasite_new ("ClayProcedureConfigRun-file-pat-export-last",
                                CLAY_PARASITE_PERSISTENT,
                                string->len + 1, string->str);
  clay_image_parasite_attach (image, parasite, FALSE);
  clay_parasite_free (parasite);

  g_string_free (string, TRUE);

  format = clay_image_get_layer_format (image, alpha);

  layer = clay_layer_new (image, width, height, format,
                          clay_object_get_name (pattern),
                          1.0, CLAY_LAYER_MODE_NORMAL);
  clay_image_add_layer (image, layer, NULL, 0, FALSE);

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL,
                   clay_temp_buf_get_data (mask), GEGL_AUTO_ROWSTRIDE);

  return image;
}

static ClayPattern *
file_pat_image_to_pattern (ClayImage     *image,
                           ClayContext   *context,
                           gint           n_drawables,
                           ClayDrawable **drawables,
                           const gchar   *name)
{
  ClayPattern *pattern;
  ClayImage   *subimage = NULL;
  const Babl  *format;
  gint         width;
  gint         height;

  g_return_val_if_fail (n_drawables > 0, NULL);
  g_return_val_if_fail (drawables != NULL, NULL);

  if (n_drawables > 1)
    {
      GList *drawable_list = NULL;

      for (gint i = 0; i < n_drawables; i++)
        drawable_list = g_list_prepend (drawable_list, drawables[i]);

      subimage = clay_image_new_from_drawables (image->clay, drawable_list, FALSE, FALSE);
      g_list_free (drawable_list);
      clay_container_remove (image->clay->images, CLAY_OBJECT (subimage));
      clay_image_resize_to_layers (subimage, context,
                                   NULL, NULL, NULL, NULL, NULL);
      width  = clay_image_get_width (subimage);
      height = clay_image_get_width (subimage);

      clay_pickable_flush (CLAY_PICKABLE (subimage));
    }
  else
    {
      width  = clay_item_get_width  (CLAY_ITEM (drawables[0]));
      height = clay_item_get_height (CLAY_ITEM (drawables[0]));
    }

  format = clay_babl_format (clay_drawable_is_gray (drawables[0]) ?
                             CLAY_GRAY : CLAY_RGB,
                             CLAY_PRECISION_U8_NON_LINEAR,
                             (subimage && clay_image_has_alpha (subimage)) ||
                             clay_drawable_has_alpha (drawables[0]),
                             NULL);

  pattern = g_object_new (CLAY_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", "image/x-clay-pat",
                          NULL);

  pattern->mask = clay_temp_buf_new (width, height, format);

  gegl_buffer_get (subimage != NULL ? clay_pickable_get_buffer (CLAY_PICKABLE (subimage)) :
                                      clay_drawable_get_buffer (drawables[0]),
                   GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, clay_temp_buf_get_data (pattern->mask),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_clear_object (&subimage);

  return pattern;
}
