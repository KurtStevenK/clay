/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-layer-modes.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *                    Øyvind Kolås <pippin@gimp.org>
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

#include <glib-object.h>
#include <gegl.h>

#include "../operations-types.h"

#include "gegl/clay-babl.h"

#include "clayoperationlayermode.h"
#include "clayoperationlayermode-blend.h"

#include "clay-layer-modes.h"


typedef struct _ClayLayerModeInfo ClayLayerModeInfo;

struct _ClayLayerModeInfo
{
  ClayLayerMode             layer_mode;
  const gchar              *op_name;
  ClayLayerModeBlendFunc    blend_function;
  ClayLayerModeFlags        flags;
  ClayLayerModeContext      context;
  ClayLayerCompositeMode    paint_composite_mode;
  ClayLayerCompositeMode    composite_mode;
  ClayLayerColorSpace       composite_space;
  ClayLayerColorSpace       blend_space;
};


/*  static variables  */

static const ClayLayerModeInfo layer_mode_infos[] =
{
  { CLAY_LAYER_MODE_NORMAL_LEGACY,

    .op_name              = "clay:normal",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_DISSOLVE,

    .op_name              = "clay:dissolve",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION
  },

  { CLAY_LAYER_MODE_BEHIND_LEGACY,

    .op_name              = "clay:behind",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_PAINT |
                            CLAY_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_MULTIPLY_LEGACY,

    .op_name              = "clay:multiply-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_SCREEN_LEGACY,

    .op_name              = "clay:screen-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_OVERLAY_LEGACY,

    .op_name              = "clay:softlight-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_DIFFERENCE_LEGACY,

    .op_name              = "clay:difference-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_ADDITION_LEGACY,

    .op_name              = "clay:addition-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_SUBTRACT_LEGACY,

    .op_name              = "clay:subtract-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY,

    .op_name              = "clay:darken-only-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY,

    .op_name              = "clay:lighten-only-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSV_HUE_LEGACY,

    .op_name              = "clay:hsv-hue-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSV_SATURATION_LEGACY,

    .op_name              = "clay:hsv-saturation-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSL_COLOR_LEGACY,

    .op_name              = "clay:hsl-color-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSV_VALUE_LEGACY,

    .op_name              = "clay:hsv-value-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_DIVIDE_LEGACY,

    .op_name              = "clay:divide-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_DODGE_LEGACY,

    .op_name              = "clay:dodge-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_BURN_LEGACY,

    .op_name              = "clay:burn-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HARDLIGHT_LEGACY,

    .op_name              = "clay:hardlight-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_SOFTLIGHT_LEGACY,

    .op_name              = "clay:softlight-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY,

    .op_name              = "clay:grain-extract-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY,

    .op_name              = "clay:grain-merge-legacy",
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_COLOR_ERASE_LEGACY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_color_erase,
    .flags                = CLAY_LAYER_MODE_FLAG_LEGACY                    |
                            CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            CLAY_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = CLAY_LAYER_MODE_CONTEXT_PAINT |
                            CLAY_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_OVERLAY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_overlay,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_LCH_HUE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_lch_hue,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_LAB
  },

  { CLAY_LAYER_MODE_LCH_CHROMA,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_lch_chroma,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_LAB
  },

  { CLAY_LAYER_MODE_LCH_COLOR,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_lch_color,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_LAB
  },

  { CLAY_LAYER_MODE_LCH_LIGHTNESS,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_lch_lightness,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_LAB
  },

  { CLAY_LAYER_MODE_NORMAL,

    .op_name              = "clay:normal",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_BEHIND,

    .op_name              = "clay:behind",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_PAINT |
                            CLAY_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_MULTIPLY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_multiply,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_SCREEN,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_screen,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_DIFFERENCE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_difference,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_ADDITION,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_addition,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_SUBTRACT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_subtract,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_DARKEN_ONLY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_darken_only,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { CLAY_LAYER_MODE_LIGHTEN_ONLY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_lighten_only,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
    /* no blend_space: reuse composite space, no conversion thus fewer copies */
  },

  { CLAY_LAYER_MODE_HSV_HUE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_hsv_hue,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSV_SATURATION,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_hsv_saturation,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSL_COLOR,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_hsl_color,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_HSV_VALUE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_hsv_value,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR
  },

  { CLAY_LAYER_MODE_DIVIDE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_divide,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_DODGE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_dodge,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_BURN,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_burn,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_HARDLIGHT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_hardlight,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_SOFTLIGHT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_softlight,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_GRAIN_EXTRACT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_grain_extract,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_GRAIN_MERGE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_grain_merge,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_VIVID_LIGHT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_vivid_light,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_PIN_LIGHT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_pin_light,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_LINEAR_LIGHT,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_linear_light,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_HARD_MIX,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_hard_mix,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_EXCLUSION,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_exclusion,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_LINEAR_BURN,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_linear_burn,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL
  },

  { CLAY_LAYER_MODE_LUMA_DARKEN_ONLY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_luma_darken_only,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_luma_lighten_only,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_LUMINANCE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_luminance,
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_COLOR_ERASE,

    .op_name              = "clay:layer-mode",
    .blend_function       = clay_operation_layer_mode_blend_color_erase,
    .flags                = CLAY_LAYER_MODE_FLAG_SUBTRACTIVE,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,
    .blend_space          = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_ERASE,

    .op_name              = "clay:erase",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_SUBTRACTIVE           |
                            CLAY_LAYER_MODE_FLAG_ALPHA_ONLY            |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_MERGE,

    .op_name              = "clay:merge",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_SPLIT,

    .op_name              = "clay:split",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_SUBTRACTIVE               |
                            CLAY_LAYER_MODE_FLAG_ALPHA_ONLY                |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_ALL,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP
  },

  { CLAY_LAYER_MODE_PASS_THROUGH,

    .op_name              = "clay:pass-through",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE    |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_GROUP,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_REPLACE,

    .op_name              = "clay:replace",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_FILTER,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_OVERWRITE,

    .op_name              = "clay:overwrite",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_TRIVIAL,
    .context              = CLAY_LAYER_MODE_CONTEXT_PAINT,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION,
    .composite_space      = CLAY_LAYER_COLOR_SPACE_RGB_LINEAR
  },

  { CLAY_LAYER_MODE_ANTI_ERASE,

    .op_name              = "clay:anti-erase",
    .flags                = CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     |
                            CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE |
                            CLAY_LAYER_MODE_FLAG_ALPHA_ONLY,
    .paint_composite_mode = CLAY_LAYER_COMPOSITE_UNION,
    .composite_mode       = CLAY_LAYER_COMPOSITE_UNION
  }
};

static const ClayLayerMode layer_mode_group_default[] =
{
  CLAY_LAYER_MODE_PASS_THROUGH,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_REPLACE,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_NORMAL,
  CLAY_LAYER_MODE_DISSOLVE,
  CLAY_LAYER_MODE_BEHIND,
  CLAY_LAYER_MODE_COLOR_ERASE,
  CLAY_LAYER_MODE_ERASE,
  CLAY_LAYER_MODE_ANTI_ERASE,
  CLAY_LAYER_MODE_MERGE,
  CLAY_LAYER_MODE_SPLIT,
  CLAY_LAYER_MODE_OVERWRITE,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_LIGHTEN_ONLY,
  CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY,
  CLAY_LAYER_MODE_SCREEN,
  CLAY_LAYER_MODE_DODGE,
  CLAY_LAYER_MODE_ADDITION,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_DARKEN_ONLY,
  CLAY_LAYER_MODE_LUMA_DARKEN_ONLY,
  CLAY_LAYER_MODE_MULTIPLY,
  CLAY_LAYER_MODE_BURN,
  CLAY_LAYER_MODE_LINEAR_BURN,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_OVERLAY,
  CLAY_LAYER_MODE_SOFTLIGHT,
  CLAY_LAYER_MODE_HARDLIGHT,
  CLAY_LAYER_MODE_VIVID_LIGHT,
  CLAY_LAYER_MODE_PIN_LIGHT,
  CLAY_LAYER_MODE_LINEAR_LIGHT,
  CLAY_LAYER_MODE_HARD_MIX,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_DIFFERENCE,
  CLAY_LAYER_MODE_EXCLUSION,
  CLAY_LAYER_MODE_SUBTRACT,
  CLAY_LAYER_MODE_GRAIN_EXTRACT,
  CLAY_LAYER_MODE_GRAIN_MERGE,
  CLAY_LAYER_MODE_DIVIDE,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_HSV_HUE,
  CLAY_LAYER_MODE_HSV_SATURATION,
  CLAY_LAYER_MODE_HSL_COLOR,
  CLAY_LAYER_MODE_HSV_VALUE,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_LCH_HUE,
  CLAY_LAYER_MODE_LCH_CHROMA,
  CLAY_LAYER_MODE_LCH_COLOR,
  CLAY_LAYER_MODE_LCH_LIGHTNESS,
  CLAY_LAYER_MODE_LUMINANCE
};

static const ClayLayerMode layer_mode_group_legacy[] =
{
  CLAY_LAYER_MODE_NORMAL_LEGACY,
  CLAY_LAYER_MODE_DISSOLVE,
  CLAY_LAYER_MODE_BEHIND_LEGACY,
  CLAY_LAYER_MODE_COLOR_ERASE_LEGACY,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY,
  CLAY_LAYER_MODE_SCREEN_LEGACY,
  CLAY_LAYER_MODE_DODGE_LEGACY,
  CLAY_LAYER_MODE_ADDITION_LEGACY,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY,
  CLAY_LAYER_MODE_MULTIPLY_LEGACY,
  CLAY_LAYER_MODE_BURN_LEGACY,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_OVERLAY,
  CLAY_LAYER_MODE_SOFTLIGHT_LEGACY,
  CLAY_LAYER_MODE_HARDLIGHT_LEGACY,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_DIFFERENCE_LEGACY,
  CLAY_LAYER_MODE_SUBTRACT_LEGACY,
  CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY,
  CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY,
  CLAY_LAYER_MODE_DIVIDE_LEGACY,

  CLAY_LAYER_MODE_SEPARATOR,

  CLAY_LAYER_MODE_HSV_HUE_LEGACY,
  CLAY_LAYER_MODE_HSV_SATURATION_LEGACY,
  CLAY_LAYER_MODE_HSL_COLOR_LEGACY,
  CLAY_LAYER_MODE_HSV_VALUE_LEGACY
};

static const ClayLayerMode layer_mode_groups[][2] =
{
  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_NORMAL,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_NORMAL_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_DISSOLVE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_DISSOLVE
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_BEHIND,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_BEHIND_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_MULTIPLY,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_MULTIPLY_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_SCREEN,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_SCREEN_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_OVERLAY,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_DIFFERENCE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_DIFFERENCE_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_ADDITION,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_ADDITION_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_SUBTRACT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_SUBTRACT_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_DARKEN_ONLY,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_LIGHTEN_ONLY,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_HSV_HUE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_HSV_HUE_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_HSV_SATURATION,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_HSV_SATURATION_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_HSL_COLOR,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_HSL_COLOR_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_HSV_VALUE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_HSV_VALUE_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_DIVIDE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_DIVIDE_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_DODGE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_DODGE_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_BURN,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_BURN_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_HARDLIGHT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_HARDLIGHT_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_SOFTLIGHT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_SOFTLIGHT_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_GRAIN_EXTRACT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_GRAIN_MERGE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_COLOR_ERASE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = CLAY_LAYER_MODE_COLOR_ERASE_LEGACY,
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_VIVID_LIGHT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_PIN_LIGHT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_LINEAR_LIGHT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_HARD_MIX,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_EXCLUSION,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_LINEAR_BURN,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_LUMA_DARKEN_ONLY,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_LUMINANCE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_ERASE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_MERGE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_SPLIT,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_PASS_THROUGH,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1,
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_REPLACE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_OVERWRITE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  },

  { [CLAY_LAYER_MODE_GROUP_DEFAULT] = CLAY_LAYER_MODE_ANTI_ERASE,
    [CLAY_LAYER_MODE_GROUP_LEGACY ] = -1
  }
};

