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

#include <gio/gio.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clay-gegl-utils.h"

#include "claychannel.h"
#include "claydrawable.h"
#include "claydrawable-foreground-extract.h"
#include "clayimage.h"
#include "clayprogress.h"

#include "clay-intl.h"


/*  public functions  */

GeglBuffer *
clay_drawable_foreground_extract (ClayDrawable      *drawable,
                                  ClayMattingEngine  engine,
                                  gint               global_iterations,
                                  gint               levin_levels,
                                  gint               levin_active_levels,
                                  GeglBuffer        *trimap,
                                  ClayProgress      *progress)
{
  GeglBuffer    *drawable_buffer;
  GeglNode      *gegl;
  GeglNode      *input_node;
  GeglNode      *trimap_node;
  GeglNode      *matting_node;
  GeglNode      *output_node;
  GeglBuffer    *buffer;
  GeglProcessor *processor;
  gdouble        value;
  gint           off_x, off_y;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (trimap), NULL);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), NULL);

  progress = clay_progress_start (progress, FALSE,
                                  _("Computing alpha of unknown pixels"));

  drawable_buffer = clay_drawable_get_buffer (drawable);

  gegl = gegl_node_new ();

  trimap_node = gegl_node_new_child (gegl,
                                     "operation", "gegl:buffer-source",
                                     "buffer",    trimap,
                                     NULL);
  input_node = gegl_node_new_child (gegl,
                                    "operation", "gegl:buffer-source",
                                    "buffer",    drawable_buffer,
                                    NULL);
  output_node = gegl_node_new_child (gegl,
                                     "operation", "gegl:buffer-sink",
                                     "buffer",    &buffer,
                                     "format",    NULL,
                                     NULL);

  if (engine == CLAY_MATTING_ENGINE_GLOBAL)
    {
      matting_node = gegl_node_new_child (gegl,
                                          "operation",  "gegl:matting-global",
                                          "iterations", global_iterations,
                                          NULL);
    }
  else
    {
      matting_node = gegl_node_new_child (gegl,
                                          "operation",     "gegl:matting-levin",
                                          "levels",        levin_levels,
                                          "active_levels", levin_active_levels,
                                          NULL);
    }

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

  if (off_x || off_y)
    {
      GeglNode *pre;
      GeglNode *post;

      pre = gegl_node_new_child (gegl,
                                 "operation", "gegl:translate",
                                 "x", -1.0 * off_x,
                                 "y", -1.0 * off_y,
                                 NULL);
      post = gegl_node_new_child (gegl,
                                  "operation", "gegl:translate",
                                  "x", 1.0 * off_x,
                                  "y", 1.0 * off_y,
                                  NULL);

      gegl_node_link (trimap_node, pre);
      gegl_node_connect (pre,  "output", matting_node, "aux");
      gegl_node_link_many (input_node, matting_node, post, output_node, NULL);
    }
  else
    {
      gegl_node_connect (input_node,   "output", matting_node, "input");
      gegl_node_connect (trimap_node,  "output", matting_node, "aux");
      gegl_node_connect (matting_node, "output", output_node,  "input");
    }

  processor = gegl_node_new_processor (output_node, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        clay_progress_set_value (progress, value);
    }

  if (progress)
    clay_progress_end (progress);

  g_object_unref (processor);

  g_object_unref (gegl);

  return buffer;
}
