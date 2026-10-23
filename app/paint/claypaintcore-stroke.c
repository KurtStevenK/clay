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

#include "libclaymath/claymath.h"

#include "paint-types.h"

#include "core/clayboundary.h"
#include "core/claydrawable.h"
#include "core/clayerror.h"
#include "core/claycoords.h"

#include "path/claypath.h"
#include "path/claystroke.h"

#include "claypaintcore.h"
#include "claypaintcore-stroke.h"
#include "claypaintoptions.h"

#include "clay-intl.h"


static void clay_paint_core_stroke_emulate_dynamics (ClayCoords *coords,
                                                     gint        length);


static const ClayCoords default_coords = CLAY_COORDS_DEFAULT_VALUES;


gboolean
clay_paint_core_stroke (ClayPaintCore     *core,
                        ClayDrawable      *drawable,
                        ClayPaintOptions  *paint_options,
                        ClayCoords        *strokes,
                        gsize              n_strokes,
                        gboolean           push_undo,
                        GError           **error)
{
  GList    *drawables;
  gboolean  success = FALSE;

  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), FALSE);
  g_return_val_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (strokes != NULL, FALSE);
  g_return_val_if_fail (n_strokes > 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  drawables = g_list_prepend (NULL, drawable);

  if (clay_paint_core_start (core, drawables, paint_options, &strokes[0],
                             error))
    {
      gint i;

      core->last_coords = strokes[0];

      clay_paint_core_paint (core, drawables, paint_options,
                             CLAY_PAINT_STATE_INIT, 0);

      clay_paint_core_paint (core, drawables, paint_options,
                             CLAY_PAINT_STATE_MOTION, 0);

      for (i = 1; i < n_strokes; i++)
        {
          clay_paint_core_interpolate (core, drawables, paint_options,
                                       &strokes[i], 0);
        }

      clay_paint_core_paint (core, drawables, paint_options,
                             CLAY_PAINT_STATE_FINISH, 0);

      clay_paint_core_finish (core, drawables, push_undo);

      clay_paint_core_cleanup (core);

      success = TRUE;
    }

  g_list_free (drawables);

  return success;
}

gboolean
clay_paint_core_stroke_boundary (ClayPaintCore      *core,
                                 ClayDrawable       *drawable,
                                 ClayPaintOptions   *paint_options,
                                 gboolean            emulate_dynamics,
                                 const ClayBoundSeg *bound_segs,
                                 gint                n_bound_segs,
                                 gint                offset_x,
                                 gint                offset_y,
                                 gboolean            push_undo,
                                 GError            **error)
{
  GList        *drawables;
  ClayBoundSeg *stroke_segs;
  gint          n_stroke_segs;
  ClayCoords   *coords;
  gboolean      initialized = FALSE;
  gint          n_coords;
  gint          seg;
  gint          s;

  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), FALSE);
  g_return_val_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (bound_segs != NULL && n_bound_segs > 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  stroke_segs = clay_boundary_sort (bound_segs, n_bound_segs,
                                    &n_stroke_segs);

  if (n_stroke_segs == 0)
    return TRUE;

  coords = g_new0 (ClayCoords, n_bound_segs + 4);

  seg      = 0;
  n_coords = 0;

  /* we offset all coordinates by 0.5 to align the brush with the path */

  coords[n_coords]   = default_coords;
  coords[n_coords].x = (gdouble) (stroke_segs[0].x1 + offset_x + 0.5);
  coords[n_coords].y = (gdouble) (stroke_segs[0].y1 + offset_y + 0.5);

  n_coords++;

  drawables = g_list_prepend (NULL, drawable);

  for (s = 0; s < n_stroke_segs; s++)
    {
      while (stroke_segs[seg].x1 != -1 ||
             stroke_segs[seg].x2 != -1 ||
             stroke_segs[seg].y1 != -1 ||
             stroke_segs[seg].y2 != -1)
        {
          coords[n_coords]   = default_coords;
          coords[n_coords].x = (gdouble) (stroke_segs[seg].x1 + offset_x + 0.5);
          coords[n_coords].y = (gdouble) (stroke_segs[seg].y1 + offset_y + 0.5);

          n_coords++;
          seg++;
        }

      /* Close the stroke points up */
      coords[n_coords] = coords[0];

      n_coords++;

      if (emulate_dynamics)
        clay_paint_core_stroke_emulate_dynamics (coords, n_coords);

      if (initialized ||
          clay_paint_core_start (core, drawables, paint_options, &coords[0],
                                 error))
        {
          gint i;

          initialized = TRUE;

          core->cur_coords  = coords[0];
          core->last_coords = coords[0];

          clay_paint_core_paint (core, drawables, paint_options,
                                 CLAY_PAINT_STATE_INIT, 0);

          clay_paint_core_paint (core, drawables, paint_options,
                                 CLAY_PAINT_STATE_MOTION, 0);

          for (i = 1; i < n_coords; i++)
            {
              clay_paint_core_interpolate (core, drawables, paint_options,
                                           &coords[i], 0);
            }

          clay_paint_core_paint (core, drawables, paint_options,
                                 CLAY_PAINT_STATE_FINISH, 0);
        }
      else
        {
          break;
        }

      n_coords = 0;
      seg++;

      coords[n_coords]   = default_coords;
      coords[n_coords].x = (gdouble) (stroke_segs[seg].x1 + offset_x + 0.5);
      coords[n_coords].y = (gdouble) (stroke_segs[seg].y1 + offset_y + 0.5);

      n_coords++;
    }

  if (initialized)
    {
      clay_paint_core_finish (core, drawables, push_undo);

      clay_paint_core_cleanup (core);
    }

  g_list_free (drawables);
  g_free (coords);
  g_free (stroke_segs);

  return initialized;
}