static GeglOperation *ops[G_N_ELEMENTS (layer_mode_infos)] = { 0 };

/*  public functions  */

void
clay_layer_modes_init (void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_infos); i++)
    {
      clay_assert ((ClayLayerMode) i == layer_mode_infos[i].layer_mode);
    }
}

void
clay_layer_modes_exit (void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (layer_mode_infos); i++)
    {
      if (ops[i])
        {
          GeglNode *node;

          node = ops[i]->node;
          g_object_unref (node);
          ops[i] = NULL;
        }
    }
}

static const ClayLayerModeInfo *
clay_layer_mode_info (ClayLayerMode mode)
{
  g_return_val_if_fail (mode >= 0 && mode < G_N_ELEMENTS (layer_mode_infos),
                        &layer_mode_infos[0]);

  return &layer_mode_infos[mode];
}

gboolean
clay_layer_mode_is_legacy (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_LEGACY) != 0;
}

gboolean
clay_layer_mode_is_blend_space_mutable (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE) == 0;
}

gboolean
clay_layer_mode_is_composite_space_mutable (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE) == 0;
}

gboolean
clay_layer_mode_is_composite_mode_mutable (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE) == 0;
}

gboolean
clay_layer_mode_is_subtractive (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_SUBTRACTIVE) != 0;
}

