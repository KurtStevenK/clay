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

#include "core-types.h"

#include "claybrushpipe.h"
#include "claybrushpipe-save.h"


gboolean
clay_brush_pipe_save (ClayData       *data,
                      GOutputStream  *output,
                      GError        **error)
{
  ClayBrushPipe *pipe = CLAY_BRUSH_PIPE (data);
  const gchar   *name;
  gint           i;

  name = clay_object_get_name (pipe);

  if (! g_output_stream_printf (output, NULL, NULL, error,
                                "%s\n%d %s\n",
                                name, pipe->n_brushes, pipe->params))
    {
      return FALSE;
    }

  for (i = 0; i < pipe->n_brushes; i++)
    {
      ClayBrush *brush = pipe->brushes[i];

      if (brush &&
          ! CLAY_DATA_GET_CLASS (brush)->save (CLAY_DATA (brush),
                                               output, error))
        {
          return FALSE;
        }
    }

  return TRUE;
}
