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

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "paint-types.h"

#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/claydynamics.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/claypattern.h"
#include "core/claypickable.h"
#include "core/claysymmetry.h"

#include "clayclone.h"
#include "claycloneoptions.h"

#include "clay-intl.h"


static gboolean   clay_clone_start      (ClayPaintCore     *paint_core,
                                         GList             *drawables,
                                         ClayPaintOptions  *paint_options,
                                         const ClayCoords  *coords,
                                         GError           **error);

static void       clay_clone_motion     (ClaySourceCore    *source_core,
                                         ClayDrawable      *drawable,
                                         ClayPaintOptions  *paint_options,
                                         const ClayCoords  *coords,
                                         GeglNode          *op,
                                         gdouble            opacity,
                                         ClayPickable      *src_pickable,
                                         GeglBuffer        *src_buffer,
                                         GeglRectangle     *src_rect,
                                         gint               src_offset_x,
                                         gint               src_offset_y,
                                         GeglBuffer        *paint_buffer,
                                         gint               paint_buffer_x,
                                         gint               paint_buffer_y,
                                         gint               paint_area_offset_x,
                                         gint               paint_area_offset_y,
                                         gint               paint_area_width,
                                         gint               paint_area_height);

static gboolean   clay_clone_use_source (ClaySourceCore    *source_core,
                                         ClaySourceOptions *options);


G_DEFINE_TYPE (ClayClone, clay_clone, CLAY_TYPE_SOURCE_CORE)

#define parent_class clay_clone_parent_class


void
clay_clone_register (Clay                      *clay,
                     ClayPaintRegisterCallback  callback)
{
  (* callback) (clay,
                CLAY_TYPE_CLONE,
                CLAY_TYPE_CLONE_OPTIONS,
                "clay-clone",
                _("Clone"),
                "clay-tool-clone");
}

static void
clay_clone_class_init (ClayCloneClass *klass)
{
  ClayPaintCoreClass  *paint_core_class  = CLAY_PAINT_CORE_CLASS (klass);
  ClaySourceCoreClass *source_core_class = CLAY_SOURCE_CORE_CLASS (klass);

  paint_core_class->start       = clay_clone_start;

  source_core_class->use_source = clay_clone_use_source;
  source_core_class->motion     = clay_clone_motion;
}

static void
clay_clone_init (ClayClone *clone)
{
}

static gboolean
clay_clone_start (ClayPaintCore     *paint_core,
                  GList             *drawables,
                  ClayPaintOptions  *paint_options,
                  const ClayCoords  *coords,
                  GError           **error)
{
  ClayCloneOptions *options = CLAY_CLONE_OPTIONS (paint_options);

  if (! CLAY_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawables,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (options->clone_type == CLAY_CLONE_PATTERN)
    {
      if (! clay_context_get_pattern (CLAY_CONTEXT (options)))
        {
          g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                               _("No patterns available for use with this tool."));
          return FALSE;
        }
    }

  return TRUE;
}

static void
clay_clone_motion (ClaySourceCore   *source_core,
                   ClayDrawable     *drawable,
                   ClayPaintOptions *paint_options,
                   const ClayCoords *coords,
                   GeglNode         *op,
                   gdouble           opacity,
                   ClayPickable     *src_pickable,
                   GeglBuffer       *src_buffer,
                   GeglRectangle    *src_rect,
                   gint              src_offset_x,
                   gint              src_offset_y,
                   GeglBuffer       *paint_buffer,
                   gint              paint_buffer_x,
                   gint              paint_buffer_y,
                   gint              paint_area_offset_x,
                   gint              paint_area_offset_y,
                   gint              paint_area_width,
                   gint              paint_area_height)
{
  ClayPaintCore     *paint_core     = CLAY_PAINT_CORE (source_core);
  ClayBrushCore     *brush_core     = CLAY_BRUSH_CORE (source_core);
  ClayCloneOptions  *options        = CLAY_CLONE_OPTIONS (paint_options);
  ClaySourceOptions *source_options = CLAY_SOURCE_OPTIONS (paint_options);
  ClayContext       *context        = CLAY_CONTEXT (paint_options);
  ClayDynamics      *dynamics       = brush_core->dynamics;
  ClayImage         *image          = clay_item_get_image (CLAY_ITEM (drawable));
  gdouble            fade_point;
  gdouble            force;

  if (clay_source_core_use_source (source_core, source_options))
    {
      if (! op)
        {
          clay_gegl_buffer_copy (src_buffer,
                                 GEGL_RECTANGLE (src_rect->x,
                                                 src_rect->y,
                                                 paint_area_width,
                                                 paint_area_height),
                                 GEGL_ABYSS_NONE,
                                 paint_buffer,
                                 GEGL_RECTANGLE (paint_area_offset_x,
                                                 paint_area_offset_y,
                                                 0, 0));
        }
      else
        {
          clay_gegl_apply_operation (src_buffer, NULL, NULL, op,
                                     paint_buffer,
                                     GEGL_RECTANGLE (paint_area_offset_x,
                                                     paint_area_offset_y,
                                                     paint_area_width,
                                                     paint_area_height),
                                     FALSE);
        }
    }
  else if (options->clone_type == CLAY_CLONE_PATTERN)
    {
      ClayPattern *pattern    = clay_context_get_pattern (context);
      GeglBuffer  *src_buffer = clay_pattern_create_buffer (pattern);

      src_offset_x += gegl_buffer_get_width (src_buffer) / 2;
      src_offset_y += gegl_buffer_get_height (src_buffer) / 2;

      gegl_buffer_set_pattern (paint_buffer,
                               GEGL_RECTANGLE (paint_area_offset_x,
                                               paint_area_offset_y,
                                               paint_area_width,
                                               paint_area_height),
                               src_buffer,
                               - paint_buffer_x - src_offset_x,
                               - paint_buffer_y - src_offset_y);

      g_object_unref (src_buffer);
    }
  else
    {
      g_return_if_reached ();
    }

  fade_point = clay_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  if (clay_dynamics_is_output_enabled (dynamics, CLAY_DYNAMICS_OUTPUT_FORCE))
    force = clay_dynamics_get_linear_value (dynamics,
                                            CLAY_DYNAMICS_OUTPUT_FORCE,
                                            coords,
                                            paint_options,
                                            fade_point);
  else
    force = paint_options->brush_force;

  clay_brush_core_paste_canvas (CLAY_BRUSH_CORE (paint_core), drawable,
                                coords,
                                MIN (opacity, CLAY_OPACITY_OPAQUE),
                                clay_context_get_opacity (context),
                                clay_context_get_paint_mode (context),
                                clay_paint_options_get_brush_mode (paint_options),
                                force,

                                /* In fixed mode, paint incremental so the
                                 * individual brushes are properly applied
                                 * on top of each other.
                                 * Otherwise the stuff we paint is seamless
                                 * and we don't need intermediate masking.
                                 */
                                source_options->align_mode ==
                                CLAY_SOURCE_ALIGN_FIXED ?
                                CLAY_PAINT_INCREMENTAL : CLAY_PAINT_CONSTANT);
}

static gboolean
clay_clone_use_source (ClaySourceCore    *source_core,
                       ClaySourceOptions *options)
{
  return CLAY_CLONE_OPTIONS (options)->clone_type == CLAY_CLONE_IMAGE;
}