gboolean
clay_layer_mode_is_alpha_only (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_ALPHA_ONLY) != 0;
}

gboolean
clay_layer_mode_is_trivial (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return FALSE;

  return (info->flags & CLAY_LAYER_MODE_FLAG_TRIVIAL) != 0;
}

ClayLayerColorSpace
clay_layer_mode_get_blend_space (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return CLAY_LAYER_COLOR_SPACE_RGB_LINEAR;

  return info->blend_space;
}

ClayLayerColorSpace
clay_layer_mode_get_composite_space (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return CLAY_LAYER_COLOR_SPACE_RGB_LINEAR;

  return info->composite_space;
}

ClayLayerCompositeMode
clay_layer_mode_get_composite_mode (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return CLAY_LAYER_COMPOSITE_UNION;

  return info->composite_mode;
}

ClayLayerCompositeMode
clay_layer_mode_get_paint_composite_mode (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return CLAY_LAYER_COMPOSITE_UNION;

  return info->paint_composite_mode;
}

const gchar *
clay_layer_mode_get_operation_name (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return "clay:layer-mode";

  return info->op_name;
}

/**
 * clay_layer_mode_get_operation:
 * @mode:
 *
 * Returns: a #GeglOperation for @mode which may be reused and must not
 *          be freed.
 */
