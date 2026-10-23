/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "core/claytooloptions.h"


#define CLAY_PAINT_OPTIONS_CONTEXT_MASK CLAY_CONTEXT_PROP_MASK_FOREGROUND | \
                                        CLAY_CONTEXT_PROP_MASK_BACKGROUND | \
                                        CLAY_CONTEXT_PROP_MASK_OPACITY    | \
                                        CLAY_CONTEXT_PROP_MASK_PAINT_MODE | \
                                        CLAY_CONTEXT_PROP_MASK_BRUSH      | \
                                        CLAY_CONTEXT_PROP_MASK_DYNAMICS   | \
                                        CLAY_CONTEXT_PROP_MASK_PALETTE


typedef struct _ClayJitterOptions        ClayJitterOptions;
typedef struct _ClayFadeOptions          ClayFadeOptions;
typedef struct _ClayGradientPaintOptions ClayGradientPaintOptions;
typedef struct _ClaySmoothingOptions     ClaySmoothingOptions;

struct _ClayJitterOptions
{
  gboolean  use_jitter;
  gdouble   jitter_amount;
};

struct _ClayFadeOptions
{
  gboolean        fade_reverse;
  gdouble         fade_length;
  ClayUnit       *fade_unit;
  ClayRepeatMode  fade_repeat;
};

struct _ClayGradientPaintOptions
{
  gboolean                    gradient_reverse;
  ClayGradientBlendColorSpace gradient_blend_color_space;
  ClayRepeatMode              gradient_repeat; /* only used by gradient tool */
};

struct _ClaySmoothingOptions
{
  gboolean use_smoothing;
  gint     smoothing_quality;
  gdouble  smoothing_factor;
};


#define CLAY_TYPE_PAINT_OPTIONS            (clay_paint_options_get_type ())
#define CLAY_PAINT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAINT_OPTIONS, ClayPaintOptions))
#define CLAY_PAINT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAINT_OPTIONS, ClayPaintOptionsClass))
#define CLAY_IS_PAINT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAINT_OPTIONS))
#define CLAY_IS_PAINT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAINT_OPTIONS))
#define CLAY_PAINT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAINT_OPTIONS, ClayPaintOptionsClass))


typedef struct _ClayPaintOptionsClass ClayPaintOptionsClass;

struct _ClayPaintOptions
{
  ClayToolOptions           parent_instance;

  ClayPaintInfo            *paint_info;

  gboolean                  use_applicator;

  ClayBrush                *brush; /* weak-refed storage for the GUI */

  gdouble                   brush_size;
  gdouble                   brush_angle;
  gdouble                   brush_aspect_ratio;
  gdouble                   brush_spacing;
  gdouble                   brush_hardness;
  gdouble                   brush_force;

  gboolean                  brush_link_size;
  gboolean                  brush_link_aspect_ratio;
  gboolean                  brush_link_angle;
  gboolean                  brush_link_spacing;
  gboolean                  brush_link_hardness;

  gboolean                  brush_lock_to_view;

  ClayPaintApplicationMode  application_mode;
  ClayPaintApplicationMode  application_mode_save;

  gboolean                  hard;

  gboolean                  expand_use;
  gdouble                   expand_amount;
  ClayFillType              expand_fill_type;
  ClayAddMaskType           expand_mask_fill_type;

  ClayJitterOptions        *jitter_options;

  gboolean                  dynamics_enabled;
  ClayFadeOptions          *fade_options;
  ClayGradientPaintOptions *gradient_options;
  ClaySmoothingOptions     *smoothing_options;

  ClayViewType              brush_view_type;
  ClayViewSize              brush_view_size;
  ClayViewType              dynamics_view_type;
  ClayViewSize              dynamics_view_size;
  ClayViewType              pattern_view_type;
  ClayViewSize              pattern_view_size;
  ClayViewType              gradient_view_type;
  ClayViewSize              gradient_view_size;
};

struct _ClayPaintOptionsClass
{
  ClayToolOptionsClass  parent_class;
};


GType    clay_paint_options_get_type (void);

ClayPaintOptions *
         clay_paint_options_new                (ClayPaintInfo       *paint_info);

gdouble  clay_paint_options_get_fade           (ClayPaintOptions    *options,
                                                ClayImage           *image,
                                                gdouble              pixel_dist);

gdouble  clay_paint_options_get_jitter         (ClayPaintOptions    *options,
                                                ClayImage           *image);

gboolean clay_paint_options_get_gradient_color (ClayPaintOptions    *options,
                                                ClayImage           *image,
                                                gdouble              grad_point,
                                                gdouble              pixel_dist,
                                                GeglColor          **color);

ClayBrushApplicationMode
         clay_paint_options_get_brush_mode     (ClayPaintOptions    *options);

void     clay_paint_options_set_default_brush_size
                                               (ClayPaintOptions    *options,
                                                ClayBrush           *brush);
void     clay_paint_options_set_default_brush_angle
                                               (ClayPaintOptions    *options,
                                                ClayBrush           *brush);
void     clay_paint_options_set_default_brush_aspect_ratio
                                               (ClayPaintOptions    *options,
                                                ClayBrush           *brush);
void     clay_paint_options_set_default_brush_spacing
                                               (ClayPaintOptions    *options,
                                                ClayBrush           *brush);

void     clay_paint_options_set_default_brush_hardness
                                               (ClayPaintOptions    *options,
                                                ClayBrush           *brush);

gboolean clay_paint_options_are_dynamics_enabled (ClayPaintOptions *paint_options);
void     clay_paint_options_enable_dynamics      (ClayPaintOptions *paint_options,
                                                  gboolean          enable);

gboolean clay_paint_options_is_prop            (const gchar         *prop_name,
                                                ClayContextPropMask  prop_mask);
void     clay_paint_options_copy_props         (ClayPaintOptions    *src,
                                                ClayPaintOptions    *dest,
                                                ClayContextPropMask  prop_mask);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayPaintOptions, g_object_unref);
