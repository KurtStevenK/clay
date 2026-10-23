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

#pragma once

#include "claypaintcore.h"


#define BRUSH_CORE_SUBSAMPLE        4
#define BRUSH_CORE_SOLID_SUBSAMPLE  2
#define BRUSH_CORE_JITTER_LUTSIZE   360


#define CLAY_TYPE_BRUSH_CORE            (clay_brush_core_get_type ())
#define CLAY_BRUSH_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_CORE, ClayBrushCore))
#define CLAY_BRUSH_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_CORE, ClayBrushCoreClass))
#define CLAY_IS_BRUSH_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_CORE))
#define CLAY_IS_BRUSH_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_CORE))
#define CLAY_BRUSH_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_CORE, ClayBrushCoreClass))


typedef struct _ClayBrushCoreClass ClayBrushCoreClass;

struct _ClayBrushCore
{
  ClayPaintCore      parent_instance;

  ClayBrush         *main_brush;
  ClayBrush         *brush;
  ClayDynamics      *dynamics;
  gdouble            spacing;
  gdouble            scale;
  gdouble            aspect_ratio;
  gdouble            angle;
  gboolean           reflect;
  gdouble            hardness;

  gdouble            symmetry_angle;
  gboolean           symmetry_reflect;

  /*  brush buffers  */
  ClayTempBuf       *pressure_brush;

  ClayTempBuf       *solid_brushes[BRUSH_CORE_SOLID_SUBSAMPLE][BRUSH_CORE_SOLID_SUBSAMPLE];
  const ClayTempBuf *last_solid_brush_mask;
  gboolean           solid_cache_invalid;

  const ClayTempBuf *transform_brush;
  const ClayTempBuf *transform_pixmap;

  ClayTempBuf       *subsample_brushes[BRUSH_CORE_SUBSAMPLE + 1][BRUSH_CORE_SUBSAMPLE + 1];
  const ClayTempBuf *last_subsample_brush_mask;
  gboolean           subsample_cache_invalid;

  gdouble            jitter;
  gdouble            jitter_lut_x[BRUSH_CORE_JITTER_LUTSIZE];
  gdouble            jitter_lut_y[BRUSH_CORE_JITTER_LUTSIZE];

  GRand             *rand;
};

struct _ClayBrushCoreClass
{
  ClayPaintCoreClass  parent_class;

  /*  Set for tools that don't mind if the brush changes while painting  */
  gboolean            handles_changing_brush;

  /*  Set for tools that don't mind if the brush scales while painting  */
  gboolean            handles_transforming_brush;

  /*  Set for tools that don't mind if the brush scales mid stroke  */
  gboolean            handles_dynamic_transforming_brush;

  void (* set_brush)    (ClayBrushCore *core,
                         ClayBrush     *brush);
  void (* set_dynamics) (ClayBrushCore *core,
                         ClayDynamics  *brush);
};


GType  clay_brush_core_get_type       (void);

void   clay_brush_core_set_brush      (ClayBrushCore            *core,
                                       ClayBrush                *brush);

void   clay_brush_core_set_dynamics   (ClayBrushCore            *core,
                                       ClayDynamics             *dynamics);

void   clay_brush_core_paste_canvas   (ClayBrushCore            *core,
                                       ClayDrawable             *drawable,
                                       const ClayCoords         *coords,
                                       gdouble                   brush_opacity,
                                       gdouble                   image_opacity,
                                       ClayLayerMode             paint_mode,
                                       ClayBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness,
                                       ClayPaintApplicationMode  mode);
void   clay_brush_core_replace_canvas (ClayBrushCore            *core,
                                       ClayDrawable             *drawable,
                                       const ClayCoords         *coords,
                                       gdouble                   brush_opacity,
                                       gdouble                   image_opacity,
                                       ClayBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness,
                                       ClayPaintApplicationMode  mode);

void   clay_brush_core_color_area_with_pixmap
                                      (ClayBrushCore            *core,
                                       ClayDrawable             *drawable,
                                       const ClayCoords         *coords,
                                       GeglBuffer               *area,
                                       gint                      area_x,
                                       gint                      area_y,
                                       gboolean                  apply_mask);

const ClayTempBuf * clay_brush_core_get_brush_mask
                                      (ClayBrushCore            *core,
                                       const ClayCoords         *coords,
                                       ClayBrushApplicationMode  brush_hardness,
                                       gdouble                   dynamic_hardness);
const ClayTempBuf * clay_brush_core_get_brush_pixmap
                                      (ClayBrushCore            *core);

void   clay_brush_core_eval_transform_dynamics
                                      (ClayBrushCore            *core,
                                       ClayImage                *image,
                                       ClayPaintOptions         *paint_options,
                                       const ClayCoords         *coords);
void   clay_brush_core_eval_transform_symmetry
                                      (ClayBrushCore            *core,
                                       ClaySymmetry             *symmetry,
                                       gint                      stroke);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayBrushCore, g_object_unref);