GeglOperation *
clay_layer_mode_get_operation (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);
  const gchar             *op_name;

  op_name = clay_layer_mode_get_operation_name (mode);

  if (! info)
    info = layer_mode_infos;

  mode = info - layer_mode_infos;

  if (! ops[mode])
    {
      GeglNode      *node;
      GeglOperation *operation;

      node = gegl_node_new_child (NULL,
                                  "operation", op_name,
                                  NULL);

      operation = gegl_node_get_gegl_operation (node);
      ops[mode] = operation;

      if (CLAY_IS_OPERATION_LAYER_MODE (operation))
        {
          ClayOperationLayerMode *layer_mode = CLAY_OPERATION_LAYER_MODE (operation);

          layer_mode->layer_mode      = mode;
          layer_mode->function        = CLAY_OPERATION_LAYER_MODE_GET_CLASS (operation)->process;
          layer_mode->blend_function  = clay_layer_mode_get_blend_function (mode);
          layer_mode->blend_space     = clay_layer_mode_get_blend_space (mode);
          layer_mode->composite_space = clay_layer_mode_get_composite_space (mode);
          layer_mode->composite_mode  = clay_layer_mode_get_paint_composite_mode (mode);
        }
    }

  return ops[mode];
}

ClayLayerModeFunc
clay_layer_mode_get_function (ClayLayerMode mode)
{
  GeglOperation *operation;

  operation = clay_layer_mode_get_operation (mode);

  return CLAY_OPERATION_LAYER_MODE_GET_CLASS (operation)->process;
}

