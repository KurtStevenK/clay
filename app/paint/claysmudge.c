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

#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "core/clay-palettes.h"
#include "core/claybrush.h"
#include "core/claybrush-header.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/clayimage.h"
#include "core/claypickable.h"
#include "core/claysymmetry.h"
#include "core/claytempbuf.h"

#include "claysmudge.h"
#include "claysmudgeoptions.h"

#include "clay-intl.h"


static void       clay_smudge_finalize     (GObject          *object);

static void       clay_smudge_paint        (ClayPaintCore    *paint_core,
                                            GList            *drawables,
                                            ClayPaintOptions *paint_options,
                                            ClaySymmetry     *sym,
                                            ClayPaintState    paint_state,
                                            guint32           time);
static gboolean   clay_smudge_start        (ClayPaintCore    *paint_core,
                                            ClayDrawable     *drawable,
                                            ClayPaintOptions *paint_options,
                                            ClaySymmetry     *sym);
static void       clay_smudge_motion       (ClayPaintCore    *paint_core,
                                            ClayDrawable     *drawable,
                                            ClayPaintOptions *paint_options,
                                            ClaySymmetry     *sym);

static void       clay_smudge_accumulator_coords (ClayPaintCore    *paint_core,
                                                  const ClayCoords *coords,
                                                  gint              stroke,
                                                  gint             *x,
                                                  gint             *y);

static void       clay_smudge_accumulator_size   (ClayPaintOptions *paint_options,
                                                  const ClayCoords *coords,
                                                  gint             *accumulator_size);


G_DEFINE_TYPE (ClaySmudge, clay_smudge, CLAY_TYPE_BRUSH_CORE)

#define parent_class clay_smudge_parent_class


void
clay_smudge_register (Clay                      *clay,
                      ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_SMUDGE,
                CLAY_TYPE_SMUDGE_OPTIONS,
                "clay-smudge",
                _("Smudge"),
                "clay-tool-smudge");
}

static void
clay_smudge_class_init (ClaySmudgeClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  ClayPaintCoreClass *paint_core_class = CLAY_PAINT_CORE_CLASS (klass);
  ClayBrushCoreClass *brush_core_class = CLAY_BRUSH_CORE_CLASS (klass);

  object_class->finalize  = clay_smudge_finalize;

  paint_core_class->paint = clay_smudge_paint;

  brush_core_class->handles_changing_brush             = TRUE;
  brush_core_class->handles_transforming_brush         = TRUE;
  brush_core_class->handles_dynamic_transforming_brush = TRUE;
}

static void
clay_smudge_init (ClaySmudge *smudge)
{
}

