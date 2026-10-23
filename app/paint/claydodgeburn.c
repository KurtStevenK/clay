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
#include "libclaymath/claymath.h"

#include "paint-types.h"

#include "gegl/clay-gegl-loops.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/clayimage.h"
#include "core/claysymmetry.h"

#include "claydodgeburn.h"
#include "claydodgeburnoptions.h"

#include "clay-intl.h"


static void   clay_dodge_burn_paint  (ClayPaintCore    *paint_core,
                                      GList            *drawables,
                                      ClayPaintOptions *paint_options,
                                      ClaySymmetry     *sym,
                                      ClayPaintState    paint_state,
                                      guint32           time);
static void   clay_dodge_burn_motion (ClayPaintCore    *paint_core,
                                      ClayDrawable     *drawable,
                                      ClayPaintOptions *paint_options,
                                      ClaySymmetry     *sym);


G_DEFINE_TYPE (ClayDodgeBurn, clay_dodge_burn, CLAY_TYPE_BRUSH_CORE)

#define parent_class clay_dodge_burn_parent_class


void
clay_dodge_burn_register (Clay                      *clay,
                          ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_DODGE_BURN,
                CLAY_TYPE_DODGE_BURN_OPTIONS,
                "clay-dodge-burn",
                _("Dodge/Burn"),
                "clay-tool-dodge");
}

static void
clay_dodge_burn_class_init (ClayDodgeBurnClass *klass)
{
  ClayPaintCoreClass *paint_core_class = CLAY_PAINT_CORE_CLASS (klass);
  ClayBrushCoreClass *brush_core_class = CLAY_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint = clay_dodge_burn_paint;

  brush_core_class->handles_changing_brush = TRUE;
}

static void
clay_dodge_burn_init (ClayDodgeBurn *dodgeburn)
{
}

static void
clay_dodge_burn_paint (ClayPaintCore    *paint_core,
                       GList            *drawables,
                       ClayPaintOptions *paint_options,
                       ClaySymmetry     *sym,
                       ClayPaintState    paint_state,
                       guint32           time)
{
  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case CLAY_PAINT_STATE_INIT:
      break;

    case CLAY_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        clay_dodge_burn_motion (paint_core, iter->data, paint_options, sym);
      break;

    case CLAY_PAINT_STATE_FINISH:
      break;
    }
}

static void
clay_dodge_burn_motion (ClayPaintCore    *paint_core,
                        ClayDrawable     *drawable,
                        ClayPaintOptions *paint_options,
                        ClaySymmetry     *sym)
{
  ClayBrushCore        *brush_core = CLAY_BRUSH_CORE (paint_core);
  ClayDodgeBurnOptions *options    = CLAY_DODGE_BURN_OPTIONS (paint_options);
  ClayContext          *context    = CLAY_CONTEXT (paint_options);
  ClayDynamics         *dynamics   = CLAY_BRUSH_CORE (paint_core)->dynamics;
  ClayImage            *image      = clay_item_get_image (CLAY_ITEM (drawable));
  GeglBuffer           *src_buffer;
  GeglBuffer           *paint_buffer;
  gint                  paint_buffer_x;
  gint                  paint_buffer_y;
  gdouble               fade_point;
  gdouble               opacity;
  gdouble               force;
  ClayCoords            coords;
  gint                  off_x, off_y;
  gint                  paint_width, paint_height;
  gint                  n_strokes;
  gint                  i;

  fade_point = clay_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);
  coords = *(clay_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;
  clay_symmetry_set_origin (sym, drawable, &coords);

  opacity = clay_dynamics_get_linear_value (dynamics,
                                            CLAY_DYNAMICS_OUTPUT_OPACITY,
                                            &coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  if (paint_options->application_mode == CLAY_PAINT_CONSTANT)
    src_buffer = clay_paint_core_get_orig_image (paint_core, drawable);
  else
    src_buffer = clay_drawable_get_buffer (drawable);

  clay_brush_core_eval_transform_dynamics (brush_core,
                                           image,
                                           paint_options,
                                           &coords);
  n_strokes = clay_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords = *(clay_symmetry_get_coords (sym, i));

      clay_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = clay_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       CLAY_LAYER_MODE_NORMAL,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      if (! paint_buffer)
        continue;

      /*  DodgeBurn the region  */
      clay_gegl_dodgeburn (src_buffer,
                           GEGL_RECTANGLE (paint_buffer_x,
                                           paint_buffer_y,
                                           gegl_buffer_get_width  (paint_buffer),
                                           gegl_buffer_get_height (paint_buffer)),
                           paint_buffer,
                           GEGL_RECTANGLE (0, 0, 0, 0),
                           options->exposure / 100.0,
                           options->type,
                           options->mode);

      if (clay_dynamics_is_output_enabled (dynamics, CLAY_DYNAMICS_OUTPUT_FORCE))
        force = clay_dynamics_get_linear_value (dynamics,
                                                CLAY_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      /* Replace the newly dodgedburned area (paint_area) to the image */
      clay_brush_core_replace_canvas (CLAY_BRUSH_CORE (paint_core), drawable,
                                      &coords,
                                      MIN (opacity, CLAY_OPACITY_OPAQUE),
                                      clay_context_get_opacity (context),
                                      clay_paint_options_get_brush_mode (paint_options),
                                      force,
                                      paint_options->application_mode);
    }
}
