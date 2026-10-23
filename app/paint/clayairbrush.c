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

#include "paint-types.h"

#include "core/clay.h"
#include "core/claybrush.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/claysymmetry.h"

#include "clayairbrush.h"
#include "clayairbrushoptions.h"

#include "clay-intl.h"


#define STAMP_MAX_FPS 60


enum
{
  STAMP,
  LAST_SIGNAL
};


static void       clay_airbrush_finalize (GObject          *object);

static void       clay_airbrush_paint    (ClayPaintCore    *paint_core,
                                          GList            *drawables,
                                          ClayPaintOptions *paint_options,
                                          ClaySymmetry     *sym,
                                          ClayPaintState    paint_state,
                                          guint32           time);
static void       clay_airbrush_motion   (ClayPaintCore    *paint_core,
                                          ClayDrawable     *drawable,
                                          ClayPaintOptions *paint_options,
                                          ClaySymmetry     *sym);

static gboolean   clay_airbrush_timeout  (gpointer          data);


G_DEFINE_TYPE (ClayAirbrush, clay_airbrush, CLAY_TYPE_PAINTBRUSH)

#define parent_class clay_airbrush_parent_class

static guint airbrush_signals[LAST_SIGNAL] = { 0 };


void
clay_airbrush_register (Clay                      *clay,
                        ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_AIRBRUSH,
                CLAY_TYPE_AIRBRUSH_OPTIONS,
                "clay-airbrush",
                _("Airbrush"),
                "clay-tool-airbrush");
}

static void
clay_airbrush_class_init (ClayAirbrushClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  ClayPaintCoreClass *paint_core_class = CLAY_PAINT_CORE_CLASS (klass);

  object_class->finalize  = clay_airbrush_finalize;

  paint_core_class->paint = clay_airbrush_paint;

  airbrush_signals[STAMP] =
    g_signal_new ("stamp",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayAirbrushClass, stamp),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
clay_airbrush_init (ClayAirbrush *airbrush)
{
}

static void
clay_airbrush_finalize (GObject *object)
{
  ClayAirbrush *airbrush = CLAY_AIRBRUSH (object);

  if (airbrush->timeout_id)
    {
      g_source_remove (airbrush->timeout_id);
      airbrush->timeout_id = 0;
    }

  g_clear_object (&airbrush->sym);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_airbrush_paint (ClayPaintCore    *paint_core,
                     GList            *drawables,
                     ClayPaintOptions *paint_options,
                     ClaySymmetry     *sym,
                     ClayPaintState    paint_state,
                     guint32           time)
{
  ClayAirbrush        *airbrush = CLAY_AIRBRUSH (paint_core);
  ClayAirbrushOptions *options  = CLAY_AIRBRUSH_OPTIONS (paint_options);
  ClayDynamics        *dynamics = CLAY_BRUSH_CORE (paint_core)->dynamics;
  ClayCoords           coords;

  g_return_if_fail (g_list_length (drawables) == 1);

  if (airbrush->timeout_id)
    {
      g_source_remove (airbrush->timeout_id);
      airbrush->timeout_id = 0;
    }

  switch (paint_state)
    {
    case CLAY_PAINT_STATE_INIT:
      CLAY_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawables,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);
      break;

    case CLAY_PAINT_STATE_MOTION:
      coords = *(clay_symmetry_get_origin (sym));
      clay_airbrush_motion (paint_core, drawables->data, paint_options, sym);

      if ((options->rate != 0.0) && ! options->motion_only)
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (drawables->data));
          gdouble    fade_point;
          gdouble    dynamic_rate;
          gint       timeout;

          fade_point = clay_paint_options_get_fade (paint_options, image,
                                                    paint_core->pixel_dist);

          airbrush->drawable      = drawables->data;
          airbrush->paint_options = paint_options;

          clay_symmetry_set_origin (sym, drawables->data, &coords);
          if (airbrush->sym)
            g_object_unref (airbrush->sym);
          airbrush->sym = g_object_ref (sym);

          /* Base our timeout on the original stroke. */
          airbrush->coords = coords;

          dynamic_rate = clay_dynamics_get_linear_value (dynamics,
                                                         CLAY_DYNAMICS_OUTPUT_RATE,
                                                         &coords,
                                                         paint_options,
                                                         fade_point);

          timeout = (1000.0 / STAMP_MAX_FPS) /
                    ((options->rate / 100.0) * dynamic_rate);

          airbrush->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH,
                                                     timeout,
                                                     clay_airbrush_timeout,
                                                     airbrush, NULL);
        }
      break;

    case CLAY_PAINT_STATE_FINISH:
      CLAY_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawables,
                                                   paint_options,
                                                   sym,
                                                   paint_state, time);

      g_clear_object (&airbrush->sym);
      break;
    }
}

static void
clay_airbrush_motion (ClayPaintCore    *paint_core,
                      ClayDrawable     *drawable,
                      ClayPaintOptions *paint_options,
                      ClaySymmetry     *sym)

{
  ClayAirbrushOptions *options  = CLAY_AIRBRUSH_OPTIONS (paint_options);
  ClayDynamics        *dynamics = CLAY_BRUSH_CORE (paint_core)->dynamics;
  ClayImage           *image    = clay_item_get_image (CLAY_ITEM (drawable));
  gdouble              opacity;
  gdouble              fade_point;
  ClayCoords          *coords;

  fade_point = clay_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  coords = clay_symmetry_get_origin (sym);

  opacity = (options->flow / 100.0 *
             clay_dynamics_get_linear_value (dynamics,
                                             CLAY_DYNAMICS_OUTPUT_FLOW,
                                             coords,
                                             paint_options,
                                             fade_point));

  _clay_paintbrush_motion (paint_core, drawable, paint_options,
                           sym, opacity);
}

static gboolean
clay_airbrush_timeout (gpointer data)
{
  ClayAirbrush *airbrush = CLAY_AIRBRUSH (data);

  airbrush->timeout_id = 0;

  g_signal_emit (airbrush, airbrush_signals[STAMP], 0);

  return G_SOURCE_REMOVE;
}


/*  public functions  */


void
clay_airbrush_stamp (ClayAirbrush *airbrush)
{
  GList *drawables;

  g_return_if_fail (CLAY_IS_AIRBRUSH (airbrush));

  clay_symmetry_set_origin (airbrush->sym,
                            airbrush->drawable, &airbrush->coords);

  drawables = g_list_prepend (NULL, airbrush->drawable),
  clay_airbrush_paint (CLAY_PAINT_CORE (airbrush),
                       drawables,
                       airbrush->paint_options,
                       airbrush->sym,
                       CLAY_PAINT_STATE_MOTION, 0);
  g_list_free (drawables);

  clay_symmetry_clear_origin (airbrush->sym);
}