static void
clay_smudge_finalize (GObject *object)
{
  ClaySmudge *smudge = CLAY_SMUDGE (object);

  if (smudge->accum_buffers)
    {
      GList *iter;

      for (iter = smudge->accum_buffers; iter; iter = g_list_next (iter))
        {
          if (iter->data)
            g_object_unref (iter->data);
        }

      g_list_free (smudge->accum_buffers);
      smudge->accum_buffers = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_smudge_paint (ClayPaintCore    *paint_core,
                   GList            *drawables,
                   ClayPaintOptions *paint_options,
                   ClaySymmetry     *sym,
                   ClayPaintState    paint_state,
                   guint32           time)
{
  ClaySmudge *smudge = CLAY_SMUDGE (paint_core);

  g_return_if_fail (g_list_length (drawables) == 1);

  switch (paint_state)
    {
    case CLAY_PAINT_STATE_INIT:
      {
        ClayContext       *context    = CLAY_CONTEXT (paint_options);
        ClaySmudgeOptions *options    = CLAY_SMUDGE_OPTIONS (paint_options);
        ClayBrushCore     *brush_core = CLAY_BRUSH_CORE (paint_core);
        ClayDynamics      *dynamics   = clay_context_get_dynamics (context);

        /* Don't add to color history when
         * 1. pure smudging (flow=0)
         * 2. color is from gradient or pixmap brushes
         */
        if (options->flow > 0.0 &&
            ! clay_dynamics_is_output_enabled (dynamics, CLAY_DYNAMICS_OUTPUT_COLOR) &&
            ! (brush_core->brush && clay_brush_get_pixmap (brush_core->brush)))
          clay_palettes_add_color_history (context->clay, clay_context_get_foreground (context));
      }
      break;

    case CLAY_PAINT_STATE_MOTION:
      /* initialization fails if the user starts outside the drawable */
      if (! smudge->initialized)
        smudge->initialized = clay_smudge_start (paint_core, drawables->data,
                                                 paint_options, sym);

      if (smudge->initialized)
        clay_smudge_motion (paint_core, drawables->data, paint_options, sym);
      break;

    case CLAY_PAINT_STATE_FINISH:
      if (smudge->accum_buffers)
        {
          GList *iter;

          for  (iter = smudge->accum_buffers; iter; iter = g_list_next (iter))
            {
              if (iter->data)
                g_object_unref (iter->data);
            }

          g_list_free (smudge->accum_buffers);
          smudge->accum_buffers = NULL;
        }

      smudge->initialized = FALSE;
      break;

    default:
      break;
    }
}

static gboolean
clay_smudge_start (ClayPaintCore    *paint_core,
                   ClayDrawable     *drawable,
                   ClayPaintOptions *paint_options,
                   ClaySymmetry     *sym)
{
  ClaySmudge        *smudge     = CLAY_SMUDGE (paint_core);
  ClayBrushCore     *brush_core = CLAY_BRUSH_CORE (paint_core);
  ClaySmudgeOptions *options    = CLAY_SMUDGE_OPTIONS (paint_options);
  ClayPickable      *dest_pickable;
  GeglBuffer        *pickable_buffer;
  GeglBuffer        *paint_buffer;
  ClayCoords         coords;
  gint               dest_pickable_off_x;
  gint               dest_pickable_off_y;
  gint               paint_buffer_x;
  gint               paint_buffer_y;
  gint               accum_size;
  gint               n_strokes;
  gint               i;
  gint               x, y;
  gint               off_x, off_y;

  coords = *(clay_symmetry_get_origin (sym));

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);
  coords.x -= off_x;
  coords.y -= off_y;

  clay_brush_core_eval_transform_dynamics (brush_core,
                                           clay_item_get_image (CLAY_ITEM (drawable)),
                                           paint_options,
                                           &coords);

  if (options->sample_merged)
    {
      dest_pickable = clay_paint_core_get_image_pickable (paint_core);

      clay_item_get_offset (CLAY_ITEM (drawable),
                            &dest_pickable_off_x,
                            &dest_pickable_off_y);
    }
  else
    {
      dest_pickable = CLAY_PICKABLE (drawable);

      dest_pickable_off_x = 0;
      dest_pickable_off_y = 0;
    }

  n_strokes = clay_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      GeglBuffer *accum_buffer;

      coords = *(clay_symmetry_get_coords (sym, i));
      coords.x -= off_x;
      coords.y -= off_y;

      clay_smudge_accumulator_size (paint_options, &coords, &accum_size);

      /*  Allocate the accumulation buffer */
      accum_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                      accum_size,
                                                      accum_size),
                                      babl_format ("RGBA float"));
      smudge->accum_buffers = g_list_prepend (smudge->accum_buffers,
                                              accum_buffer);

      /*  adjust the x and y coordinates to the upper left corner of the
       *  accumulator
       */
      paint_buffer = clay_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       CLAY_LAYER_MODE_NORMAL,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       NULL, NULL);
      if (! paint_buffer)
        continue;

      /*  Fetch the buffer _after_ clay_paint_core_get_paint_buffer() as it
       *  may have expanded the drawable when "Expand Layers" is enabled.
       */
      pickable_buffer = clay_pickable_get_buffer (dest_pickable);

      clay_smudge_accumulator_coords (paint_core, &coords, 0, &x, &y);

      /*  If clipped, prefill the smudge buffer with the color at the
       *  brush position.
       */
      if (x != paint_buffer_x ||
          y != paint_buffer_y ||
          accum_size != gegl_buffer_get_width  (paint_buffer) ||
          accum_size != gegl_buffer_get_height (paint_buffer))
        {
          gfloat     pixel[4];
          GeglColor *color;
          gint       pick_x;
          gint       pick_y;

          pick_x = CLAMP ((gint) coords.x + dest_pickable_off_x,
                          0,
                          gegl_buffer_get_width (pickable_buffer) - 1);
          pick_y = CLAMP ((gint) coords.y + dest_pickable_off_y,
                          0,
                          gegl_buffer_get_height (pickable_buffer) - 1);

          clay_pickable_get_pixel_at (dest_pickable,
                                      pick_x, pick_y,
                                      babl_format ("RGBA float"),
                                      pixel);

          color = gegl_color_new (NULL);
          gegl_color_set_pixel (color, babl_format ("RGBA float"), pixel);
          gegl_buffer_set_color (accum_buffer, NULL, color);
          g_object_unref (color);
        }

      /* copy the region under the original painthit. */
      clay_gegl_buffer_copy
        (pickable_buffer,
         GEGL_RECTANGLE (paint_buffer_x + dest_pickable_off_x,
                         paint_buffer_y + dest_pickable_off_y,
                         gegl_buffer_get_width  (paint_buffer),
                         gegl_buffer_get_height (paint_buffer)),
         GEGL_ABYSS_NONE,
         accum_buffer,
         GEGL_RECTANGLE (paint_buffer_x - x,
                         paint_buffer_y - y,
                         0, 0));
    }

  smudge->accum_buffers = g_list_reverse (smudge->accum_buffers);

  return TRUE;
}

