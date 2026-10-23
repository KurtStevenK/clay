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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"

#include "paint-types.h"

#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/clay-palettes.h"
#include "core/claybrush.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/claypickable.h"
#include "core/claysymmetry.h"
#include "core/claytempbuf.h"

#include "claypaintbrush.h"
#include "claypaintoptions.h"

#include "clay-intl.h"


static void       clay_paintbrush_paint                        (ClayPaintCore             *paint_core,
                                                                GList                     *drawables,
                                                                ClayPaintOptions          *paint_options,
                                                                ClaySymmetry              *sym,
                                                                ClayPaintState             paint_state,
                                                                guint32                    time);

static gboolean   clay_paintbrush_real_get_color_history_color (ClayPaintbrush            *paintbrush,
                                                                ClayDrawable              *drawable,
                                                                ClayPaintOptions          *paint_options,
                                                                GeglColor                **color);
static void       clay_paintbrush_real_get_paint_params        (ClayPaintbrush            *paintbrush,
                                                                ClayDrawable              *drawable,
                                                                ClayPaintOptions          *paint_options,
                                                                ClaySymmetry              *sym,
                                                                gdouble                    grad_point,
                                                                ClayLayerMode             *paint_mode,
                                                                ClayPaintApplicationMode  *paint_appl_mode,
                                                                const ClayTempBuf        **paint_pixmap,
                                                                GeglColor                **paint_color);


G_DEFINE_TYPE (ClayPaintbrush, clay_paintbrush, CLAY_TYPE_BRUSH_CORE)


void
clay_paintbrush_register (Clay                      *clay,
                          ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_PAINTBRUSH,
                CLAY_TYPE_PAINT_OPTIONS,
                "clay-paintbrush",
                _("Paintbrush"),
                "clay-tool-paintbrush");
}

static void
clay_paintbrush_class_init (ClayPaintbrushClass *klass)
{
  ClayPaintCoreClass *paint_core_class = CLAY_PAINT_CORE_CLASS (klass);
  ClayBrushCoreClass *brush_core_class = CLAY_BRUSH_CORE_CLASS (klass);

  paint_core_class->paint                  = clay_paintbrush_paint;

  brush_core_class->handles_changing_brush = TRUE;

  klass->get_color_history_color           = clay_paintbrush_real_get_color_history_color;
  klass->get_paint_params                  = clay_paintbrush_real_get_paint_params;
}

static void
clay_paintbrush_init (ClayPaintbrush *paintbrush)
{
}

static void
clay_paintbrush_paint (ClayPaintCore    *paint_core,
                       GList            *drawables,
                       ClayPaintOptions *paint_options,
                       ClaySymmetry     *sym,
                       ClayPaintState    paint_state,
                       guint32           time)
{
  ClayPaintbrush *paintbrush = CLAY_PAINTBRUSH (paint_core);

  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case CLAY_PAINT_STATE_INIT:
      {
        GeglColor *color = NULL;

        for (GList *iter = drawables; iter; iter = iter->next)
          if (CLAY_PAINTBRUSH_GET_CLASS (paintbrush)->get_color_history_color &&
              CLAY_PAINTBRUSH_GET_CLASS (paintbrush)->get_color_history_color (paintbrush,
                                                                               iter->data,
                                                                               paint_options,
                                                                               &color))
            {
              ClayContext *context = CLAY_CONTEXT (paint_options);

              clay_palettes_add_color_history (context->clay, color);
            }
      }
      break;

    case CLAY_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        _clay_paintbrush_motion (paint_core, iter->data, paint_options,
                                 sym, CLAY_OPACITY_OPAQUE);
      break;

    case CLAY_PAINT_STATE_FINISH:
      g_clear_weak_pointer (&paintbrush->paint_buffer);
      g_clear_pointer (&paintbrush->paint_pixmap, clay_temp_buf_unref);
      break;
    }
}

