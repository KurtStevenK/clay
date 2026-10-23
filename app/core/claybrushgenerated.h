/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush_generated module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include "claybrush.h"


#define CLAY_TYPE_BRUSH_GENERATED            (clay_brush_generated_get_type ())
#define CLAY_BRUSH_GENERATED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_GENERATED, ClayBrushGenerated))
#define CLAY_BRUSH_GENERATED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_GENERATED, ClayBrushGeneratedClass))
#define CLAY_IS_BRUSH_GENERATED(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_GENERATED))
#define CLAY_IS_BRUSH_GENERATED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_GENERATED))
#define CLAY_BRUSH_GENERATED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_GENERATED, ClayBrushGeneratedClass))


/* When changing these values, also update it in pdb/groups/brush.pdb */
#define CLAY_BRUSH_GENERATED_MIN_RADIUS         0.1
#define CLAY_BRUSH_GENERATED_MAX_RADIUS      4000.0

typedef struct _ClayBrushGeneratedClass ClayBrushGeneratedClass;

struct _ClayBrushGenerated
{
  ClayBrush               parent_instance;

  ClayBrushGeneratedShape shape;
  gfloat                  radius;
  gint                    spikes;       /* 2 - 20     */
  gfloat                  hardness;     /* 0.0 - 1.0  */
  gfloat                  aspect_ratio; /* y/x        */
  gfloat                  angle;        /* in degrees */
};

struct _ClayBrushGeneratedClass
{
  ClayBrushClass  parent_class;
};


GType       clay_brush_generated_get_type     (void);

ClayData  * clay_brush_generated_new          (const gchar             *name,
                                               ClayBrushGeneratedShape  shape,
                                               gfloat                   radius,
                                               gint                     spikes,
                                               gfloat                   hardness,
                                               gfloat                   aspect_ratio,
                                               gfloat                   angle);

ClayBrushGeneratedShape
        clay_brush_generated_set_shape        (ClayBrushGenerated      *brush,
                                               ClayBrushGeneratedShape  shape);
gfloat  clay_brush_generated_set_radius       (ClayBrushGenerated      *brush,
                                               gfloat                   radius);
gint    clay_brush_generated_set_spikes       (ClayBrushGenerated      *brush,
                                               gint                     spikes);
gfloat  clay_brush_generated_set_hardness     (ClayBrushGenerated      *brush,
                                               gfloat                   hardness);
gfloat  clay_brush_generated_set_aspect_ratio (ClayBrushGenerated      *brush,
                                               gfloat                   ratio);
gfloat  clay_brush_generated_set_angle        (ClayBrushGenerated      *brush,
                                               gfloat                   angle);

ClayBrushGeneratedShape
        clay_brush_generated_get_shape        (ClayBrushGenerated      *brush);
gfloat  clay_brush_generated_get_radius       (ClayBrushGenerated      *brush);
gint    clay_brush_generated_get_spikes       (ClayBrushGenerated      *brush);
gfloat  clay_brush_generated_get_hardness     (ClayBrushGenerated      *brush);
gfloat  clay_brush_generated_get_aspect_ratio (ClayBrushGenerated      *brush);
gfloat  clay_brush_generated_get_angle        (ClayBrushGenerated      *brush);