static void
clay_smudge_motion (ClayPaintCore    *paint_core,
                    ClayDrawable     *drawable,
                    ClayPaintOptions *paint_options,
                    ClaySymmetry     *sym)
{
  ClaySmudge         *smudge     = CLAY_SMUDGE (paint_core);
  ClayBrushCore      *brush_core = CLAY_BRUSH_CORE (paint_core);
  ClaySmudgeOptions  *options    = CLAY_SMUDGE_OPTIONS (paint_options);
  ClayContext        *context    = CLAY_CONTEXT (paint_options);
  ClayDynamics       *dynamics   = CLAY_BRUSH_CORE (paint_core)->dynamics;
  ClayImage          *image      = clay_item_get_image (CLAY_ITEM (drawable));
  ClayPickable       *dest_pickable;
  GeglBuffer         *paint_buffer;
  gint                dest_pickable_off_x;
  gint                dest_pickable_off_y;
  gint                paint_buffer_x;
  gint                paint_buffer_y;
  gint                paint_buffer_width;
  gint                paint_buffer_height;
  /* brush dynamics */
  gdouble             fade_point;
  gdouble             opacity;
  gdouble             rate;
  gdouble             flow;
  gdouble             grad_point;
  /* brush color */
  GeglColor          *brush_color  = NULL; /* whether use single color or pixmap */
  /* accum buffer */
  gint                x, y;
  GeglBuffer         *accum_buffer;
  /* other variables */
  gdouble             force;
  ClayCoords          coords;
  gint                off_x, off_y;
  gint                paint_width, paint_height;
  gint                n_strokes;
  gint                i;

  if (options->sample_merged)
    {
      dest_pickable = clay_paint_core_get_image_pickable (paint_core);

      clay_item_get_offset (CLAY_ITEM (drawable),
                            &dest_pickable_off_x,
                            &dest_pickable_off_y);
    }
  else
    {
      dest_pickable = CLAY_PICKABLE (drawable);

      dest_pickable_off_x = 0;
      dest_pickable_off_y = 0;
    }

  fade_point = clay_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

  coords    = *(clay_symmetry_get_origin (sym));
  coords.x -= off_x;
  coords.y -= off_y;
  clay_symmetry_set_origin (sym, drawable, &coords);
  paint_core->sym = sym;

  opacity = clay_dynamics_get_linear_value (dynamics,
                                            CLAY_DYNAMICS_OUTPUT_OPACITY,
                                            &coords,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  clay_brush_core_eval_transform_dynamics (brush_core,
                                           image,
                                           paint_options,
                                           &coords);

  /* Get brush dynamic values other than opacity */
  rate = ((options->rate / 100.0) *
          clay_dynamics_get_linear_value (dynamics,
                                          CLAY_DYNAMICS_OUTPUT_RATE,
                                          &coords,
                                          paint_options,
                                          fade_point));

  flow = ((options->flow / 100.0) *
          clay_dynamics_get_linear_value (dynamics,
                                          CLAY_DYNAMICS_OUTPUT_FLOW,
                                          &coords,
                                          paint_options,
                                          fade_point));

  grad_point = clay_dynamics_get_linear_value (dynamics,
                                               CLAY_DYNAMICS_OUTPUT_COLOR,
                                               &coords,
                                               paint_options,
                                               fade_point);

  /* Get current gradient color, brush pixmap, or foreground color */
  if (clay_paint_options_get_gradient_color (paint_options, image,
                                             grad_point,
                                             paint_core->pixel_dist,
                                             &brush_color))
    {
      /* No more processing needed */
    }
  else if (brush_core->brush && clay_brush_get_pixmap (brush_core->brush))
    {
    }
  else
    {
      brush_color = g_object_ref (clay_context_get_foreground (context));
    }

  n_strokes = clay_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords    = *(clay_symmetry_get_coords (sym, i));

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

      paint_buffer_width  = gegl_buffer_get_width  (paint_buffer);
      paint_buffer_height = gegl_buffer_get_height (paint_buffer);

      /*  Get the unclipped accumulator coordinates  */
      clay_smudge_accumulator_coords (paint_core, &coords, i, &x, &y);

      accum_buffer = g_list_nth_data (smudge->accum_buffers, i);

      /* Old smudge tool:
       *  Smudge uses the buffer Accum.
       *  For each successive painthit Accum is built like this
       *    Accum =  rate*Accum  + (1-rate)*I.
       *  where I is the pixels under the current painthit.
       *  Then the paint area (paint_area) is built as
       *    (Accum,1) (if no alpha),
       */

      /* 2017/4/22: New smudge painting tool:
       * Accum=rate*Accum + (1-rate)*I
       * if brush_color_ptr!=NULL
       *   Paint=(1-flow)*Accum + flow*BrushColor
       * else, draw brush pixmap on the paint_buffer and
       *   Paint=(1-flow)*Accum + flow*Paint
       *
       * For non-pixmap brushes, calculate blending in
       * clay_gegl_smudge_with_paint() instead of calling
       * gegl_buffer_set_color() to reduce gegl's internal processing.
       */
      if (! brush_color && flow > 0.0)
        {
          clay_brush_core_color_area_with_pixmap (brush_core, drawable,
                                                  &coords,
                                                  paint_buffer,
                                                  paint_buffer_x,
                                                  paint_buffer_y,
                                                  TRUE);
        }

      if (accum_buffer)
        clay_gegl_smudge_with_paint (accum_buffer,
                                     GEGL_RECTANGLE (paint_buffer_x - x,
                                                     paint_buffer_y - y,
                                                     paint_buffer_width,
                                                     paint_buffer_height),
                                     clay_pickable_get_buffer (dest_pickable),
                                     GEGL_RECTANGLE (paint_buffer_x +
                                                     dest_pickable_off_x,
                                                     paint_buffer_y +
                                                     dest_pickable_off_y,
                                                     paint_buffer_width,
                                                     paint_buffer_height),
                                     brush_color,
                                     paint_buffer,
                                     options->no_erasing,
                                     flow,
                                     rate);

      if (clay_dynamics_is_output_enabled (dynamics, CLAY_DYNAMICS_OUTPUT_FORCE))
        force = clay_dynamics_get_linear_value (dynamics,
                                                CLAY_DYNAMICS_OUTPUT_FORCE,
                                                &coords,
                                                paint_options,
                                                fade_point);
      else
        force = paint_options->brush_force;

      clay_brush_core_replace_canvas (CLAY_BRUSH_CORE (paint_core), drawable,
                                      &coords,
                                      MIN (opacity, CLAY_OPACITY_OPAQUE),
                                      clay_context_get_opacity (context),
                                      clay_paint_options_get_brush_mode (paint_options),
                                      force,
                                      CLAY_PAINT_INCREMENTAL);
    }

  g_clear_object (&brush_color);
}

