
/* Generated data (by clay-mkenums) */

#include "stamp-operations-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "operations-enums.h"
#include "clay-intl.h"

/* enumerations from "operations-enums.h" */
GType
clay_layer_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_LAYER_COLOR_SPACE_AUTO, "CLAY_LAYER_COLOR_SPACE_AUTO", "auto" },
    { CLAY_LAYER_COLOR_SPACE_RGB_LINEAR, "CLAY_LAYER_COLOR_SPACE_RGB_LINEAR", "rgb-linear" },
    { CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR, "CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR", "rgb-non-linear" },
    { CLAY_LAYER_COLOR_SPACE_LAB, "CLAY_LAYER_COLOR_SPACE_LAB", "lab" },
    { CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, "CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL", "rgb-perceptual" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_LAYER_COLOR_SPACE_AUTO, NC_("layer-color-space", "Auto"), NULL },
    { CLAY_LAYER_COLOR_SPACE_RGB_LINEAR, NC_("layer-color-space", "RGB (linear)"), NULL },
    { CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR, NC_("layer-color-space", "RGB (from color profile)"), NULL },
    { CLAY_LAYER_COLOR_SPACE_LAB, NC_("layer-color-space", "LAB"), NULL },
    { CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, NC_("layer-color-space", "RGB (perceptual)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayLayerColorSpace", values);
      clay_type_set_translation_context (type, "layer-color-space");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_layer_composite_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_LAYER_COMPOSITE_AUTO, "CLAY_LAYER_COMPOSITE_AUTO", "auto" },
    { CLAY_LAYER_COMPOSITE_UNION, "CLAY_LAYER_COMPOSITE_UNION", "union" },
    { CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP, "CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP", "clip-to-backdrop" },
    { CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER, "CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER", "clip-to-layer" },
    { CLAY_LAYER_COMPOSITE_INTERSECTION, "CLAY_LAYER_COMPOSITE_INTERSECTION", "intersection" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_LAYER_COMPOSITE_AUTO, NC_("layer-composite-mode", "Auto"), NULL },
    { CLAY_LAYER_COMPOSITE_UNION, NC_("layer-composite-mode", "Union"), NULL },
    { CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP, NC_("layer-composite-mode", "Clip to backdrop"), NULL },
    { CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER, NC_("layer-composite-mode", "Clip to layer"), NULL },
    { CLAY_LAYER_COMPOSITE_INTERSECTION, NC_("layer-composite-mode", "Intersection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayLayerCompositeMode", values);
      clay_type_set_translation_context (type, "layer-composite-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_layer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_LAYER_MODE_NORMAL_LEGACY, "CLAY_LAYER_MODE_NORMAL_LEGACY", "normal-legacy" },
    { CLAY_LAYER_MODE_DISSOLVE, "CLAY_LAYER_MODE_DISSOLVE", "dissolve" },
    { CLAY_LAYER_MODE_BEHIND_LEGACY, "CLAY_LAYER_MODE_BEHIND_LEGACY", "behind-legacy" },
    { CLAY_LAYER_MODE_MULTIPLY_LEGACY, "CLAY_LAYER_MODE_MULTIPLY_LEGACY", "multiply-legacy" },
    { CLAY_LAYER_MODE_SCREEN_LEGACY, "CLAY_LAYER_MODE_SCREEN_LEGACY", "screen-legacy" },
    { CLAY_LAYER_MODE_OVERLAY_LEGACY, "CLAY_LAYER_MODE_OVERLAY_LEGACY", "overlay-legacy" },
    { CLAY_LAYER_MODE_DIFFERENCE_LEGACY, "CLAY_LAYER_MODE_DIFFERENCE_LEGACY", "difference-legacy" },
    { CLAY_LAYER_MODE_ADDITION_LEGACY, "CLAY_LAYER_MODE_ADDITION_LEGACY", "addition-legacy" },
    { CLAY_LAYER_MODE_SUBTRACT_LEGACY, "CLAY_LAYER_MODE_SUBTRACT_LEGACY", "subtract-legacy" },
    { CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY, "CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY", "darken-only-legacy" },
    { CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY, "CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY", "lighten-only-legacy" },
    { CLAY_LAYER_MODE_HSV_HUE_LEGACY, "CLAY_LAYER_MODE_HSV_HUE_LEGACY", "hsv-hue-legacy" },
    { CLAY_LAYER_MODE_HSV_SATURATION_LEGACY, "CLAY_LAYER_MODE_HSV_SATURATION_LEGACY", "hsv-saturation-legacy" },
    { CLAY_LAYER_MODE_HSL_COLOR_LEGACY, "CLAY_LAYER_MODE_HSL_COLOR_LEGACY", "hsl-color-legacy" },
    { CLAY_LAYER_MODE_HSV_VALUE_LEGACY, "CLAY_LAYER_MODE_HSV_VALUE_LEGACY", "hsv-value-legacy" },
    { CLAY_LAYER_MODE_DIVIDE_LEGACY, "CLAY_LAYER_MODE_DIVIDE_LEGACY", "divide-legacy" },
    { CLAY_LAYER_MODE_DODGE_LEGACY, "CLAY_LAYER_MODE_DODGE_LEGACY", "dodge-legacy" },
    { CLAY_LAYER_MODE_BURN_LEGACY, "CLAY_LAYER_MODE_BURN_LEGACY", "burn-legacy" },
    { CLAY_LAYER_MODE_HARDLIGHT_LEGACY, "CLAY_LAYER_MODE_HARDLIGHT_LEGACY", "hardlight-legacy" },
    { CLAY_LAYER_MODE_SOFTLIGHT_LEGACY, "CLAY_LAYER_MODE_SOFTLIGHT_LEGACY", "softlight-legacy" },
    { CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY, "CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY", "grain-extract-legacy" },
    { CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY, "CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY", "grain-merge-legacy" },
    { CLAY_LAYER_MODE_COLOR_ERASE_LEGACY, "CLAY_LAYER_MODE_COLOR_ERASE_LEGACY", "color-erase-legacy" },
    { CLAY_LAYER_MODE_OVERLAY, "CLAY_LAYER_MODE_OVERLAY", "overlay" },
    { CLAY_LAYER_MODE_LCH_HUE, "CLAY_LAYER_MODE_LCH_HUE", "lch-hue" },
    { CLAY_LAYER_MODE_LCH_CHROMA, "CLAY_LAYER_MODE_LCH_CHROMA", "lch-chroma" },
    { CLAY_LAYER_MODE_LCH_COLOR, "CLAY_LAYER_MODE_LCH_COLOR", "lch-color" },
    { CLAY_LAYER_MODE_LCH_LIGHTNESS, "CLAY_LAYER_MODE_LCH_LIGHTNESS", "lch-lightness" },
    { CLAY_LAYER_MODE_NORMAL, "CLAY_LAYER_MODE_NORMAL", "normal" },
    { CLAY_LAYER_MODE_BEHIND, "CLAY_LAYER_MODE_BEHIND", "behind" },
    { CLAY_LAYER_MODE_MULTIPLY, "CLAY_LAYER_MODE_MULTIPLY", "multiply" },
    { CLAY_LAYER_MODE_SCREEN, "CLAY_LAYER_MODE_SCREEN", "screen" },
    { CLAY_LAYER_MODE_DIFFERENCE, "CLAY_LAYER_MODE_DIFFERENCE", "difference" },
    { CLAY_LAYER_MODE_ADDITION, "CLAY_LAYER_MODE_ADDITION", "addition" },
    { CLAY_LAYER_MODE_SUBTRACT, "CLAY_LAYER_MODE_SUBTRACT", "subtract" },
    { CLAY_LAYER_MODE_DARKEN_ONLY, "CLAY_LAYER_MODE_DARKEN_ONLY", "darken-only" },
    { CLAY_LAYER_MODE_LIGHTEN_ONLY, "CLAY_LAYER_MODE_LIGHTEN_ONLY", "lighten-only" },
    { CLAY_LAYER_MODE_HSV_HUE, "CLAY_LAYER_MODE_HSV_HUE", "hsv-hue" },
    { CLAY_LAYER_MODE_HSV_SATURATION, "CLAY_LAYER_MODE_HSV_SATURATION", "hsv-saturation" },
    { CLAY_LAYER_MODE_HSL_COLOR, "CLAY_LAYER_MODE_HSL_COLOR", "hsl-color" },
    { CLAY_LAYER_MODE_HSV_VALUE, "CLAY_LAYER_MODE_HSV_VALUE", "hsv-value" },
    { CLAY_LAYER_MODE_DIVIDE, "CLAY_LAYER_MODE_DIVIDE", "divide" },
    { CLAY_LAYER_MODE_DODGE, "CLAY_LAYER_MODE_DODGE", "dodge" },
    { CLAY_LAYER_MODE_BURN, "CLAY_LAYER_MODE_BURN", "burn" },
    { CLAY_LAYER_MODE_HARDLIGHT, "CLAY_LAYER_MODE_HARDLIGHT", "hardlight" },
    { CLAY_LAYER_MODE_SOFTLIGHT, "CLAY_LAYER_MODE_SOFTLIGHT", "softlight" },
    { CLAY_LAYER_MODE_GRAIN_EXTRACT, "CLAY_LAYER_MODE_GRAIN_EXTRACT", "grain-extract" },
    { CLAY_LAYER_MODE_GRAIN_MERGE, "CLAY_LAYER_MODE_GRAIN_MERGE", "grain-merge" },
    { CLAY_LAYER_MODE_VIVID_LIGHT, "CLAY_LAYER_MODE_VIVID_LIGHT", "vivid-light" },
    { CLAY_LAYER_MODE_PIN_LIGHT, "CLAY_LAYER_MODE_PIN_LIGHT", "pin-light" },
    { CLAY_LAYER_MODE_LINEAR_LIGHT, "CLAY_LAYER_MODE_LINEAR_LIGHT", "linear-light" },
    { CLAY_LAYER_MODE_HARD_MIX, "CLAY_LAYER_MODE_HARD_MIX", "hard-mix" },
    { CLAY_LAYER_MODE_EXCLUSION, "CLAY_LAYER_MODE_EXCLUSION", "exclusion" },
    { CLAY_LAYER_MODE_LINEAR_BURN, "CLAY_LAYER_MODE_LINEAR_BURN", "linear-burn" },
    { CLAY_LAYER_MODE_LUMA_DARKEN_ONLY, "CLAY_LAYER_MODE_LUMA_DARKEN_ONLY", "luma-darken-only" },
    { CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY, "CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY", "luma-lighten-only" },
    { CLAY_LAYER_MODE_LUMINANCE, "CLAY_LAYER_MODE_LUMINANCE", "luminance" },
    { CLAY_LAYER_MODE_COLOR_ERASE, "CLAY_LAYER_MODE_COLOR_ERASE", "color-erase" },
    { CLAY_LAYER_MODE_ERASE, "CLAY_LAYER_MODE_ERASE", "erase" },
    { CLAY_LAYER_MODE_MERGE, "CLAY_LAYER_MODE_MERGE", "merge" },
    { CLAY_LAYER_MODE_SPLIT, "CLAY_LAYER_MODE_SPLIT", "split" },
    { CLAY_LAYER_MODE_PASS_THROUGH, "CLAY_LAYER_MODE_PASS_THROUGH", "pass-through" },
    { CLAY_LAYER_MODE_REPLACE, "CLAY_LAYER_MODE_REPLACE", "replace" },
    { CLAY_LAYER_MODE_OVERWRITE, "CLAY_LAYER_MODE_OVERWRITE", "overwrite" },
    { CLAY_LAYER_MODE_ANTI_ERASE, "CLAY_LAYER_MODE_ANTI_ERASE", "anti-erase" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_LAYER_MODE_NORMAL_LEGACY, NC_("layer-mode", "Normal (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Normal (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_NORMAL_LEGACY, NC_("layer-mode", "Normal (l)"), NULL },
    { CLAY_LAYER_MODE_DISSOLVE, NC_("layer-mode", "Dissolve"), NULL },
    { CLAY_LAYER_MODE_BEHIND_LEGACY, NC_("layer-mode", "Behind (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Behind (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_BEHIND_LEGACY, NC_("layer-mode", "Behind (l)"), NULL },
    { CLAY_LAYER_MODE_MULTIPLY_LEGACY, NC_("layer-mode", "Multiply (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Multiply (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_MULTIPLY_LEGACY, NC_("layer-mode", "Multiply (l)"), NULL },
    { CLAY_LAYER_MODE_SCREEN_LEGACY, NC_("layer-mode", "Screen (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Screen (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_SCREEN_LEGACY, NC_("layer-mode", "Screen (l)"), NULL },
    { CLAY_LAYER_MODE_OVERLAY_LEGACY, NC_("layer-mode", "Old broken Overlay"), NULL },
    /* Translators: this is an abbreviated version of "Old broken Overlay".
       Keep it short. */
    { CLAY_LAYER_MODE_OVERLAY_LEGACY, NC_("layer-mode", "Old Overlay"), NULL },
    { CLAY_LAYER_MODE_DIFFERENCE_LEGACY, NC_("layer-mode", "Difference (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Difference (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_DIFFERENCE_LEGACY, NC_("layer-mode", "Difference (l)"), NULL },
    { CLAY_LAYER_MODE_ADDITION_LEGACY, NC_("layer-mode", "Addition (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Addition (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_ADDITION_LEGACY, NC_("layer-mode", "Addition (l)"), NULL },
    { CLAY_LAYER_MODE_SUBTRACT_LEGACY, NC_("layer-mode", "Subtract (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Subtract (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_SUBTRACT_LEGACY, NC_("layer-mode", "Subtract (l)"), NULL },
    { CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY, NC_("layer-mode", "Darken only (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Darken only (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY, NC_("layer-mode", "Darken only (l)"), NULL },
    { CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY, NC_("layer-mode", "Lighten only (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Lighten only (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY, NC_("layer-mode", "Lighten only (l)"), NULL },
    { CLAY_LAYER_MODE_HSV_HUE_LEGACY, NC_("layer-mode", "HSV Hue (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Hue (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_HSV_HUE_LEGACY, NC_("layer-mode", "HSV Hue (l)"), NULL },
    { CLAY_LAYER_MODE_HSV_SATURATION_LEGACY, NC_("layer-mode", "HSV Saturation (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Saturation (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_HSV_SATURATION_LEGACY, NC_("layer-mode", "HSV Saturation (l)"), NULL },
    { CLAY_LAYER_MODE_HSL_COLOR_LEGACY, NC_("layer-mode", "HSL Color (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSL Color (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_HSL_COLOR_LEGACY, NC_("layer-mode", "HSL Color (l)"), NULL },
    { CLAY_LAYER_MODE_HSV_VALUE_LEGACY, NC_("layer-mode", "HSV Value (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "HSV Value (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_HSV_VALUE_LEGACY, NC_("layer-mode", "HSV Value (l)"), NULL },
    { CLAY_LAYER_MODE_DIVIDE_LEGACY, NC_("layer-mode", "Divide (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Divide (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_DIVIDE_LEGACY, NC_("layer-mode", "Divide (l)"), NULL },
    { CLAY_LAYER_MODE_DODGE_LEGACY, NC_("layer-mode", "Dodge (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Dodge (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_DODGE_LEGACY, NC_("layer-mode", "Dodge (l)"), NULL },
    { CLAY_LAYER_MODE_BURN_LEGACY, NC_("layer-mode", "Burn (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Burn (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_BURN_LEGACY, NC_("layer-mode", "Burn (l)"), NULL },
    { CLAY_LAYER_MODE_HARDLIGHT_LEGACY, NC_("layer-mode", "Hard light (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Hard light (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_HARDLIGHT_LEGACY, NC_("layer-mode", "Hard light (l)"), NULL },
    { CLAY_LAYER_MODE_SOFTLIGHT_LEGACY, NC_("layer-mode", "Soft light (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Soft light (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_SOFTLIGHT_LEGACY, NC_("layer-mode", "Soft light (l)"), NULL },
    { CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY, NC_("layer-mode", "Grain extract (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Grain extract (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY, NC_("layer-mode", "Grain extract (l)"), NULL },
    { CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY, NC_("layer-mode", "Grain merge (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Grain merge (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY, NC_("layer-mode", "Grain merge (l)"), NULL },
    { CLAY_LAYER_MODE_COLOR_ERASE_LEGACY, NC_("layer-mode", "Color erase (legacy)"), NULL },
    /* Translators: this is an abbreviated version of "Color erase (legacy)".
       Keep it short. */
    { CLAY_LAYER_MODE_COLOR_ERASE_LEGACY, NC_("layer-mode", "Color erase (l)"), NULL },
    { CLAY_LAYER_MODE_OVERLAY, NC_("layer-mode", "Overlay"), NULL },
    { CLAY_LAYER_MODE_LCH_HUE, NC_("layer-mode", "LCh Hue"), NULL },
    { CLAY_LAYER_MODE_LCH_CHROMA, NC_("layer-mode", "LCh Chroma"), NULL },
    { CLAY_LAYER_MODE_LCH_COLOR, NC_("layer-mode", "LCh Color"), NULL },
    { CLAY_LAYER_MODE_LCH_LIGHTNESS, NC_("layer-mode", "LCh Lightness"), NULL },
    { CLAY_LAYER_MODE_NORMAL, NC_("layer-mode", "Normal"), NULL },
    { CLAY_LAYER_MODE_BEHIND, NC_("layer-mode", "Behind"), NULL },
    { CLAY_LAYER_MODE_MULTIPLY, NC_("layer-mode", "Multiply"), NULL },
    { CLAY_LAYER_MODE_SCREEN, NC_("layer-mode", "Screen"), NULL },
    { CLAY_LAYER_MODE_DIFFERENCE, NC_("layer-mode", "Difference"), NULL },
    { CLAY_LAYER_MODE_ADDITION, NC_("layer-mode", "Addition"), NULL },
    { CLAY_LAYER_MODE_SUBTRACT, NC_("layer-mode", "Subtract"), NULL },
    { CLAY_LAYER_MODE_DARKEN_ONLY, NC_("layer-mode", "Darken only"), NULL },
    { CLAY_LAYER_MODE_LIGHTEN_ONLY, NC_("layer-mode", "Lighten only"), NULL },
    { CLAY_LAYER_MODE_HSV_HUE, NC_("layer-mode", "HSV Hue"), NULL },
    { CLAY_LAYER_MODE_HSV_SATURATION, NC_("layer-mode", "HSV Saturation"), NULL },
    { CLAY_LAYER_MODE_HSL_COLOR, NC_("layer-mode", "HSL Color"), NULL },
    { CLAY_LAYER_MODE_HSV_VALUE, NC_("layer-mode", "HSV Value"), NULL },
    { CLAY_LAYER_MODE_DIVIDE, NC_("layer-mode", "Divide"), NULL },
    { CLAY_LAYER_MODE_DODGE, NC_("layer-mode", "Dodge"), NULL },
    { CLAY_LAYER_MODE_BURN, NC_("layer-mode", "Burn"), NULL },
    { CLAY_LAYER_MODE_HARDLIGHT, NC_("layer-mode", "Hard light"), NULL },
    { CLAY_LAYER_MODE_SOFTLIGHT, NC_("layer-mode", "Soft light"), NULL },
    { CLAY_LAYER_MODE_GRAIN_EXTRACT, NC_("layer-mode", "Grain extract"), NULL },
    { CLAY_LAYER_MODE_GRAIN_MERGE, NC_("layer-mode", "Grain merge"), NULL },
    { CLAY_LAYER_MODE_VIVID_LIGHT, NC_("layer-mode", "Vivid light"), NULL },
    { CLAY_LAYER_MODE_PIN_LIGHT, NC_("layer-mode", "Pin light"), NULL },
    { CLAY_LAYER_MODE_LINEAR_LIGHT, NC_("layer-mode", "Linear light"), NULL },
    { CLAY_LAYER_MODE_HARD_MIX, NC_("layer-mode", "Hard mix"), NULL },
    { CLAY_LAYER_MODE_EXCLUSION, NC_("layer-mode", "Exclusion"), NULL },
    { CLAY_LAYER_MODE_LINEAR_BURN, NC_("layer-mode", "Linear burn"), NULL },
    { CLAY_LAYER_MODE_LUMA_DARKEN_ONLY, NC_("layer-mode", "Luma/Luminance darken only"), NULL },
    /* Translators: this is an abbreviated version of "Luma/Luminance darken only".
       Keep it short. */
    { CLAY_LAYER_MODE_LUMA_DARKEN_ONLY, NC_("layer-mode", "Luma darken only"), NULL },
    { CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY, NC_("layer-mode", "Luma/Luminance lighten only"), NULL },
    /* Translators: this is an abbreviated version of "Luma/Luminance lighten only".
       Keep it short. */
    { CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY, NC_("layer-mode", "Luma lighten only"), NULL },
    { CLAY_LAYER_MODE_LUMINANCE, NC_("layer-mode", "Luminance"), NULL },
    { CLAY_LAYER_MODE_COLOR_ERASE, NC_("layer-mode", "Color erase"), NULL },
    { CLAY_LAYER_MODE_ERASE, NC_("layer-mode", "Erase"), NULL },
    { CLAY_LAYER_MODE_MERGE, NC_("layer-mode", "Merge"), NULL },
    { CLAY_LAYER_MODE_SPLIT, NC_("layer-mode", "Split"), NULL },
    { CLAY_LAYER_MODE_PASS_THROUGH, NC_("layer-mode", "Pass through"), NULL },
    { CLAY_LAYER_MODE_REPLACE, NC_("layer-mode", "Replace"), NULL },
    { CLAY_LAYER_MODE_OVERWRITE, NC_("layer-mode", "Overwrite"), NULL },
    { CLAY_LAYER_MODE_ANTI_ERASE, NC_("layer-mode", "Anti erase"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayLayerMode", values);
      clay_type_set_translation_context (type, "layer-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_layer_mode_group_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_LAYER_MODE_GROUP_DEFAULT, "CLAY_LAYER_MODE_GROUP_DEFAULT", "default" },
    { CLAY_LAYER_MODE_GROUP_LEGACY, "CLAY_LAYER_MODE_GROUP_LEGACY", "legacy" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_LAYER_MODE_GROUP_DEFAULT, NC_("layer-mode-group", "Default"), NULL },
    { CLAY_LAYER_MODE_GROUP_LEGACY, NC_("layer-mode-group", "Legacy"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayLayerModeGroup", values);
      clay_type_set_translation_context (type, "layer-mode-group");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_layer_mode_context_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { CLAY_LAYER_MODE_CONTEXT_LAYER, "CLAY_LAYER_MODE_CONTEXT_LAYER", "layer" },
    { CLAY_LAYER_MODE_CONTEXT_GROUP, "CLAY_LAYER_MODE_CONTEXT_GROUP", "group" },
    { CLAY_LAYER_MODE_CONTEXT_PAINT, "CLAY_LAYER_MODE_CONTEXT_PAINT", "paint" },
    { CLAY_LAYER_MODE_CONTEXT_FILTER, "CLAY_LAYER_MODE_CONTEXT_FILTER", "filter" },
    { CLAY_LAYER_MODE_CONTEXT_ALL, "CLAY_LAYER_MODE_CONTEXT_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const ClayFlagsDesc descs[] =
  {
    { CLAY_LAYER_MODE_CONTEXT_LAYER, "CLAY_LAYER_MODE_CONTEXT_LAYER", NULL },
    { CLAY_LAYER_MODE_CONTEXT_GROUP, "CLAY_LAYER_MODE_CONTEXT_GROUP", NULL },
    { CLAY_LAYER_MODE_CONTEXT_PAINT, "CLAY_LAYER_MODE_CONTEXT_PAINT", NULL },
    { CLAY_LAYER_MODE_CONTEXT_FILTER, "CLAY_LAYER_MODE_CONTEXT_FILTER", NULL },
    { CLAY_LAYER_MODE_CONTEXT_ALL, "CLAY_LAYER_MODE_CONTEXT_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("ClayLayerModeContext", values);
      clay_type_set_translation_context (type, "layer-mode-context");
      clay_flags_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

