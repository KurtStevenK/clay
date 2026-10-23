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

#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/clay-palettes.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/clayimage.h"
#include "core/claypickable.h"
#include "core/claysymmetry.h"

#include "clayeraser.h"
#include "clayeraseroptions.h"

#include "clay-intl.h"


static gboolean   clay_eraser_get_color_history_color (ClayPaintbrush            *paintbrush,
                                                       ClayDrawable              *drawable,
                                                       ClayPaintOptions          *paint_options,
                                                       GeglColor                **color);
static void       clay_eraser_get_paint_params        (ClayPaintbrush            *paintbrush,
                                                       ClayDrawable              *drawable,
                                                       ClayPaintOptions          *paint_options,
                                                       ClaySymmetry              *sym,
                                                       gdouble                    grad_point,
                                                       ClayLayerMode             *paint_mode,
                                                       ClayPaintApplicationMode  *paint_appl_mode,
                                                       const ClayTempBuf        **paint_pixmap,
                                                       GeglColor                **paint_color);


G_DEFINE_TYPE (ClayEraser, clay_eraser, CLAY_TYPE_PAINTBRUSH)


void
clay_eraser_register (Clay                      *clay,
                      ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_ERASER,
                CLAY_TYPE_ERASER_OPTIONS,
                "clay-eraser",
                _("Eraser"),
                "clay-tool-eraser");
}

static void
clay_eraser_class_init (ClayEraserClass *klass)
{
  ClayPaintbrushClass *paintbrush_class = CLAY_PAINTBRUSH_CLASS (klass);

  paintbrush_class->get_color_history_color = clay_eraser_get_color_history_color;
  paintbrush_class->get_paint_params        = clay_eraser_get_paint_params;
}

static void
clay_eraser_init (ClayEraser *eraser)
{
}

static gboolean
clay_eraser_get_color_history_color (ClayPaintbrush    *paintbrush,
                                     ClayDrawable      *drawable,
                                     ClayPaintOptions  *paint_options,
                                     GeglColor        **color)
{
  /* Erasing on a drawable without alpha is equivalent to
   * drawing with background color. So let's save history.
   */
  if (! clay_drawable_has_alpha (drawable))
    {
      ClayContext *context = CLAY_CONTEXT (paint_options);

      *color = clay_context_get_background (context);

      return TRUE;
    }

  return FALSE;
}

static void
clay_eraser_get_paint_params (ClayPaintbrush            *paintbrush,
                              ClayDrawable              *drawable,
                              ClayPaintOptions          *paint_options,
                              ClaySymmetry              *sym,
                              gdouble                    grad_point,
                              ClayLayerMode             *paint_mode,
                              ClayPaintApplicationMode  *paint_appl_mode,
                              const ClayTempBuf        **paint_pixmap,
                              GeglColor                **paint_color)
{
  ClayEraserOptions *options = CLAY_ERASER_OPTIONS (paint_options);
  ClayContext       *context = CLAY_CONTEXT (paint_options);

  *paint_color = gegl_color_duplicate (clay_context_get_background (context));

  if (options->anti_erase)
    *paint_mode = CLAY_LAYER_MODE_ANTI_ERASE;
  else if (clay_drawable_has_alpha (drawable))
    *paint_mode = CLAY_LAYER_MODE_ERASE;
  else
    *paint_mode = CLAY_LAYER_MODE_NORMAL_LEGACY;
}