ClayLayerModeBlendFunc
clay_layer_mode_get_blend_function (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return NULL;

  return info->blend_function;
}

ClayLayerModeContext
clay_layer_mode_get_context (ClayLayerMode mode)
{
  const ClayLayerModeInfo *info = clay_layer_mode_info (mode);

  if (! info)
    return 0;

  return info->context;
}

ClayLayerMode *
clay_layer_mode_get_context_array (ClayLayerMode         mode,
                                   ClayLayerModeContext  context,
                                   gint                 *n_modes)
{
  ClayLayerModeGroup   group;
  const ClayLayerMode *group_modes;
  gint                 n_group_modes;
  ClayLayerMode       *array;
  gint                 i;

  group = clay_layer_mode_get_group (mode);

  group_modes = clay_layer_mode_get_group_array (group, &n_group_modes);

  array = g_new0 (ClayLayerMode, n_group_modes);
  *n_modes = 0;

  for (i = 0; i < n_group_modes; i++)
    {
      if (group_modes[i] != CLAY_LAYER_MODE_SEPARATOR &&
          (clay_layer_mode_get_context (group_modes[i]) & context))
        {
          array[*n_modes] = group_modes[i];
          (*n_modes)++;
        }
    }

  return array;
}

static gboolean
is_mode_in_array (const ClayLayerMode *modes,
                  gint                 n_modes,
                  ClayLayerMode        mode)
{
  gint i;

  for (i = 0; i < n_modes; i++)
    {
      if (modes[i] == mode)
        return TRUE;
    }

  return FALSE;
}

ClayLayerModeGroup
clay_layer_mode_get_group (ClayLayerMode mode)
{
  if (is_mode_in_array (layer_mode_group_default,
                        G_N_ELEMENTS (layer_mode_group_default), mode))
    {
      return CLAY_LAYER_MODE_GROUP_DEFAULT;
    }
  else if (is_mode_in_array (layer_mode_group_legacy,
                             G_N_ELEMENTS (layer_mode_group_legacy), mode))
    {
      return CLAY_LAYER_MODE_GROUP_LEGACY;
    }

  return CLAY_LAYER_MODE_GROUP_DEFAULT;
}

const ClayLayerMode *
clay_layer_mode_get_group_array (ClayLayerModeGroup  group,
                                 gint               *n_modes)
{
  g_return_val_if_fail (n_modes != NULL, NULL);

  switch (group)
    {
    case CLAY_LAYER_MODE_GROUP_DEFAULT:
      *n_modes = G_N_ELEMENTS (layer_mode_group_default);
      return layer_mode_group_default;

    case CLAY_LAYER_MODE_GROUP_LEGACY:
      *n_modes = G_N_ELEMENTS (layer_mode_group_legacy);
      return layer_mode_group_legacy;

    default:
      g_return_val_if_reached (NULL);
    }
}