static void
clay_smudge_accumulator_coords (ClayPaintCore    *paint_core,
                                const ClayCoords *coords,
                                gint              stroke,
                                gint             *x,
                                gint             *y)
{
  ClaySmudge *smudge = CLAY_SMUDGE (paint_core);
  GeglBuffer *accum_buffer;

  *x = (gint) coords->x;
  *y = (gint) coords->y;

  if (smudge->accum_buffers)
    {
      accum_buffer = g_list_nth_data (smudge->accum_buffers, stroke);

      if (accum_buffer)
        {
          *x -= gegl_buffer_get_width  (accum_buffer) / 2;
          *y -= gegl_buffer_get_height (accum_buffer) / 2;
        }
    }
}

static void
clay_smudge_accumulator_size (ClayPaintOptions *paint_options,
                              const ClayCoords *coords,
                              gint             *accumulator_size)
{
  gdouble max_view_scale = 1.0;
  gdouble max_brush_size;

  if (paint_options->brush_lock_to_view)
    max_view_scale = MAX (coords->xscale, coords->yscale);

  max_brush_size = MIN (paint_options->brush_size / max_view_scale,
                        CLAY_BRUSH_MAX_SIZE);

  /* Note: the max brush mask size plus a border of 1 pixel and a
   * little headroom
   */
  *accumulator_size = ceil (sqrt (2 * SQR (max_brush_size + 1)) + 2);
}