gboolean
clay_paint_core_stroke_path (ClayPaintCore     *core,
                             ClayDrawable      *drawable,
                             ClayPaintOptions  *paint_options,
                             gboolean           emulate_dynamics,
                             ClayPath          *path,
                             gboolean           push_undo,
                             GError           **error)
{
  GList    *drawables;
  GList    *stroke;
  gboolean  initialized = FALSE;
  gboolean  due_to_lack_of_points = FALSE;
  gint      off_x, off_y;

  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), FALSE);
  g_return_val_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (CLAY_IS_PATH (path), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  clay_item_get_offset (CLAY_ITEM (path),  &off_x, &off_y);

  drawables = g_list_prepend (NULL, drawable);

  for (stroke = path->strokes->head;
       stroke;
       stroke = stroke->next)
    {
      GArray   *coords;
      gboolean  closed;

      coords = clay_stroke_interpolate (CLAY_STROKE (stroke->data),
                                        1.0, &closed);

      if (coords && coords->len)
        {
          gint i;

          for (i = 0; i < coords->len; i++)
            {
              g_array_index (coords, ClayCoords, i).x += off_x;
              g_array_index (coords, ClayCoords, i).y += off_y;
            }

          if (emulate_dynamics)
            clay_paint_core_stroke_emulate_dynamics ((ClayCoords *) coords->data,
                                                     coords->len);

          if (initialized ||
              clay_paint_core_start (core, drawables, paint_options,
                                     &g_array_index (coords, ClayCoords, 0),
                                     error))
            {
              initialized = TRUE;

              core->cur_coords  = g_array_index (coords, ClayCoords, 0);
              core->last_coords = g_array_index (coords, ClayCoords, 0);

              clay_paint_core_paint (core, drawables, paint_options,
                                     CLAY_PAINT_STATE_INIT, 0);

              clay_paint_core_paint (core, drawables, paint_options,
                                     CLAY_PAINT_STATE_MOTION, 0);

              for (i = 1; i < coords->len; i++)
                {
                  clay_paint_core_interpolate (core, drawables, paint_options,
                                               &g_array_index (coords, ClayCoords, i),
                                               0);
                }

              clay_paint_core_paint (core, drawables, paint_options,
                                     CLAY_PAINT_STATE_FINISH, 0);
            }
          else
            {
              if (coords)
                g_array_free (coords, TRUE);

              break;
            }
        }
      else
        {
          due_to_lack_of_points = TRUE;
        }

      if (coords)
        g_array_free (coords, TRUE);
    }

  if (initialized)
    {
      clay_paint_core_finish (core, drawables, push_undo);

      clay_paint_core_cleanup (core);
    }

  if (! initialized && due_to_lack_of_points && *error == NULL)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Not enough points to stroke"));
    }

  g_list_free (drawables);

  return initialized;
}

static void
clay_paint_core_stroke_emulate_dynamics (ClayCoords *coords,
                                         gint        length)
{
  const gint ramp_length = length / 3;

  /* Calculate and create pressure ramp parameters */
  if (ramp_length > 0)
    {
      gdouble slope = 1.0 / (gdouble) (ramp_length);
      gint    i;

      /* Calculate pressure start ramp */
      for (i = 0; i < ramp_length; i++)
        {
          coords[i].pressure =  i * slope;
        }

      /* Calculate pressure end ramp */
      for (i = length - ramp_length; i < length; i++)
        {
          coords[i].pressure = 1.0 - (i - (length - ramp_length)) * slope;
        }
    }

  /* Calculate and create velocity ramp parameters */
  if (length > 0)
    {
      gdouble slope = 1.0 / length;
      gint    i;

      /* Calculate velocity end ramp */
      for (i = 0; i < length; i++)
        {
          coords[i].velocity = i * slope;
        }
    }

  if (length > 1)
    {
      gint i;
      /* Fill in direction */
      for (i = 1; i < length; i++)
        {
          coords[i].direction = clay_coords_direction (&coords[i-1], &coords[i]);
        }

      coords[0].direction = coords[1].direction;
    }
}
