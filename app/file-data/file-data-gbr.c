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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core/core-types.h"

#include "core/clay.h"
#include "core/claybrush.h"
#include "core/claybrush-load.h"
#include "core/claybrush-private.h"
#include "core/claycontainer.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayimage-merge.h"
#include "core/clayimage-new.h"
#include "core/clayimage-resize.h"
#include "core/claylayer-new.h"
#include "core/clayparamspecs.h"
#include "core/claypickable.h"
#include "core/claytempbuf.h"

#include "pdb/clayprocedure.h"

#include "file-data-gbr.h"

#include "clay-intl.h"


/*  local function prototypes  */

static ClayImage * file_gbr_brush_to_image (Clay          *clay,
                                            ClayBrush     *brush);
static ClayBrush * file_gbr_image_to_brush (ClayImage     *image,
                                            ClayContext   *context,
                                            gint           n_drawables,
                                            ClayDrawable **drawables,
                                            const gchar   *name,
                                            gdouble        spacing);


/*  public functions  */

ClayValueArray *
file_gbr_load_invoker (ClayProcedure         *procedure,
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
      ClayBrush *brush = clay_brush_load_brush (context, file, input, error);

      if (brush)
        {
          image = file_gbr_brush_to_image (clay, brush);
          g_object_unref (brush);
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
file_gbr_save_invoker (ClayProcedure         *procedure,
                       Clay                  *clay,
                       ClayContext           *context,
                       ClayProgress          *progress,
                       const ClayValueArray  *args,
                       GError               **error)
{
  ClayValueArray  *return_vals;
  ClayImage       *image;
  ClayDrawable   **drawables;
  gint             n_drawables;
  ClayBrush       *brush;
  const gchar     *name;
  GFile           *file;
  gint             spacing;
  gboolean         success;

  clay_set_busy (clay);

  image       = g_value_get_object (clay_value_array_index (args, 1));
  drawables   = (ClayDrawable **) g_value_get_boxed (clay_value_array_index (args, 2));
  n_drawables = clay_core_object_array_get_length ((GObject **) drawables);
  file        = g_value_get_object (clay_value_array_index (args, 3));
  spacing     = g_value_get_int    (clay_value_array_index (args, 4));
  name        = g_value_get_string (clay_value_array_index (args, 5));

  brush = file_gbr_image_to_brush (image, context, n_drawables, drawables, name, spacing);

  clay_data_set_file (CLAY_DATA (brush), file, TRUE, TRUE);

  success = clay_data_save (CLAY_DATA (brush), error);

  g_object_unref (brush);

  return_vals = clay_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  clay_unset_busy (clay);

  return return_vals;
}

ClayLayer *
file_gbr_brush_to_layer (ClayImage *image,
                         ClayBrush *brush)
{
  ClayLayer    *layer;
  const Babl   *format;
  gboolean      alpha;
  gint          width;
  gint          height;
  gint          image_width;
  gint          image_height;
  ClayTempBuf  *mask;
  ClayTempBuf  *pixmap;
  GeglBuffer   *buffer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_BRUSH (brush), NULL);

  mask   = clay_brush_get_mask   (brush);
  pixmap = clay_brush_get_pixmap (brush);

  if (pixmap)
    alpha = TRUE;
  else
    alpha = FALSE;

  width  = clay_temp_buf_get_width  (mask);
  height = clay_temp_buf_get_height (mask);

  image_width  = clay_image_get_width  (image);
  image_height = clay_image_get_height (image);

  if (width > image_width || height > image_height)
    {
      gint new_width  = MAX (image_width,  width);
      gint new_height = MAX (image_height, height);

      clay_image_resize (image, clay_get_user_context (image->clay),
                         new_width, new_height,
                         (new_width  - image_width) / 2,
                         (new_height - image_height) / 2,
                         NULL);

      image_width  = new_width;
      image_height = new_height;
    }

  format = clay_image_get_layer_format (image, alpha);

  layer = clay_layer_new (image, width, height, format,
                          clay_object_get_name (brush),
                          1.0, CLAY_LAYER_MODE_NORMAL);

  clay_item_set_offset (CLAY_ITEM (layer),
                        (image_width  - width)  / 2,
                        (image_height - height) / 2);

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  if (pixmap)
    {
      guchar *pixmap_data;
      guchar *mask_data;
      guchar *p;
      guchar *m;
      gint    i;

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       babl_format ("R'G'B' u8"),
                       clay_temp_buf_get_data (pixmap), GEGL_AUTO_ROWSTRIDE);

      pixmap_data = gegl_buffer_linear_open (buffer, NULL, NULL, NULL);
      mask_data   = clay_temp_buf_get_data (mask);

      for (i = 0, p = pixmap_data, m = mask_data;
           i < width * height;
           i++, p += 4, m += 1)
        {
          p[3] = *m;
        }

      gegl_buffer_linear_close (buffer, pixmap_data);
    }
  else
    {
      guchar *mask_data = clay_temp_buf_get_data (mask);
      gint    i;

      for (i = 0; i < width * height; i++)
        mask_data[i] = 255 - mask_data[i];

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       babl_format ("Y' u8"),
                       mask_data, GEGL_AUTO_ROWSTRIDE);
    }

  return layer;
}

