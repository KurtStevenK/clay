/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-operations.c
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

#include "libclaybase/claybase.h"

#include "operations-types.h"

#include "core/clay.h"

#include "clay-operations.h"

#include "clayoperationborder.h"
#include "clayoperationbuffersourcevalidate.h"
#include "clayoperationcagecoefcalc.h"
#include "clayoperationcagetransform.h"
#include "clayoperationcomposecrop.h"
#include "clayoperationequalize.h"
#include "clayoperationfillsource.h"
#include "clayoperationflood.h"
#include "clayoperationgradient.h"
#include "clayoperationgrow.h"
#include "clayoperationhistogramsink.h"
#include "clayoperationmaskcomponents.h"
#include "clayoperationoffset.h"
#include "clayoperationprofiletransform.h"
#include "clayoperationscalarmultiply.h"
#include "clayoperationsemiflatten.h"
#include "clayoperationsetalpha.h"
#include "clayoperationshrink.h"
#include "clayoperationthresholdalpha.h"

#include "clayoperationbrightnesscontrast.h"
#include "clayoperationcolorbalance.h"
#include "clayoperationcolorize.h"
#include "clayoperationcurves.h"
#include "clayoperationdesaturate.h"
#include "clayoperationhuesaturation.h"
#include "clayoperationlevels.h"
#include "clayoperationposterize.h"
#include "clayoperationthreshold.h"

#include "clay-operation-config.h"
#include "claybrightnesscontrastconfig.h"
#include "claycolorbalanceconfig.h"
#include "claycurvesconfig.h"
#include "clayhuesaturationconfig.h"
#include "claylevelsconfig.h"

#include "layer-modes-legacy/clayoperationadditionlegacy.h"
#include "layer-modes-legacy/clayoperationburnlegacy.h"
#include "layer-modes-legacy/clayoperationdarkenonlylegacy.h"
#include "layer-modes-legacy/clayoperationdifferencelegacy.h"
#include "layer-modes-legacy/clayoperationdividelegacy.h"
#include "layer-modes-legacy/clayoperationdodgelegacy.h"
#include "layer-modes-legacy/clayoperationgrainextractlegacy.h"
#include "layer-modes-legacy/clayoperationgrainmergelegacy.h"
#include "layer-modes-legacy/clayoperationhardlightlegacy.h"
#include "layer-modes-legacy/clayoperationhslcolorlegacy.h"
#include "layer-modes-legacy/clayoperationhsvhuelegacy.h"
#include "layer-modes-legacy/clayoperationhsvsaturationlegacy.h"
#include "layer-modes-legacy/clayoperationhsvvaluelegacy.h"
#include "layer-modes-legacy/clayoperationlightenonlylegacy.h"
#include "layer-modes-legacy/clayoperationmultiplylegacy.h"
#include "layer-modes-legacy/clayoperationscreenlegacy.h"
#include "layer-modes-legacy/clayoperationsoftlightlegacy.h"
#include "layer-modes-legacy/clayoperationsubtractlegacy.h"

#include "layer-modes/clay-layer-modes.h"
#include "layer-modes/clayoperationantierase.h"
#include "layer-modes/clayoperationbehind.h"
#include "layer-modes/clayoperationdissolve.h"
#include "layer-modes/clayoperationerase.h"
#include "layer-modes/clayoperationmerge.h"
#include "layer-modes/clayoperationnormal.h"
#include "layer-modes/clayoperationpassthrough.h"
#include "layer-modes/clayoperationoverwrite.h"
#include "layer-modes/clayoperationreplace.h"
#include "layer-modes/clayoperationsplit.h"


static void
set_compat_file (GType        type,
                 const gchar *basename)
{
  GFile *file  = clay_directory_file ("tool-options", basename, NULL);
  GQuark quark = g_quark_from_static_string ("compat-file");

  g_type_set_qdata (type, quark, file);
}

static void
set_settings_folder (GType        type,
                     const gchar *basename)
{
  GFile *file  = clay_directory_file (basename, NULL);
  GQuark quark = g_quark_from_static_string ("settings-folder");

  g_type_set_qdata (type, quark, file);
}

