/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claystrokeoptions.h
 * Copyright (C) 2003 Simon Budig
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

#include "clayfilloptions.h"


#define CLAY_TYPE_STROKE_OPTIONS (clay_stroke_options_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayStrokeOptions,
                          clay_stroke_options,
                          CLAY, STROKE_OPTIONS,
                          ClayFillOptions)


struct _ClayStrokeOptionsClass
{
  ClayFillOptionsClass  parent_class;

  void (* dash_info_changed) (ClayStrokeOptions *stroke_options,
                              ClayDashPreset     preset);
};


ClayStrokeOptions * clay_stroke_options_new                  (Clay              *clay,
                                                              ClayContext       *context,
                                                              gboolean           use_context_color);

ClayStrokeMethod    clay_stroke_options_get_method           (ClayStrokeOptions *options);

gdouble             clay_stroke_options_get_width            (ClayStrokeOptions *options);
ClayUnit          * clay_stroke_options_get_unit             (ClayStrokeOptions *options);
ClayCapStyle        clay_stroke_options_get_cap_style        (ClayStrokeOptions *options);
ClayJoinStyle       clay_stroke_options_get_join_style       (ClayStrokeOptions *options);
gdouble             clay_stroke_options_get_miter_limit      (ClayStrokeOptions *options);
gdouble             clay_stroke_options_get_dash_offset      (ClayStrokeOptions *options);
GArray            * clay_stroke_options_get_dash_info        (ClayStrokeOptions *options);

ClayPaintOptions  * clay_stroke_options_get_paint_options    (ClayStrokeOptions *options);
gboolean            clay_stroke_options_get_emulate_dynamics (ClayStrokeOptions *options);

void                clay_stroke_options_take_dash_pattern    (ClayStrokeOptions *options,
                                                              ClayDashPreset     preset,
                                                              GArray            *pattern);

void                clay_stroke_options_prepare              (ClayStrokeOptions *options,
                                                              ClayContext       *context,
                                                              ClayPaintOptions  *paint_options);
void                clay_stroke_options_finish               (ClayStrokeOptions *options);