ClayBrush *
file_gbr_drawable_to_brush (ClayDrawable        *drawable,
                            const GeglRectangle *rect,
                            const gchar         *name,
                            gdouble              spacing)
{
  ClayBrush   *brush;
  GeglBuffer  *buffer;
  ClayTempBuf *mask;
  ClayTempBuf *pixmap = NULL;
  gint         width;
  gint         height;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  buffer = clay_drawable_get_buffer (drawable);
  width  = rect->width;
  height = rect->height;

  if (width < 0 || height < 0)
    return NULL;

  brush = g_object_new (CLAY_TYPE_BRUSH,
                        "name",      name,
                        "mime-type", "image/x-clay-gbr",
                        "spacing",   spacing,
                        NULL);

  mask = clay_temp_buf_new (width, height, babl_format ("Y u8"));

  if (clay_drawable_is_gray (drawable))
    {
      guchar *m = clay_temp_buf_get_data (mask);
      gint    i;

      if (clay_drawable_has_alpha (drawable))
        {
          GeglBufferIterator *iter;
          GeglBuffer         *mask_buffer;

          mask_buffer = clay_temp_buf_create_buffer (mask);

          iter = gegl_buffer_iterator_new (buffer, rect, 0,
                                           babl_format ("Y'A float"),
                                           GEGL_ACCESS_READ, GEGL_ABYSS_NONE,
                                           2);
          gegl_buffer_iterator_add (iter, mask_buffer, rect, 0,
                                    babl_format ("Y' float"),
                                    GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *data   = (gfloat *) iter->items[0].data;
              gfloat *m_data = (gfloat *) iter->items[1].data;
              gint    j;

              for (j = 0; j < iter->length; j++)
                {
                  /* Composite brush color on top of white (1.0, 1.0, 1.0, 1.0) */
                  if (data[1] < 1.0)
                    data[0] = (1.0 - data[1]) + (data[0] * data[1]);

                  m_data[0] = data[0];

                  data += 2;
                  m_data++;
                }
            }

          gegl_buffer_get (mask_buffer, rect, 1.0,
                           babl_format ("Y' u8"), m,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
          g_object_unref (mask_buffer);
        }
      else
        {
          gegl_buffer_get (buffer, rect, 1.0,
                           babl_format ("Y' u8"), m,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

      /*  invert  */
      for (i = 0; i < width * height; i++)
        m[i] = 255 - m[i];
    }
  else
    {
      pixmap = clay_temp_buf_new (width, height, babl_format ("R'G'B' u8"));

      gegl_buffer_get (buffer, rect, 1.0,
                       babl_format ("R'G'B' u8"),
                       clay_temp_buf_get_data (pixmap),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (buffer, rect, 1.0,
                       babl_format ("A u8"),
                       clay_temp_buf_get_data (mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }


  brush->priv->mask   = mask;
  brush->priv->pixmap = pixmap;

  return brush;
}


/*  private functions  */

static ClayImage *
file_gbr_brush_to_image (Clay      *clay,
                         ClayBrush *brush)
{
  ClayImage         *image;
  ClayLayer         *layer;
  ClayImageBaseType  base_type;
  gint               width;
  gint               height;
  ClayTempBuf       *mask   = clay_brush_get_mask   (brush);
  ClayTempBuf       *pixmap = clay_brush_get_pixmap (brush);
  GString           *string;
  ClayConfigWriter  *writer;
  ClayParasite      *parasite;

  if (pixmap)
    base_type = CLAY_RGB;
  else
    base_type = CLAY_GRAY;

  width  = clay_temp_buf_get_width  (mask);
  height = clay_temp_buf_get_height (mask);

  image = clay_image_new (clay, width, height, base_type,
                          CLAY_PRECISION_U8_NON_LINEAR);

  string = g_string_new (NULL);
  writer = clay_config_writer_new_from_string (string);

  clay_config_writer_open (writer, "spacing");
  clay_config_writer_printf (writer, "%d", clay_brush_get_spacing (brush));
  clay_config_writer_close (writer);

  clay_config_writer_linefeed (writer);

  clay_config_writer_open (writer, "description");
  clay_config_writer_string (writer, clay_object_get_name (brush));
  clay_config_writer_close (writer);

  clay_config_writer_finish (writer, NULL, NULL);

  parasite = clay_parasite_new ("ClayProcedureConfigRun-file-gbr-export-last",
                                CLAY_PARASITE_PERSISTENT,
                                string->len + 1, string->str);
  clay_image_parasite_attach (image, parasite, FALSE);
  clay_parasite_free (parasite);

  g_string_free (string, TRUE);

  layer = file_gbr_brush_to_layer (image, brush);
  clay_image_add_layer (image, layer, NULL, 0, FALSE);

  return image;
}

static ClayBrush *
file_gbr_image_to_brush (ClayImage     *image,
                         ClayContext   *context,
                         gint           n_drawables,
                         ClayDrawable **drawables,
                         const gchar   *name,
                         gdouble        spacing)
{
  ClayBrush    *brush    = NULL;
  ClayImage    *subimage = NULL;
  ClayDrawable *drawable;
  gint          width;
  gint          height;

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
      drawable = CLAY_DRAWABLE (clay_image_merge_visible_layers (subimage, context, CLAY_CLIP_TO_IMAGE,
                                                                 FALSE, TRUE, NULL));
      clay_pickable_flush (CLAY_PICKABLE (subimage));
    }
  else
    {
      drawable = drawables[0];
    }
  width  = clay_item_get_width  (CLAY_ITEM (drawable));
  height = clay_item_get_height (CLAY_ITEM (drawable));

  brush = file_gbr_drawable_to_brush (drawable,
                                      GEGL_RECTANGLE (0, 0, width, height),
                                      name, spacing);
  g_clear_object (&subimage);

  return brush;
}
