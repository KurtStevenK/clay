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

#include "claydata.h"


#define CLAY_TYPE_TOOL_PRESET            (clay_tool_preset_get_type ())
#define CLAY_TOOL_PRESET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_PRESET, ClayToolPreset))
#define CLAY_TOOL_PRESET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_PRESET, ClayToolPresetClass))
#define CLAY_IS_TOOL_PRESET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_PRESET))
#define CLAY_IS_TOOL_PRESET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_PRESET))
#define CLAY_TOOL_PRESET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_PRESET, ClayToolPresetClass))


typedef struct _ClayToolPresetClass ClayToolPresetClass;

struct _ClayToolPreset
{
  ClayData         parent_instance;

  Clay            *clay;
  ClayToolOptions *tool_options;

  gboolean         use_fg_bg;
  gboolean         use_opacity_paint_mode;
  gboolean         use_brush;
  gboolean         use_dynamics;
  gboolean         use_mybrush;
  gboolean         use_gradient;
  gboolean         use_pattern;
  gboolean         use_palette;
  gboolean         use_font;
};

struct _ClayToolPresetClass
{
  ClayDataClass  parent_class;
};


GType                 clay_tool_preset_get_type      (void);

ClayData            * clay_tool_preset_new           (ClayContext    *context,
                                                      const gchar    *unused);

ClayContextPropMask   clay_tool_preset_get_prop_mask (ClayToolPreset *preset);