gboolean
clay_layer_mode_get_for_group (ClayLayerMode       old_mode,
                               ClayLayerModeGroup  new_group,
                               ClayLayerMode      *new_mode)
{
  gint i;

  g_return_val_if_fail (new_mode != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (layer_mode_groups); i++)
    {
      if (is_mode_in_array (layer_mode_groups[i], 2, old_mode))
        {
          *new_mode = layer_mode_groups[i][new_group];

          if (*new_mode != -1)
            return TRUE;

          return FALSE;
        }
    }

  *new_mode = -1;

  return FALSE;
}

const Babl *
clay_layer_mode_get_format (ClayLayerMode           mode,
                            ClayLayerColorSpace     blend_space,
                            ClayLayerColorSpace     composite_space,
                            ClayLayerCompositeMode  composite_mode,
                            const Babl             *preferred_format)
{
  ClayLayerCompositeRegion composite_region;

  /* for now, all modes perform i/o in the composite space. */
  (void) mode;
  (void) blend_space;

  if (composite_space == CLAY_LAYER_COLOR_SPACE_AUTO)
    composite_space = clay_layer_mode_get_composite_space (mode);

  if (composite_mode == CLAY_LAYER_COMPOSITE_AUTO)
    composite_mode = clay_layer_mode_get_composite_mode (mode);

  composite_region = clay_layer_mode_get_included_region (mode, composite_mode);

  if (clay_layer_mode_is_alpha_only (mode))
    {
      if (composite_region != CLAY_LAYER_COMPOSITE_REGION_UNION)
        {
          /* alpha-only layer modes don't combine colors in non-union composite
           * modes, hence we can disregard the composite space.
           */
          composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
        }
    }
  else if (clay_layer_mode_is_trivial (mode))
    {
      if (! (composite_region & CLAY_LAYER_COMPOSITE_REGION_DESTINATION))
        {
          /* trivial layer modes don't combine colors when only the source
           * region is included, hence we can disregard the composite space.
           */
          composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
        }
    }

  switch (composite_space)
    {
    case CLAY_LAYER_COLOR_SPACE_AUTO:
      /* compositing is color-space agnostic.  return a format that has a fast
       * conversion path to/from the preferred format.
       */
      if (! preferred_format ||
          clay_babl_format_get_trc (preferred_format) == CLAY_TRC_LINEAR)
        return babl_format_with_space ("RGBA float", preferred_format);
      else
        return babl_format_with_space ("R'G'B'A float", preferred_format);

    case CLAY_LAYER_COLOR_SPACE_RGB_LINEAR:
      return babl_format_with_space ("RGBA float", preferred_format);

    case CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR:
      return babl_format_with_space ("R'G'B'A float", preferred_format);

    case CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
      return babl_format_with_space ("R~G~B~A float", preferred_format);

    case CLAY_LAYER_COLOR_SPACE_LAB:
      return babl_format_with_space ("CIE Lab alpha float", preferred_format);

    default:
      g_return_val_if_reached (babl_format_with_space ("RGBA float", preferred_format));
    }
}

ClayLayerCompositeRegion
clay_layer_mode_get_included_region (ClayLayerMode          mode,
                                     ClayLayerCompositeMode composite_mode)
{
  if (composite_mode == CLAY_LAYER_COMPOSITE_AUTO)
    composite_mode = clay_layer_mode_get_composite_mode (mode);

  switch (composite_mode)
    {
    case CLAY_LAYER_COMPOSITE_UNION:
      return CLAY_LAYER_COMPOSITE_REGION_UNION;

    case CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      return CLAY_LAYER_COMPOSITE_REGION_DESTINATION;

    case CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER:
      return CLAY_LAYER_COMPOSITE_REGION_SOURCE;

    case CLAY_LAYER_COMPOSITE_INTERSECTION:
      return CLAY_LAYER_COMPOSITE_REGION_INTERSECTION;

    default:
      g_return_val_if_reached (CLAY_LAYER_COMPOSITE_REGION_INTERSECTION);
    }
}
