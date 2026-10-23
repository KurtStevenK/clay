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
#include "libclaybase/clayparasiteio.h"
#include "libclaycolor/claycolor.h"

#include "core/core-types.h"

#include "core/clay.h"
#include "core/claybrushpipe.h"
#include "core/claybrushpipe-load.h"
#include "core/claybrush-private.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/claylayer-new.h"
#include "core/clayparamspecs.h"
#include "core/claytempbuf.h"

#include "pdb/clayprocedure.h"

#include "file-data-gbr.h"
#include "file-data-gih.h"

#include "clay-intl.h"


/*  local function prototypes  */

static ClayImage     * file_gih_pipe_to_image (Clay          *clay,
                                               ClayBrushPipe *pipe);
static ClayBrushPipe * file_gih_image_to_pipe (ClayImage     *image,
                                               const gchar   *name,
                                               gdouble        spacing,
                                               const gchar   *paramstring);


/*  public functions  */

ClayValueArray *
file_gih_load_invoker (ClayProcedure         *procedure,
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
      GList *list = clay_brush_pipe_load (context, file, input, error);

      if (list)
        {
          ClayBrushPipe *pipe = list->data;

          g_list_free (list);

          image = file_gih_pipe_to_image (clay, pipe);
          g_object_unref (pipe);
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
file_gih_save_invoker (ClayProcedure         *procedure,
                       Clay                  *clay,
                       ClayContext           *context,
                       ClayProgress          *progress,
                       const ClayValueArray  *args,
                       GError               **error)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  ClayBrushPipe  *pipe;
  const gchar    *name;
  const gchar    *params;
  GFile          *file;
  gint            spacing;
  gboolean        success;

  clay_set_busy (clay);

  image   = g_value_get_object (clay_value_array_index (args, 1));
  /* XXX: drawable list is currently unused. GIH saving just uses the
   * whole layer list.
   */
  file    = g_value_get_object (clay_value_array_index (args, 3));
  spacing = g_value_get_int    (clay_value_array_index (args, 4));
  name    = g_value_get_string (clay_value_array_index (args, 5));
  params  = g_value_get_string (clay_value_array_index (args, 6));

  pipe = file_gih_image_to_pipe (image, name, spacing, params);

  clay_data_set_file (CLAY_DATA (pipe), file, TRUE, TRUE);

  success = clay_data_save (CLAY_DATA (pipe), error);

  g_object_unref (pipe);

  return_vals = clay_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  clay_unset_busy (clay);

  return return_vals;
}


/*  private functions  */

static ClayImage *
file_gih_pipe_to_image (Clay          *clay,
                        ClayBrushPipe *pipe)
{
  ClayImage         *image;
  const gchar       *name;
  ClayImageBaseType  base_type;
  ClayParasite      *parasite;
  gchar              spacing[8];
  gint               i;

  if (clay_brush_get_pixmap (pipe->current))
    base_type = CLAY_RGB;
  else
    base_type = CLAY_GRAY;

  name = clay_object_get_name (pipe);

  image = clay_image_new (clay, 1, 1, base_type,
                          CLAY_PRECISION_U8_NON_LINEAR);

  parasite = clay_parasite_new ("clay-brush-pipe-name",
                                CLAY_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  clay_image_parasite_attach (image, parasite, FALSE);
  clay_parasite_free (parasite);

  g_snprintf (spacing, sizeof (spacing), "%d",
              clay_brush_get_spacing (CLAY_BRUSH (pipe)));

  parasite = clay_parasite_new ("clay-brush-pipe-spacing",
                                CLAY_PARASITE_PERSISTENT,
                                strlen (spacing) + 1, spacing);
  clay_image_parasite_attach (image, parasite, FALSE);
  clay_parasite_free (parasite);

  for (i = 0; i < pipe->n_brushes; i++)
    {
      ClayLayer *layer;

      layer = file_gbr_brush_to_layer (image, pipe->brushes[i]);
      clay_image_add_layer (image, layer, NULL, i, FALSE);
    }

  if (pipe->params)
    {
      ClayPixPipeParams  params;
      gchar             *paramstring;

      /* Since we do not (yet) load the pipe as described in the
       * header, but use one layer per brush, we have to alter the
       * paramstring before attaching it as a parasite.
       *
       * (this comment copied over from file-gih, whatever "as
       * described in the header" means) -- mitch
       */

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      clay_pixpipe_params_init (&params);
      clay_pixpipe_params_parse (pipe->params, &params);
      G_GNUC_END_IGNORE_DEPRECATIONS

      params.cellwidth  = clay_image_get_width  (image);
      params.cellheight = clay_image_get_height (image);
      params.cols       = 1;
      params.rows       = 1;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      paramstring = clay_pixpipe_params_build (&params);
      G_GNUC_END_IGNORE_DEPRECATIONS
      if (paramstring)
        {
          parasite = clay_parasite_new ("clay-brush-pipe-parameters",
                                        CLAY_PARASITE_PERSISTENT,
                                        strlen (paramstring) + 1,
                                        paramstring);
          clay_image_parasite_attach (image, parasite, FALSE);
          clay_parasite_free (parasite);
          g_free (paramstring);
        }

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      clay_pixpipe_params_free (&params);
      G_GNUC_END_IGNORE_DEPRECATIONS
    }

  return image;
}

static ClayBrushPipe *
file_gih_image_to_pipe (ClayImage   *image,
                        const gchar *name,
                        gdouble      spacing,
                        const gchar *paramstring)
{
  ClayBrushPipe     *pipe;
  ClayPixPipeParams  params;
  GList             *layers;
  GList             *list;
  GList             *brushes = NULL;
  gint               image_width;
  gint               image_height;
  gint               i;

  pipe = g_object_new (CLAY_TYPE_BRUSH_PIPE,
                       "name",      name,
                       "mime-type", "image/x-clay-gih",
                       "spacing",   spacing,
                       NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  clay_pixpipe_params_init (&params);
  clay_pixpipe_params_parse (paramstring, &params);
  G_GNUC_END_IGNORE_DEPRECATIONS

  image_width  = clay_image_get_width  (image);
  image_height = clay_image_get_height (image);

  layers = clay_image_get_layer_iter (image);

  for (list = layers; list; list = g_list_next (list))
    {
      ClayLayer *layer = list->data;
      gint       width;
      gint       height;
      gint       offset_x;
      gint       offset_y;
      gint       row;

      width  = clay_item_get_width  (CLAY_ITEM (layer));
      height = clay_item_get_height (CLAY_ITEM (layer));

      clay_item_get_offset (CLAY_ITEM (layer), &offset_x, &offset_y);

      /* Since we assume positive layer offsets we need to make sure this
       * is always the case or we will crash for grayscale layers.
       * See issue #6436. */
      if (offset_x < 0)
        {
          g_warning (_("Negative x offset: %d for layer %s corrected."),
                     offset_x, clay_object_get_name (layer));
          width += offset_x;
          offset_x = 0;
        }
      if (offset_y < 0)
        {
          g_warning (_("Negative y offset: %d for layer %s corrected."),
                     offset_y, clay_object_get_name (layer));
          height += offset_y;
          offset_y = 0;
        }

      for (row = 0; row < params.rows; row++)
        {
          gint y, ynext;
          gint thisy, thish;
          gint col;

          y     = (row * image_height) / params.rows;
          ynext = ((row + 1) * image_height / params.rows);

          /* Assume layer is offset to positive direction in x and y.
           * That's reasonable, as otherwise all of the layer
           * won't be visible.
           * thisy and thisx are in the drawable's coordinate space.
           */
          thisy = MAX (0, y - offset_y);
          thish = (ynext - offset_y) - thisy;
          thish = MIN (thish, height - thisy);

          for (col = 0; col < params.cols; col++)
            {
              ClayBrush *brush;
              gint       x, xnext;
              gint       thisx, thisw;

              x     = (col * image_width / params.cols);
              xnext = ((col + 1) * image_width / params.cols);
              thisx = MAX (0, x - offset_x);
              thisw = (xnext - offset_x) - thisx;
              thisw = MIN (thisw, width - thisx);

              brush = file_gbr_drawable_to_brush (CLAY_DRAWABLE (layer),
                                                  GEGL_RECTANGLE (thisx, thisy,
                                                                  thisw, thish),
                                                  clay_object_get_name (layer),
                                                  spacing);

              if (brush)
                brushes = g_list_prepend (brushes, brush);
            }
        }
    }

  brushes = g_list_reverse (brushes);

  pipe->n_brushes = g_list_length (brushes);
  pipe->brushes   = g_new0 (ClayBrush *, pipe->n_brushes);

  for (list = brushes, i = 0; list; list = g_list_next (list), i++)
    pipe->brushes[i] = list->data;

  g_list_free (brushes);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  clay_pixpipe_params_free (&params);
  G_GNUC_END_IGNORE_DEPRECATIONS

  clay_brush_pipe_set_params (pipe, paramstring);

  return pipe;
}