void
clay_operations_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_layer_modes_init ();

  g_type_class_ref (CLAY_TYPE_OPERATION_BORDER);
  g_type_class_ref (CLAY_TYPE_OPERATION_BUFFER_SOURCE_VALIDATE);
  g_type_class_ref (CLAY_TYPE_OPERATION_CAGE_COEF_CALC);
  g_type_class_ref (CLAY_TYPE_OPERATION_CAGE_TRANSFORM);
  g_type_class_ref (CLAY_TYPE_OPERATION_COMPOSE_CROP);
  g_type_class_ref (CLAY_TYPE_OPERATION_EQUALIZE);
  g_type_class_ref (CLAY_TYPE_OPERATION_FILL_SOURCE);
  g_type_class_ref (CLAY_TYPE_OPERATION_FLOOD);
  g_type_class_ref (CLAY_TYPE_OPERATION_GRADIENT);
  g_type_class_ref (CLAY_TYPE_OPERATION_GROW);
  g_type_class_ref (CLAY_TYPE_OPERATION_HISTOGRAM_SINK);
  g_type_class_ref (CLAY_TYPE_OPERATION_MASK_COMPONENTS);
  g_type_class_ref (CLAY_TYPE_OPERATION_OFFSET);
  g_type_class_ref (CLAY_TYPE_OPERATION_PROFILE_TRANSFORM);
  g_type_class_ref (CLAY_TYPE_OPERATION_SCALAR_MULTIPLY);
  g_type_class_ref (CLAY_TYPE_OPERATION_SEMI_FLATTEN);
  g_type_class_ref (CLAY_TYPE_OPERATION_SET_ALPHA);
  g_type_class_ref (CLAY_TYPE_OPERATION_SHRINK);
  g_type_class_ref (CLAY_TYPE_OPERATION_THRESHOLD_ALPHA);

  g_type_class_ref (CLAY_TYPE_OPERATION_BRIGHTNESS_CONTRAST);
  g_type_class_ref (CLAY_TYPE_OPERATION_COLOR_BALANCE);
  g_type_class_ref (CLAY_TYPE_OPERATION_COLORIZE);
  g_type_class_ref (CLAY_TYPE_OPERATION_CURVES);
  g_type_class_ref (CLAY_TYPE_OPERATION_DESATURATE);
  g_type_class_ref (CLAY_TYPE_OPERATION_HUE_SATURATION);
  g_type_class_ref (CLAY_TYPE_OPERATION_LEVELS);
  g_type_class_ref (CLAY_TYPE_OPERATION_POSTERIZE);
  g_type_class_ref (CLAY_TYPE_OPERATION_THRESHOLD);

  g_type_class_ref (CLAY_TYPE_OPERATION_NORMAL);
  g_type_class_ref (CLAY_TYPE_OPERATION_DISSOLVE);
  g_type_class_ref (CLAY_TYPE_OPERATION_BEHIND);
  g_type_class_ref (CLAY_TYPE_OPERATION_MULTIPLY_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_SCREEN_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_DIFFERENCE_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_ADDITION_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_SUBTRACT_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_DARKEN_ONLY_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_LIGHTEN_ONLY_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_HSV_HUE_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_HSV_SATURATION_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_HSL_COLOR_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_HSV_VALUE_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_DIVIDE_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_DODGE_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_BURN_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_HARDLIGHT_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_SOFTLIGHT_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_GRAIN_EXTRACT_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_GRAIN_MERGE_LEGACY);
  g_type_class_ref (CLAY_TYPE_OPERATION_ERASE);
  g_type_class_ref (CLAY_TYPE_OPERATION_MERGE);
  g_type_class_ref (CLAY_TYPE_OPERATION_SPLIT);
  g_type_class_ref (CLAY_TYPE_OPERATION_PASS_THROUGH);
  g_type_class_ref (CLAY_TYPE_OPERATION_OVERWRITE);
  g_type_class_ref (CLAY_TYPE_OPERATION_REPLACE);
  g_type_class_ref (CLAY_TYPE_OPERATION_ANTI_ERASE);

  clay_operation_config_init_start (clay);

  clay_operation_config_register (clay,
                                  "clay:brightness-contrast",
                                  CLAY_TYPE_BRIGHTNESS_CONTRAST_CONFIG);
  set_compat_file (CLAY_TYPE_BRIGHTNESS_CONTRAST_CONFIG,
                   "clay-brightness-contrast-tool.settings");
  set_settings_folder (CLAY_TYPE_BRIGHTNESS_CONTRAST_CONFIG,
                       "brightness-contrast");

  clay_operation_config_register (clay,
                                  "clay:color-balance",
                                  CLAY_TYPE_COLOR_BALANCE_CONFIG);
  set_compat_file (CLAY_TYPE_COLOR_BALANCE_CONFIG,
                   "clay-color-balance-tool.settings");
  set_settings_folder (CLAY_TYPE_COLOR_BALANCE_CONFIG,
                       "color-balance");

  clay_operation_config_register (clay,
                                  "clay:curves",
                                  CLAY_TYPE_CURVES_CONFIG);
  set_compat_file (CLAY_TYPE_CURVES_CONFIG,
                   "clay-curves-tool.settings");
  set_settings_folder (CLAY_TYPE_CURVES_CONFIG,
                       "curves");

  clay_operation_config_register (clay,
                                  "clay:hue-saturation",
                                  CLAY_TYPE_HUE_SATURATION_CONFIG);
  set_compat_file (CLAY_TYPE_HUE_SATURATION_CONFIG,
                   "clay-hue-saturation-tool.settings");
  set_settings_folder (CLAY_TYPE_HUE_SATURATION_CONFIG,
                       "hue-saturation");

  clay_operation_config_register (clay,
                                  "clay:levels",
                                  CLAY_TYPE_LEVELS_CONFIG);
  set_compat_file (CLAY_TYPE_LEVELS_CONFIG,
                   "clay-levels-tool.settings");
  set_settings_folder (CLAY_TYPE_LEVELS_CONFIG,
                       "levels");

  clay_operation_config_init_end (clay);
}

void
clay_operations_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_layer_modes_exit ();
  clay_operation_config_exit (clay);
}