static gboolean
clay_paintbrush_real_get_color_history_color (ClayPaintbrush    *paintbrush,
                                              ClayDrawable      *drawable,
                                              ClayPaintOptions  *paint_options,
                                              GeglColor        **color)
{
  ClayContext   *context    = CLAY_CONTEXT (paint_options);
  ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paintbrush);
  ClayDynamics  *dynamics   = clay_context_get_dynamics (context);

  /* We don't save gradient color history and pixmap brushes
   * have no color to save.
   */
  if (clay_dynamics_is_output_enabled (dynamics, CLAY_DYNAMICS_OUTPUT_COLOR) ||
      (brush_core->brush && clay_brush_get_pixmap (brush_core->brush)))
    {
      return FALSE;
    }

  *color = clay_context_get_foreground (context);

  return TRUE;
}

static void
clay_paintbrush_real_get_paint_params (ClayPaintbrush            *paintbrush,
                                       ClayDrawable              *drawable,
                                       ClayPaintOptions          *paint_options,
                                       ClaySymmetry              *sym,
                                       gdouble                    grad_point,
                                       ClayLayerMode             *paint_mode,
                                       ClayPaintApplicationMode  *paint_appl_mode,
                                       const ClayTempBuf        **paint_pixmap,
                                       GeglColor                **paint_color)
{
  ClayPaintCore *paint_core = CLAY_PAINT_CORE (paintbrush);
  ClayBrushCore *brush_core = CLAY_BRUSH_CORE (paintbrush);
  ClayContext   *context    = CLAY_CONTEXT (paint_options);
  ClayImage     *image      = clay_item_get_image (CLAY_ITEM (drawable));

  *paint_mode  = clay_context_get_paint_mode (context);
  *paint_color = NULL;

  if (clay_paint_options_are_dynamics_enabled (paint_options) &&
      clay_paint_options_get_gradient_color (paint_options, image,
                                             grad_point,
                                             paint_core->pixel_dist,
                                             paint_color))
    {
      /* optionally take the color from the current gradient */
      *paint_appl_mode = CLAY_PAINT_INCREMENTAL;
    }
  else if (brush_core->brush && clay_brush_get_pixmap (brush_core->brush))
    {
      /* otherwise check if the brush has a pixmap and use that to
       * color the area
       */
      *paint_pixmap = clay_brush_core_get_brush_pixmap (brush_core);

      *paint_appl_mode = CLAY_PAINT_INCREMENTAL;
    }
  else
    {
      /* otherwise fill the area with the foreground color */
      *paint_color = gegl_color_duplicate (clay_context_get_foreground (context));
    }
}

