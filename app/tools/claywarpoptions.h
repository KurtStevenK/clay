/* xyz.kainzmayer.ClayDesign
 *
 * claywarpoptions.h
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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


#define CLAY_TYPE_WARP_OPTIONS            (clay_warp_options_get_type ())
#define CLAY_WARP_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_WARP_OPTIONS, ClayWarpOptions))
#define CLAY_WARP_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_WARP_OPTIONS, ClayWarpOptionsClass))
#define CLAY_IS_WARP_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_WARP_OPTIONS))
#define CLAY_IS_WARP_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_WARP_OPTIONS))
#define CLAY_WARP_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_WARP_OPTIONS, ClayWarpOptionsClass))


typedef struct _ClayWarpOptions      ClayWarpOptions;
typedef struct _ClayWarpOptionsClass ClayWarpOptionsClass;

struct _ClayWarpOptions
{
  ClayToolOptions        parent_instance;

  ClayWarpBehavior       behavior;
  gdouble                effect_size;
  gdouble                effect_hardness;
  gdouble                effect_strength;
  gdouble                stroke_spacing;
  ClayInterpolationType  interpolation;
  GeglAbyssPolicy        abyss_policy;
  gboolean               high_quality_preview;
  gboolean               real_time_preview;

  gboolean               stroke_during_motion;
  gboolean               stroke_periodically;
  gdouble                stroke_periodically_rate;

  gint                   n_animation_frames;

  /*  options gui  */
  GtkWidget             *behavior_combo;
  GtkWidget             *stroke_frame;
  GtkWidget             *animate_button;
};

struct _ClayWarpOptionsClass
{
  ClayToolOptionsClass  parent_class;
};


GType       clay_warp_options_get_type (void);

GtkWidget * clay_warp_options_gui      (ClayToolOptions *tool_options);