void
_clay_paintbrush_motion (ClayPaintCore    *paint_core,
                         ClayDrawable     *drawable,
                         ClayPaintOptions *paint_options,
                         ClaySymmetry     *sym,
                         gdouble           opacity)
{
  ClayBrushCore    *brush_core = CLAY_BRUSH_CORE (paint_core);
  ClayPaintbrush   *paintbrush = CLAY_PAINTBRUSH (paint_core);
  ClayContext      *context    = CLAY_CONTEXT (paint_options);
  ClayDynamics     *dynamics   = brush_core->dynamics;
  ClayImage        *image      = clay_item_get_image (CLAY_ITEM (drawable));
  gdouble           fade_point;
  gdouble           grad_point;
  gdouble           force;
  ClayCoords        coords;
  gint              n_strokes;
  gint              off_x, off_y;
  gint              i;

  fade_point = clay_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

  coords    = *(clay_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;
  clay_symmetry_set_origin (sym, drawable, &coords);
  paint_core->sym = sym;

  /* Some settings are based on the original stroke. */
  opacity *= clay_dynamics_get_linear_value (dynamics,
                                             CLAY_DYNAMICS_OUTPUT_OPACITY,
                                             &coords,
                                             paint_options,
                                             fade_point);
  if (opacity == 0.0)
    return;

  if (CLAY_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
    {
      clay_brush_core_eval_transform_dynamics (brush_core,
                                               image,
                                               paint_options,
                                               &coords);
    }

  grad_point = clay_dynamics_get_linear_value (dynamics,
                                               CLAY_DYNAMICS_OUTPUT_COLOR,
                                               &coords,
                                               paint_options,
                                               fade_point);

  n_strokes = clay_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      ClayLayerMode             paint_mode;
      ClayPaintApplicationMode  paint_appl_mode;
      GeglBuffer               *paint_buffer;
      gint                      paint_buffer_x;
      gint                      paint_buffer_y;
      const ClayTempBuf        *paint_pixmap = NULL;
      GeglColor                *paint_color;
      gint                      paint_width, paint_height;

      paint_appl_mode = paint_options->application_mode;

      if (CLAY_BRUSH_CORE_GET_CLASS (brush_core)->handles_transforming_brush)
        clay_brush_core_eval_transform_symmetry (brush_core, sym, i);

      CLAY_PAINTBRUSH_GET_CLASS (paintbrush)->get_paint_params (paintbrush,
                                                                drawable,
                                                                paint_options,
                                                                sym,
                                                                grad_point,
                                                                &paint_mode,
                                                                &paint_appl_mode,
                                                                &paint_pixmap,
                                                                &paint_color);

      coords    = *(clay_symmetry_get_coords (sym, i));

      paint_buffer = clay_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       paint_mode,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       &paint_width,
                                                       &paint_height);
      coords = *(clay_symmetry_get_coords (sym, i));

      if (! paint_buffer)
        continue;

      if (! paint_pixmap)
        {
          gdouble alpha;

          gegl_color_get_pixel (paint_color, babl_format ("A double"), &alpha);
          opacity *= alpha;
          clay_color_set_alpha (paint_color, CLAY_OPACITY_OPAQUE);
        }

      /* fill the paint buffer.  we can skip this step when reusing the
       * previous paint buffer, if the paint color/pixmap hasn't changed
       * (unless using an applicator, which currently modifies the paint buffer
       * in-place).
       */
      if (paint_core->applicators                  ||
          paint_buffer != paintbrush->paint_buffer ||
          paint_pixmap != paintbrush->paint_pixmap ||
          (! paint_pixmap &&
           ! clay_color_is_perceptually_identical (paint_color,
                                                   paintbrush->paint_color)))
        {
          g_set_weak_pointer (&paintbrush->paint_buffer, paint_buffer);

          if (paint_pixmap != paintbrush->paint_pixmap)
            {
              g_clear_pointer (&paintbrush->paint_pixmap, clay_temp_buf_unref);

              if (paint_pixmap)
                paintbrush->paint_pixmap = clay_temp_buf_ref (paint_pixmap);
            }

          g_clear_object (&paintbrush->paint_color);
          paintbrush->paint_color = paint_color;

          if (paint_pixmap)
            clay_brush_core_color_area_with_pixmap (brush_core, drawable,
                                                    &coords,
                                                    paint_buffer,
                                                    paint_buffer_x,
                                                    paint_buffer_y,
                                                    FALSE);
          else
            gegl_buffer_set_color (paint_buffer, NULL, paint_color);
        }
      else
        {
          g_clear_object (&paint_color);
        }

      if (clay_dynamics_is_output_enabled (dynamics, CLAY_DYNAMICS_OUTPUT_FORCE))
        force = clay_dynamics_get_linear_value (dynamics,
                                                CLAY_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      /* finally, let the brush core paste the colored area on the canvas */
      clay_brush_core_paste_canvas (brush_core, drawable,
                                    &coords,
                                    MIN (opacity, CLAY_OPACITY_OPAQUE),
                                    clay_context_get_opacity (context),
                                    paint_mode,
                                    clay_paint_options_get_brush_mode (paint_options),
                                    force,
                                    paint_appl_mode);
    }
}
