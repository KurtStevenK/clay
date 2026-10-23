
/* Generated data (by clay-mkenums) */

#include "stamp-claybaseenums.h"
#include "config.h"
#include <glib-object.h>
#include "claybasetypes.h"
#include "libclay/libclay-intl.h"
#include "claybaseenums.h"


/* enumerations from "claybaseenums.h" */
GType
clay_add_mask_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ADD_MASK_WHITE, "CLAY_ADD_MASK_WHITE", "white" },
    { CLAY_ADD_MASK_BLACK, "CLAY_ADD_MASK_BLACK", "black" },
    { CLAY_ADD_MASK_ALPHA, "CLAY_ADD_MASK_ALPHA", "alpha" },
    { CLAY_ADD_MASK_ALPHA_TRANSFER, "CLAY_ADD_MASK_ALPHA_TRANSFER", "alpha-transfer" },
    { CLAY_ADD_MASK_SELECTION, "CLAY_ADD_MASK_SELECTION", "selection" },
    { CLAY_ADD_MASK_COPY, "CLAY_ADD_MASK_COPY", "copy" },
    { CLAY_ADD_MASK_CHANNEL, "CLAY_ADD_MASK_CHANNEL", "channel" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ADD_MASK_WHITE, NC_("add-mask-type", "_White (full opacity)"), NULL },
    { CLAY_ADD_MASK_BLACK, NC_("add-mask-type", "_Black (full transparency)"), NULL },
    { CLAY_ADD_MASK_ALPHA, NC_("add-mask-type", "Layer's _alpha channel"), NULL },
    { CLAY_ADD_MASK_ALPHA_TRANSFER, NC_("add-mask-type", "_Transfer layer's alpha channel"), NULL },
    { CLAY_ADD_MASK_SELECTION, NC_("add-mask-type", "_Selection"), NULL },
    { CLAY_ADD_MASK_COPY, NC_("add-mask-type", "_Grayscale copy of layer"), NULL },
    { CLAY_ADD_MASK_CHANNEL, NC_("add-mask-type", "C_hannel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayAddMaskType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "add-mask-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_brush_generated_shape_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_BRUSH_GENERATED_CIRCLE, "CLAY_BRUSH_GENERATED_CIRCLE", "circle" },
    { CLAY_BRUSH_GENERATED_SQUARE, "CLAY_BRUSH_GENERATED_SQUARE", "square" },
    { CLAY_BRUSH_GENERATED_DIAMOND, "CLAY_BRUSH_GENERATED_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_BRUSH_GENERATED_CIRCLE, NC_("brush-generated-shape", "Circle"), NULL },
    { CLAY_BRUSH_GENERATED_SQUARE, NC_("brush-generated-shape", "Square"), NULL },
    { CLAY_BRUSH_GENERATED_DIAMOND, NC_("brush-generated-shape", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayBrushGeneratedShape", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "brush-generated-shape");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_cap_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CAP_BUTT, "CLAY_CAP_BUTT", "butt" },
    { CLAY_CAP_ROUND, "CLAY_CAP_ROUND", "round" },
    { CLAY_CAP_SQUARE, "CLAY_CAP_SQUARE", "square" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CAP_BUTT, NC_("cap-style", "Butt"), NULL },
    { CLAY_CAP_ROUND, NC_("cap-style", "Round"), NULL },
    { CLAY_CAP_SQUARE, NC_("cap-style", "Square"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCapStyle", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "cap-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_channel_ops_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CHANNEL_OP_ADD, "CLAY_CHANNEL_OP_ADD", "add" },
    { CLAY_CHANNEL_OP_SUBTRACT, "CLAY_CHANNEL_OP_SUBTRACT", "subtract" },
    { CLAY_CHANNEL_OP_REPLACE, "CLAY_CHANNEL_OP_REPLACE", "replace" },
    { CLAY_CHANNEL_OP_INTERSECT, "CLAY_CHANNEL_OP_INTERSECT", "intersect" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CHANNEL_OP_ADD, NC_("channel-ops", "Add to the current selection"), NULL },
    { CLAY_CHANNEL_OP_SUBTRACT, NC_("channel-ops", "Subtract from the current selection"), NULL },
    { CLAY_CHANNEL_OP_REPLACE, NC_("channel-ops", "Replace the current selection"), NULL },
    { CLAY_CHANNEL_OP_INTERSECT, NC_("channel-ops", "Intersect with the current selection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayChannelOps", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "channel-ops");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_channel_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CHANNEL_RED, "CLAY_CHANNEL_RED", "red" },
    { CLAY_CHANNEL_GREEN, "CLAY_CHANNEL_GREEN", "green" },
    { CLAY_CHANNEL_BLUE, "CLAY_CHANNEL_BLUE", "blue" },
    { CLAY_CHANNEL_GRAY, "CLAY_CHANNEL_GRAY", "gray" },
    { CLAY_CHANNEL_INDEXED, "CLAY_CHANNEL_INDEXED", "indexed" },
    { CLAY_CHANNEL_ALPHA, "CLAY_CHANNEL_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CHANNEL_RED, NC_("channel-type", "Red"), NULL },
    { CLAY_CHANNEL_GREEN, NC_("channel-type", "Green"), NULL },
    { CLAY_CHANNEL_BLUE, NC_("channel-type", "Blue"), NULL },
    { CLAY_CHANNEL_GRAY, NC_("channel-type", "Gray"), NULL },
    { CLAY_CHANNEL_INDEXED, NC_("channel-type", "Indexed"), NULL },
    { CLAY_CHANNEL_ALPHA, NC_("channel-type", "Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayChannelType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "channel-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_check_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CHECK_SIZE_SMALL_CHECKS, "CLAY_CHECK_SIZE_SMALL_CHECKS", "small-checks" },
    { CLAY_CHECK_SIZE_MEDIUM_CHECKS, "CLAY_CHECK_SIZE_MEDIUM_CHECKS", "medium-checks" },
    { CLAY_CHECK_SIZE_LARGE_CHECKS, "CLAY_CHECK_SIZE_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CHECK_SIZE_SMALL_CHECKS, NC_("check-size", "Small"), NULL },
    { CLAY_CHECK_SIZE_MEDIUM_CHECKS, NC_("check-size", "Medium"), NULL },
    { CLAY_CHECK_SIZE_LARGE_CHECKS, NC_("check-size", "Large"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCheckSize", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "check-size");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_check_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CHECK_TYPE_LIGHT_CHECKS, "CLAY_CHECK_TYPE_LIGHT_CHECKS", "light-checks" },
    { CLAY_CHECK_TYPE_GRAY_CHECKS, "CLAY_CHECK_TYPE_GRAY_CHECKS", "gray-checks" },
    { CLAY_CHECK_TYPE_DARK_CHECKS, "CLAY_CHECK_TYPE_DARK_CHECKS", "dark-checks" },
    { CLAY_CHECK_TYPE_WHITE_ONLY, "CLAY_CHECK_TYPE_WHITE_ONLY", "white-only" },
    { CLAY_CHECK_TYPE_GRAY_ONLY, "CLAY_CHECK_TYPE_GRAY_ONLY", "gray-only" },
    { CLAY_CHECK_TYPE_BLACK_ONLY, "CLAY_CHECK_TYPE_BLACK_ONLY", "black-only" },
    { CLAY_CHECK_TYPE_CUSTOM_CHECKS, "CLAY_CHECK_TYPE_CUSTOM_CHECKS", "custom-checks" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CHECK_TYPE_LIGHT_CHECKS, NC_("check-type", "Light checks"), NULL },
    { CLAY_CHECK_TYPE_GRAY_CHECKS, NC_("check-type", "Mid-tone checks"), NULL },
    { CLAY_CHECK_TYPE_DARK_CHECKS, NC_("check-type", "Dark checks"), NULL },
    { CLAY_CHECK_TYPE_WHITE_ONLY, NC_("check-type", "White only"), NULL },
    { CLAY_CHECK_TYPE_GRAY_ONLY, NC_("check-type", "Gray only"), NULL },
    { CLAY_CHECK_TYPE_BLACK_ONLY, NC_("check-type", "Black only"), NULL },
    { CLAY_CHECK_TYPE_CUSTOM_CHECKS, NC_("check-type", "Custom checks"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCheckType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "check-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_clone_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CLONE_IMAGE, "CLAY_CLONE_IMAGE", "image" },
    { CLAY_CLONE_PATTERN, "CLAY_CLONE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CLONE_IMAGE, NC_("clone-type", "Image"), NULL },
    { CLAY_CLONE_PATTERN, NC_("clone-type", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCloneType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "clone-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_tag_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_TAG_NONE, "CLAY_COLOR_TAG_NONE", "none" },
    { CLAY_COLOR_TAG_BLUE, "CLAY_COLOR_TAG_BLUE", "blue" },
    { CLAY_COLOR_TAG_GREEN, "CLAY_COLOR_TAG_GREEN", "green" },
    { CLAY_COLOR_TAG_YELLOW, "CLAY_COLOR_TAG_YELLOW", "yellow" },
    { CLAY_COLOR_TAG_ORANGE, "CLAY_COLOR_TAG_ORANGE", "orange" },
    { CLAY_COLOR_TAG_BROWN, "CLAY_COLOR_TAG_BROWN", "brown" },
    { CLAY_COLOR_TAG_RED, "CLAY_COLOR_TAG_RED", "red" },
    { CLAY_COLOR_TAG_VIOLET, "CLAY_COLOR_TAG_VIOLET", "violet" },
    { CLAY_COLOR_TAG_GRAY, "CLAY_COLOR_TAG_GRAY", "gray" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_TAG_NONE, NC_("color-tag", "None"), NULL },
    { CLAY_COLOR_TAG_BLUE, NC_("color-tag", "Blue"), NULL },
    { CLAY_COLOR_TAG_GREEN, NC_("color-tag", "Green"), NULL },
    { CLAY_COLOR_TAG_YELLOW, NC_("color-tag", "Yellow"), NULL },
    { CLAY_COLOR_TAG_ORANGE, NC_("color-tag", "Orange"), NULL },
    { CLAY_COLOR_TAG_BROWN, NC_("color-tag", "Brown"), NULL },
    { CLAY_COLOR_TAG_RED, NC_("color-tag", "Red"), NULL },
    { CLAY_COLOR_TAG_VIOLET, NC_("color-tag", "Violet"), NULL },
    { CLAY_COLOR_TAG_GRAY, NC_("color-tag", "Gray"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorTag", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "color-tag");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_component_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COMPONENT_TYPE_U8, "CLAY_COMPONENT_TYPE_U8", "u8" },
    { CLAY_COMPONENT_TYPE_U16, "CLAY_COMPONENT_TYPE_U16", "u16" },
    { CLAY_COMPONENT_TYPE_U32, "CLAY_COMPONENT_TYPE_U32", "u32" },
    { CLAY_COMPONENT_TYPE_HALF, "CLAY_COMPONENT_TYPE_HALF", "half" },
    { CLAY_COMPONENT_TYPE_FLOAT, "CLAY_COMPONENT_TYPE_FLOAT", "float" },
    { CLAY_COMPONENT_TYPE_DOUBLE, "CLAY_COMPONENT_TYPE_DOUBLE", "double" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COMPONENT_TYPE_U8, NC_("component-type", "8-bit integer"), NULL },
    { CLAY_COMPONENT_TYPE_U16, NC_("component-type", "16-bit integer"), NULL },
    { CLAY_COMPONENT_TYPE_U32, NC_("component-type", "32-bit integer"), NULL },
    { CLAY_COMPONENT_TYPE_HALF, NC_("component-type", "16-bit floating point"), NULL },
    { CLAY_COMPONENT_TYPE_FLOAT, NC_("component-type", "32-bit floating point"), NULL },
    { CLAY_COMPONENT_TYPE_DOUBLE, NC_("component-type", "64-bit floating point"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayComponentType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "component-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_convert_palette_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CONVERT_PALETTE_GENERATE, "CLAY_CONVERT_PALETTE_GENERATE", "generate" },
    { CLAY_CONVERT_PALETTE_WEB, "CLAY_CONVERT_PALETTE_WEB", "web" },
    { CLAY_CONVERT_PALETTE_MONO, "CLAY_CONVERT_PALETTE_MONO", "mono" },
    { CLAY_CONVERT_PALETTE_CUSTOM, "CLAY_CONVERT_PALETTE_CUSTOM", "custom" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CONVERT_PALETTE_GENERATE, NC_("convert-palette-type", "_Generate optimum palette"), NULL },
    { CLAY_CONVERT_PALETTE_WEB, NC_("convert-palette-type", "Use _web-optimized palette"), NULL },
    { CLAY_CONVERT_PALETTE_MONO, NC_("convert-palette-type", "Use _black and white (1-bit) palette"), NULL },
    { CLAY_CONVERT_PALETTE_CUSTOM, NC_("convert-palette-type", "Use custom _palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayConvertPaletteType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "convert-palette-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_convolve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CONVOLVE_BLUR, "CLAY_CONVOLVE_BLUR", "blur" },
    { CLAY_CONVOLVE_SHARPEN, "CLAY_CONVOLVE_SHARPEN", "sharpen" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CONVOLVE_BLUR, NC_("convolve-type", "Blur"), NULL },
    { CLAY_CONVOLVE_SHARPEN, NC_("convolve-type", "Sharpen"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayConvolveType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "convolve-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_desaturate_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_DESATURATE_LIGHTNESS, "CLAY_DESATURATE_LIGHTNESS", "lightness" },
    { CLAY_DESATURATE_LUMA, "CLAY_DESATURATE_LUMA", "luma" },
    { CLAY_DESATURATE_AVERAGE, "CLAY_DESATURATE_AVERAGE", "average" },
    { CLAY_DESATURATE_LUMINANCE, "CLAY_DESATURATE_LUMINANCE", "luminance" },
    { CLAY_DESATURATE_VALUE, "CLAY_DESATURATE_VALUE", "value" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_DESATURATE_LIGHTNESS, NC_("desaturate-mode", "Lightness (HSL)"), NULL },
    { CLAY_DESATURATE_LUMA, NC_("desaturate-mode", "Luma"), NULL },
    { CLAY_DESATURATE_AVERAGE, NC_("desaturate-mode", "Average (HSI Intensity)"), NULL },
    { CLAY_DESATURATE_LUMINANCE, NC_("desaturate-mode", "Luminance"), NULL },
    { CLAY_DESATURATE_VALUE, NC_("desaturate-mode", "Value (HSV)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayDesaturateMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "desaturate-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_dodge_burn_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_DODGE_BURN_TYPE_DODGE, "CLAY_DODGE_BURN_TYPE_DODGE", "dodge" },
    { CLAY_DODGE_BURN_TYPE_BURN, "CLAY_DODGE_BURN_TYPE_BURN", "burn" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_DODGE_BURN_TYPE_DODGE, NC_("dodge-burn-type", "Dodge"), NULL },
    { CLAY_DODGE_BURN_TYPE_BURN, NC_("dodge-burn-type", "Burn"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayDodgeBurnType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "dodge-burn-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_fill_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_FILL_FOREGROUND, "CLAY_FILL_FOREGROUND", "foreground" },
    { CLAY_FILL_BACKGROUND, "CLAY_FILL_BACKGROUND", "background" },
    { CLAY_FILL_CIELAB_MIDDLE_GRAY, "CLAY_FILL_CIELAB_MIDDLE_GRAY", "cielab-middle-gray" },
    { CLAY_FILL_WHITE, "CLAY_FILL_WHITE", "white" },
    { CLAY_FILL_TRANSPARENT, "CLAY_FILL_TRANSPARENT", "transparent" },
    { CLAY_FILL_PATTERN, "CLAY_FILL_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_FILL_FOREGROUND, NC_("fill-type", "Foreground color"), NULL },
    { CLAY_FILL_BACKGROUND, NC_("fill-type", "Background color"), NULL },
    { CLAY_FILL_CIELAB_MIDDLE_GRAY, NC_("fill-type", "Middle Gray (CIELAB)"), NULL },
    { CLAY_FILL_WHITE, NC_("fill-type", "White"), NULL },
    { CLAY_FILL_TRANSPARENT, NC_("fill-type", "Transparency"), NULL },
    { CLAY_FILL_PATTERN, NC_("fill-type", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayFillType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "fill-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_foreground_extract_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_FOREGROUND_EXTRACT_MATTING, "CLAY_FOREGROUND_EXTRACT_MATTING", "matting" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_FOREGROUND_EXTRACT_MATTING, "CLAY_FOREGROUND_EXTRACT_MATTING", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayForegroundExtractMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "foreground-extract-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_gradient_blend_color_space_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRADIENT_BLEND_RGB_PERCEPTUAL, "CLAY_GRADIENT_BLEND_RGB_PERCEPTUAL", "rgb-perceptual" },
    { CLAY_GRADIENT_BLEND_RGB_LINEAR, "CLAY_GRADIENT_BLEND_RGB_LINEAR", "rgb-linear" },
    { CLAY_GRADIENT_BLEND_CIE_LAB, "CLAY_GRADIENT_BLEND_CIE_LAB", "cie-lab" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRADIENT_BLEND_RGB_PERCEPTUAL, NC_("gradient-blend-color-space", "Perceptual RGB"), NULL },
    { CLAY_GRADIENT_BLEND_RGB_LINEAR, NC_("gradient-blend-color-space", "Linear RGB"), NULL },
    { CLAY_GRADIENT_BLEND_CIE_LAB, NC_("gradient-blend-color-space", "CIE Lab"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGradientBlendColorSpace", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "gradient-blend-color-space");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_gradient_segment_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRADIENT_SEGMENT_RGB, "CLAY_GRADIENT_SEGMENT_RGB", "rgb" },
    { CLAY_GRADIENT_SEGMENT_HSV_CCW, "CLAY_GRADIENT_SEGMENT_HSV_CCW", "hsv-ccw" },
    { CLAY_GRADIENT_SEGMENT_HSV_CW, "CLAY_GRADIENT_SEGMENT_HSV_CW", "hsv-cw" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRADIENT_SEGMENT_RGB, NC_("gradient-segment-color", "RGB"), NULL },
    { CLAY_GRADIENT_SEGMENT_HSV_CCW, NC_("gradient-segment-color", "HSV (counter-clockwise hue)"), NULL },
    /* Translators: this is an abbreviated version of "HSV (counter-clockwise hue)".
       Keep it short. */
    { CLAY_GRADIENT_SEGMENT_HSV_CCW, NC_("gradient-segment-color", "HSV (ccw)"), NULL },
    { CLAY_GRADIENT_SEGMENT_HSV_CW, NC_("gradient-segment-color", "HSV (clockwise hue)"), NULL },
    /* Translators: this is an abbreviated version of "HSV (clockwise hue)".
       Keep it short. */
    { CLAY_GRADIENT_SEGMENT_HSV_CW, NC_("gradient-segment-color", "HSV (cw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGradientSegmentColor", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "gradient-segment-color");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_gradient_segment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRADIENT_SEGMENT_LINEAR, "CLAY_GRADIENT_SEGMENT_LINEAR", "linear" },
    { CLAY_GRADIENT_SEGMENT_CURVED, "CLAY_GRADIENT_SEGMENT_CURVED", "curved" },
    { CLAY_GRADIENT_SEGMENT_SINE, "CLAY_GRADIENT_SEGMENT_SINE", "sine" },
    { CLAY_GRADIENT_SEGMENT_SPHERE_INCREASING, "CLAY_GRADIENT_SEGMENT_SPHERE_INCREASING", "sphere-increasing" },
    { CLAY_GRADIENT_SEGMENT_SPHERE_DECREASING, "CLAY_GRADIENT_SEGMENT_SPHERE_DECREASING", "sphere-decreasing" },
    { CLAY_GRADIENT_SEGMENT_STEP, "CLAY_GRADIENT_SEGMENT_STEP", "step" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRADIENT_SEGMENT_LINEAR, NC_("gradient-segment-type", "Linear"), NULL },
    { CLAY_GRADIENT_SEGMENT_CURVED, NC_("gradient-segment-type", "Curved"), NULL },
    { CLAY_GRADIENT_SEGMENT_SINE, NC_("gradient-segment-type", "Sinusoidal"), NULL },
    { CLAY_GRADIENT_SEGMENT_SPHERE_INCREASING, NC_("gradient-segment-type", "Spherical (increasing)"), NULL },
    /* Translators: this is an abbreviated version of "Spherical (increasing)".
       Keep it short. */
    { CLAY_GRADIENT_SEGMENT_SPHERE_INCREASING, NC_("gradient-segment-type", "Spherical (inc)"), NULL },
    { CLAY_GRADIENT_SEGMENT_SPHERE_DECREASING, NC_("gradient-segment-type", "Spherical (decreasing)"), NULL },
    /* Translators: this is an abbreviated version of "Spherical (decreasing)".
       Keep it short. */
    { CLAY_GRADIENT_SEGMENT_SPHERE_DECREASING, NC_("gradient-segment-type", "Spherical (dec)"), NULL },
    { CLAY_GRADIENT_SEGMENT_STEP, NC_("gradient-segment-type", "Step"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGradientSegmentType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "gradient-segment-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_gradient_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRADIENT_LINEAR, "CLAY_GRADIENT_LINEAR", "linear" },
    { CLAY_GRADIENT_BILINEAR, "CLAY_GRADIENT_BILINEAR", "bilinear" },
    { CLAY_GRADIENT_RADIAL, "CLAY_GRADIENT_RADIAL", "radial" },
    { CLAY_GRADIENT_SQUARE, "CLAY_GRADIENT_SQUARE", "square" },
    { CLAY_GRADIENT_CONICAL_SYMMETRIC, "CLAY_GRADIENT_CONICAL_SYMMETRIC", "conical-symmetric" },
    { CLAY_GRADIENT_CONICAL_ASYMMETRIC, "CLAY_GRADIENT_CONICAL_ASYMMETRIC", "conical-asymmetric" },
    { CLAY_GRADIENT_SHAPEBURST_ANGULAR, "CLAY_GRADIENT_SHAPEBURST_ANGULAR", "shapeburst-angular" },
    { CLAY_GRADIENT_SHAPEBURST_SPHERICAL, "CLAY_GRADIENT_SHAPEBURST_SPHERICAL", "shapeburst-spherical" },
    { CLAY_GRADIENT_SHAPEBURST_DIMPLED, "CLAY_GRADIENT_SHAPEBURST_DIMPLED", "shapeburst-dimpled" },
    { CLAY_GRADIENT_SPIRAL_CLOCKWISE, "CLAY_GRADIENT_SPIRAL_CLOCKWISE", "spiral-clockwise" },
    { CLAY_GRADIENT_SPIRAL_ANTICLOCKWISE, "CLAY_GRADIENT_SPIRAL_ANTICLOCKWISE", "spiral-anticlockwise" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRADIENT_LINEAR, NC_("gradient-type", "Linear"), NULL },
    { CLAY_GRADIENT_BILINEAR, NC_("gradient-type", "Bi-linear"), NULL },
    { CLAY_GRADIENT_RADIAL, NC_("gradient-type", "Radial"), NULL },
    { CLAY_GRADIENT_SQUARE, NC_("gradient-type", "Square"), NULL },
    { CLAY_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (symmetric)"), NULL },
    /* Translators: this is an abbreviated version of "Conical (symmetric)".
       Keep it short. */
    { CLAY_GRADIENT_CONICAL_SYMMETRIC, NC_("gradient-type", "Conical (sym)"), NULL },
    { CLAY_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asymmetric)"), NULL },
    /* Translators: this is an abbreviated version of "Conical (asymmetric)".
       Keep it short. */
    { CLAY_GRADIENT_CONICAL_ASYMMETRIC, NC_("gradient-type", "Conical (asym)"), NULL },
    { CLAY_GRADIENT_SHAPEBURST_ANGULAR, NC_("gradient-type", "Shaped (angular)"), NULL },
    { CLAY_GRADIENT_SHAPEBURST_SPHERICAL, NC_("gradient-type", "Shaped (spherical)"), NULL },
    { CLAY_GRADIENT_SHAPEBURST_DIMPLED, NC_("gradient-type", "Shaped (dimpled)"), NULL },
    { CLAY_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (clockwise)"), NULL },
    /* Translators: this is an abbreviated version of "Spiral (clockwise)".
       Keep it short. */
    { CLAY_GRADIENT_SPIRAL_CLOCKWISE, NC_("gradient-type", "Spiral (cw)"), NULL },
    { CLAY_GRADIENT_SPIRAL_ANTICLOCKWISE, NC_("gradient-type", "Spiral (counter-clockwise)"), NULL },
    /* Translators: this is an abbreviated version of "Spiral (counter-clockwise)".
       Keep it short. */
    { CLAY_GRADIENT_SPIRAL_ANTICLOCKWISE, NC_("gradient-type", "Spiral (ccw)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGradientType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "gradient-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_grid_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRID_DOTS, "CLAY_GRID_DOTS", "dots" },
    { CLAY_GRID_INTERSECTIONS, "CLAY_GRID_INTERSECTIONS", "intersections" },
    { CLAY_GRID_ON_OFF_DASH, "CLAY_GRID_ON_OFF_DASH", "on-off-dash" },
    { CLAY_GRID_DOUBLE_DASH, "CLAY_GRID_DOUBLE_DASH", "double-dash" },
    { CLAY_GRID_SOLID, "CLAY_GRID_SOLID", "solid" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRID_DOTS, NC_("grid-style", "Intersections (dots)"), NULL },
    { CLAY_GRID_INTERSECTIONS, NC_("grid-style", "Intersections (crosshairs)"), NULL },
    { CLAY_GRID_ON_OFF_DASH, NC_("grid-style", "Dashed"), NULL },
    { CLAY_GRID_DOUBLE_DASH, NC_("grid-style", "Double dashed"), NULL },
    { CLAY_GRID_SOLID, NC_("grid-style", "Solid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGridStyle", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "grid-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_hue_range_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HUE_RANGE_ALL, "CLAY_HUE_RANGE_ALL", "all" },
    { CLAY_HUE_RANGE_RED, "CLAY_HUE_RANGE_RED", "red" },
    { CLAY_HUE_RANGE_YELLOW, "CLAY_HUE_RANGE_YELLOW", "yellow" },
    { CLAY_HUE_RANGE_GREEN, "CLAY_HUE_RANGE_GREEN", "green" },
    { CLAY_HUE_RANGE_CYAN, "CLAY_HUE_RANGE_CYAN", "cyan" },
    { CLAY_HUE_RANGE_BLUE, "CLAY_HUE_RANGE_BLUE", "blue" },
    { CLAY_HUE_RANGE_MAGENTA, "CLAY_HUE_RANGE_MAGENTA", "magenta" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HUE_RANGE_ALL, "CLAY_HUE_RANGE_ALL", NULL },
    { CLAY_HUE_RANGE_RED, "CLAY_HUE_RANGE_RED", NULL },
    { CLAY_HUE_RANGE_YELLOW, "CLAY_HUE_RANGE_YELLOW", NULL },
    { CLAY_HUE_RANGE_GREEN, "CLAY_HUE_RANGE_GREEN", NULL },
    { CLAY_HUE_RANGE_CYAN, "CLAY_HUE_RANGE_CYAN", NULL },
    { CLAY_HUE_RANGE_BLUE, "CLAY_HUE_RANGE_BLUE", NULL },
    { CLAY_HUE_RANGE_MAGENTA, "CLAY_HUE_RANGE_MAGENTA", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayHueRange", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "hue-range");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_icon_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ICON_TYPE_ICON_NAME, "CLAY_ICON_TYPE_ICON_NAME", "icon-name" },
    { CLAY_ICON_TYPE_PIXBUF, "CLAY_ICON_TYPE_PIXBUF", "pixbuf" },
    { CLAY_ICON_TYPE_IMAGE_FILE, "CLAY_ICON_TYPE_IMAGE_FILE", "image-file" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ICON_TYPE_ICON_NAME, NC_("icon-type", "Icon name"), NULL },
    { CLAY_ICON_TYPE_PIXBUF, NC_("icon-type", "Pixbuf"), NULL },
    { CLAY_ICON_TYPE_IMAGE_FILE, NC_("icon-type", "Image file"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayIconType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "icon-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_image_base_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RGB, "CLAY_RGB", "rgb" },
    { CLAY_GRAY, "CLAY_GRAY", "gray" },
    { CLAY_INDEXED, "CLAY_INDEXED", "indexed" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RGB, NC_("image-base-type", "RGB color"), NULL },
    { CLAY_GRAY, NC_("image-base-type", "Grayscale"), NULL },
    { CLAY_INDEXED, NC_("image-base-type", "Indexed color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayImageBaseType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "image-base-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_image_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RGB_IMAGE, "CLAY_RGB_IMAGE", "rgb-image" },
    { CLAY_RGBA_IMAGE, "CLAY_RGBA_IMAGE", "rgba-image" },
    { CLAY_GRAY_IMAGE, "CLAY_GRAY_IMAGE", "gray-image" },
    { CLAY_GRAYA_IMAGE, "CLAY_GRAYA_IMAGE", "graya-image" },
    { CLAY_INDEXED_IMAGE, "CLAY_INDEXED_IMAGE", "indexed-image" },
    { CLAY_INDEXEDA_IMAGE, "CLAY_INDEXEDA_IMAGE", "indexeda-image" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RGB_IMAGE, NC_("image-type", "RGB"), NULL },
    { CLAY_RGBA_IMAGE, NC_("image-type", "RGB-alpha"), NULL },
    { CLAY_GRAY_IMAGE, NC_("image-type", "Grayscale"), NULL },
    { CLAY_GRAYA_IMAGE, NC_("image-type", "Grayscale-alpha"), NULL },
    { CLAY_INDEXED_IMAGE, NC_("image-type", "Indexed"), NULL },
    { CLAY_INDEXEDA_IMAGE, NC_("image-type", "Indexed-alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayImageType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "image-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_ink_blob_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_INK_BLOB_TYPE_CIRCLE, "CLAY_INK_BLOB_TYPE_CIRCLE", "circle" },
    { CLAY_INK_BLOB_TYPE_SQUARE, "CLAY_INK_BLOB_TYPE_SQUARE", "square" },
    { CLAY_INK_BLOB_TYPE_DIAMOND, "CLAY_INK_BLOB_TYPE_DIAMOND", "diamond" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_INK_BLOB_TYPE_CIRCLE, NC_("ink-blob-type", "Circle"), NULL },
    { CLAY_INK_BLOB_TYPE_SQUARE, NC_("ink-blob-type", "Square"), NULL },
    { CLAY_INK_BLOB_TYPE_DIAMOND, NC_("ink-blob-type", "Diamond"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayInkBlobType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "ink-blob-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_interpolation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_INTERPOLATION_NONE, "CLAY_INTERPOLATION_NONE", "none" },
    { CLAY_INTERPOLATION_LINEAR, "CLAY_INTERPOLATION_LINEAR", "linear" },
    { CLAY_INTERPOLATION_CUBIC, "CLAY_INTERPOLATION_CUBIC", "cubic" },
    { CLAY_INTERPOLATION_NOHALO, "CLAY_INTERPOLATION_NOHALO", "nohalo" },
    { CLAY_INTERPOLATION_LOHALO, "CLAY_INTERPOLATION_LOHALO", "lohalo" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_INTERPOLATION_NONE, NC_("interpolation-type", "None"), NULL },
    { CLAY_INTERPOLATION_LINEAR, NC_("interpolation-type", "Linear"), NULL },
    { CLAY_INTERPOLATION_CUBIC, NC_("interpolation-type", "Cubic"), NULL },
    { CLAY_INTERPOLATION_NOHALO, NC_("interpolation-type", "NoHalo"), NULL },
    { CLAY_INTERPOLATION_LOHALO, NC_("interpolation-type", "LoHalo"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayInterpolationType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "interpolation-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_join_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_JOIN_MITER, "CLAY_JOIN_MITER", "miter" },
    { CLAY_JOIN_ROUND, "CLAY_JOIN_ROUND", "round" },
    { CLAY_JOIN_BEVEL, "CLAY_JOIN_BEVEL", "bevel" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_JOIN_MITER, NC_("join-style", "Miter"), NULL },
    { CLAY_JOIN_ROUND, NC_("join-style", "Round"), NULL },
    { CLAY_JOIN_BEVEL, NC_("join-style", "Bevel"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayJoinStyle", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "join-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_mask_apply_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MASK_APPLY, "CLAY_MASK_APPLY", "apply" },
    { CLAY_MASK_DISCARD, "CLAY_MASK_DISCARD", "discard" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MASK_APPLY, "CLAY_MASK_APPLY", NULL },
    { CLAY_MASK_DISCARD, "CLAY_MASK_DISCARD", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMaskApplyMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "mask-apply-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_merge_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_EXPAND_AS_NECESSARY, "CLAY_EXPAND_AS_NECESSARY", "expand-as-necessary" },
    { CLAY_CLIP_TO_IMAGE, "CLAY_CLIP_TO_IMAGE", "clip-to-image" },
    { CLAY_CLIP_TO_BOTTOM_LAYER, "CLAY_CLIP_TO_BOTTOM_LAYER", "clip-to-bottom-layer" },
    { CLAY_FLATTEN_IMAGE, "CLAY_FLATTEN_IMAGE", "flatten-image" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_EXPAND_AS_NECESSARY, NC_("merge-type", "Expanded as necessary"), NULL },
    { CLAY_CLIP_TO_IMAGE, NC_("merge-type", "Clipped to image"), NULL },
    { CLAY_CLIP_TO_BOTTOM_LAYER, NC_("merge-type", "Clipped to bottom layer"), NULL },
    { CLAY_FLATTEN_IMAGE, NC_("merge-type", "Flatten"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMergeType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "merge-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_message_handler_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MESSAGE_BOX, "CLAY_MESSAGE_BOX", "message-box" },
    { CLAY_CONSOLE, "CLAY_CONSOLE", "console" },
    { CLAY_ERROR_CONSOLE, "CLAY_ERROR_CONSOLE", "error-console" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MESSAGE_BOX, "CLAY_MESSAGE_BOX", NULL },
    { CLAY_CONSOLE, "CLAY_CONSOLE", NULL },
    { CLAY_ERROR_CONSOLE, "CLAY_ERROR_CONSOLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMessageHandlerType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "message-handler-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_offset_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_OFFSET_COLOR, "CLAY_OFFSET_COLOR", "color" },
    { CLAY_OFFSET_TRANSPARENT, "CLAY_OFFSET_TRANSPARENT", "transparent" },
    { CLAY_OFFSET_WRAP_AROUND, "CLAY_OFFSET_WRAP_AROUND", "wrap-around" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_OFFSET_COLOR, "CLAY_OFFSET_COLOR", NULL },
    { CLAY_OFFSET_TRANSPARENT, "CLAY_OFFSET_TRANSPARENT", NULL },
    { CLAY_OFFSET_WRAP_AROUND, "CLAY_OFFSET_WRAP_AROUND", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayOffsetType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "offset-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_orientation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ORIENTATION_HORIZONTAL, "CLAY_ORIENTATION_HORIZONTAL", "horizontal" },
    { CLAY_ORIENTATION_VERTICAL, "CLAY_ORIENTATION_VERTICAL", "vertical" },
    { CLAY_ORIENTATION_UNKNOWN, "CLAY_ORIENTATION_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ORIENTATION_HORIZONTAL, NC_("orientation-type", "Horizontal"), NULL },
    { CLAY_ORIENTATION_VERTICAL, NC_("orientation-type", "Vertical"), NULL },
    { CLAY_ORIENTATION_UNKNOWN, NC_("orientation-type", "Unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayOrientationType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "orientation-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_paint_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PAINT_CONSTANT, "CLAY_PAINT_CONSTANT", "constant" },
    { CLAY_PAINT_INCREMENTAL, "CLAY_PAINT_INCREMENTAL", "incremental" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PAINT_CONSTANT, NC_("paint-application-mode", "Constant"), NULL },
    { CLAY_PAINT_INCREMENTAL, NC_("paint-application-mode", "Incremental"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPaintApplicationMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "paint-application-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_pdb_error_handler_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PDB_ERROR_HANDLER_INTERNAL, "CLAY_PDB_ERROR_HANDLER_INTERNAL", "internal" },
    { CLAY_PDB_ERROR_HANDLER_PLUGIN, "CLAY_PDB_ERROR_HANDLER_PLUGIN", "plugin" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PDB_ERROR_HANDLER_INTERNAL, "CLAY_PDB_ERROR_HANDLER_INTERNAL", NULL },
    { CLAY_PDB_ERROR_HANDLER_PLUGIN, "CLAY_PDB_ERROR_HANDLER_PLUGIN", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPDBErrorHandler", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "pdb-error-handler");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_pdb_proc_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PDB_PROC_TYPE_INTERNAL, "CLAY_PDB_PROC_TYPE_INTERNAL", "internal" },
    { CLAY_PDB_PROC_TYPE_PLUGIN, "CLAY_PDB_PROC_TYPE_PLUGIN", "plugin" },
    { CLAY_PDB_PROC_TYPE_PERSISTENT, "CLAY_PDB_PROC_TYPE_PERSISTENT", "persistent" },
    { CLAY_PDB_PROC_TYPE_TEMPORARY, "CLAY_PDB_PROC_TYPE_TEMPORARY", "temporary" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PDB_PROC_TYPE_INTERNAL, NC_("pdb-proc-type", "Internal Clay procedure"), NULL },
    { CLAY_PDB_PROC_TYPE_PLUGIN, NC_("pdb-proc-type", "Clay Plug-In"), NULL },
    { CLAY_PDB_PROC_TYPE_PERSISTENT, NC_("pdb-proc-type", "Clay Persistent Plug-In"), NULL },
    { CLAY_PDB_PROC_TYPE_TEMPORARY, NC_("pdb-proc-type", "Temporary Procedure"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPDBProcType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "pdb-proc-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_pdb_status_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PDB_EXECUTION_ERROR, "CLAY_PDB_EXECUTION_ERROR", "execution-error" },
    { CLAY_PDB_CALLING_ERROR, "CLAY_PDB_CALLING_ERROR", "calling-error" },
    { CLAY_PDB_PASS_THROUGH, "CLAY_PDB_PASS_THROUGH", "pass-through" },
    { CLAY_PDB_SUCCESS, "CLAY_PDB_SUCCESS", "success" },
    { CLAY_PDB_CANCEL, "CLAY_PDB_CANCEL", "cancel" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PDB_EXECUTION_ERROR, "CLAY_PDB_EXECUTION_ERROR", NULL },
    { CLAY_PDB_CALLING_ERROR, "CLAY_PDB_CALLING_ERROR", NULL },
    { CLAY_PDB_PASS_THROUGH, "CLAY_PDB_PASS_THROUGH", NULL },
    { CLAY_PDB_SUCCESS, "CLAY_PDB_SUCCESS", NULL },
    { CLAY_PDB_CANCEL, "CLAY_PDB_CANCEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPDBStatusType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "pdb-status-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PRECISION_U8_LINEAR, "CLAY_PRECISION_U8_LINEAR", "u8-linear" },
    { CLAY_PRECISION_U8_NON_LINEAR, "CLAY_PRECISION_U8_NON_LINEAR", "u8-non-linear" },
    { CLAY_PRECISION_U8_PERCEPTUAL, "CLAY_PRECISION_U8_PERCEPTUAL", "u8-perceptual" },
    { CLAY_PRECISION_U16_LINEAR, "CLAY_PRECISION_U16_LINEAR", "u16-linear" },
    { CLAY_PRECISION_U16_NON_LINEAR, "CLAY_PRECISION_U16_NON_LINEAR", "u16-non-linear" },
    { CLAY_PRECISION_U16_PERCEPTUAL, "CLAY_PRECISION_U16_PERCEPTUAL", "u16-perceptual" },
    { CLAY_PRECISION_U32_LINEAR, "CLAY_PRECISION_U32_LINEAR", "u32-linear" },
    { CLAY_PRECISION_U32_NON_LINEAR, "CLAY_PRECISION_U32_NON_LINEAR", "u32-non-linear" },
    { CLAY_PRECISION_U32_PERCEPTUAL, "CLAY_PRECISION_U32_PERCEPTUAL", "u32-perceptual" },
    { CLAY_PRECISION_HALF_LINEAR, "CLAY_PRECISION_HALF_LINEAR", "half-linear" },
    { CLAY_PRECISION_HALF_NON_LINEAR, "CLAY_PRECISION_HALF_NON_LINEAR", "half-non-linear" },
    { CLAY_PRECISION_HALF_PERCEPTUAL, "CLAY_PRECISION_HALF_PERCEPTUAL", "half-perceptual" },
    { CLAY_PRECISION_FLOAT_LINEAR, "CLAY_PRECISION_FLOAT_LINEAR", "float-linear" },
    { CLAY_PRECISION_FLOAT_NON_LINEAR, "CLAY_PRECISION_FLOAT_NON_LINEAR", "float-non-linear" },
    { CLAY_PRECISION_FLOAT_PERCEPTUAL, "CLAY_PRECISION_FLOAT_PERCEPTUAL", "float-perceptual" },
    { CLAY_PRECISION_DOUBLE_LINEAR, "CLAY_PRECISION_DOUBLE_LINEAR", "double-linear" },
    { CLAY_PRECISION_DOUBLE_NON_LINEAR, "CLAY_PRECISION_DOUBLE_NON_LINEAR", "double-non-linear" },
    { CLAY_PRECISION_DOUBLE_PERCEPTUAL, "CLAY_PRECISION_DOUBLE_PERCEPTUAL", "double-perceptual" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PRECISION_U8_LINEAR, NC_("precision", "8-bit linear integer"), NULL },
    { CLAY_PRECISION_U8_NON_LINEAR, NC_("precision", "8-bit non-linear integer"), NULL },
    { CLAY_PRECISION_U8_PERCEPTUAL, NC_("precision", "8-bit perceptual integer"), NULL },
    { CLAY_PRECISION_U16_LINEAR, NC_("precision", "16-bit linear integer"), NULL },
    { CLAY_PRECISION_U16_NON_LINEAR, NC_("precision", "16-bit non-linear integer"), NULL },
    { CLAY_PRECISION_U16_PERCEPTUAL, NC_("precision", "16-bit perceptual integer"), NULL },
    { CLAY_PRECISION_U32_LINEAR, NC_("precision", "32-bit linear integer"), NULL },
    { CLAY_PRECISION_U32_NON_LINEAR, NC_("precision", "32-bit non-linear integer"), NULL },
    { CLAY_PRECISION_U32_PERCEPTUAL, NC_("precision", "32-bit perceptual integer"), NULL },
    { CLAY_PRECISION_HALF_LINEAR, NC_("precision", "16-bit linear floating point"), NULL },
    { CLAY_PRECISION_HALF_NON_LINEAR, NC_("precision", "16-bit non-linear floating point"), NULL },
    { CLAY_PRECISION_HALF_PERCEPTUAL, NC_("precision", "16-bit perceptual floating point"), NULL },
    { CLAY_PRECISION_FLOAT_LINEAR, NC_("precision", "32-bit linear floating point"), NULL },
    { CLAY_PRECISION_FLOAT_NON_LINEAR, NC_("precision", "32-bit non-linear floating point"), NULL },
    { CLAY_PRECISION_FLOAT_PERCEPTUAL, NC_("precision", "32-bit perceptual floating point"), NULL },
    { CLAY_PRECISION_DOUBLE_LINEAR, NC_("precision", "64-bit linear floating point"), NULL },
    { CLAY_PRECISION_DOUBLE_NON_LINEAR, NC_("precision", "64-bit non-linear floating point"), NULL },
    { CLAY_PRECISION_DOUBLE_PERCEPTUAL, NC_("precision", "64-bit perceptual floating point"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPrecision", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "precision");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_procedure_sensitivity_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { CLAY_PROCEDURE_SENSITIVE_DRAWABLE, "CLAY_PROCEDURE_SENSITIVE_DRAWABLE", "drawable" },
    { CLAY_PROCEDURE_SENSITIVE_DRAWABLES, "CLAY_PROCEDURE_SENSITIVE_DRAWABLES", "drawables" },
    { CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES, "CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES", "no-drawables" },
    { CLAY_PROCEDURE_SENSITIVE_NO_IMAGE, "CLAY_PROCEDURE_SENSITIVE_NO_IMAGE", "no-image" },
    { CLAY_PROCEDURE_SENSITIVE_ALWAYS, "CLAY_PROCEDURE_SENSITIVE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const ClayFlagsDesc descs[] =
  {
    { CLAY_PROCEDURE_SENSITIVE_DRAWABLE, "CLAY_PROCEDURE_SENSITIVE_DRAWABLE", NULL },
    { CLAY_PROCEDURE_SENSITIVE_DRAWABLES, "CLAY_PROCEDURE_SENSITIVE_DRAWABLES", NULL },
    { CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES, "CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES", NULL },
    { CLAY_PROCEDURE_SENSITIVE_NO_IMAGE, "CLAY_PROCEDURE_SENSITIVE_NO_IMAGE", NULL },
    { CLAY_PROCEDURE_SENSITIVE_ALWAYS, "CLAY_PROCEDURE_SENSITIVE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("ClayProcedureSensitivityMask", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "procedure-sensitivity-mask");
      clay_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_progress_command_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PROGRESS_COMMAND_START, "CLAY_PROGRESS_COMMAND_START", "start" },
    { CLAY_PROGRESS_COMMAND_END, "CLAY_PROGRESS_COMMAND_END", "end" },
    { CLAY_PROGRESS_COMMAND_SET_TEXT, "CLAY_PROGRESS_COMMAND_SET_TEXT", "set-text" },
    { CLAY_PROGRESS_COMMAND_SET_VALUE, "CLAY_PROGRESS_COMMAND_SET_VALUE", "set-value" },
    { CLAY_PROGRESS_COMMAND_PULSE, "CLAY_PROGRESS_COMMAND_PULSE", "pulse" },
    { CLAY_PROGRESS_COMMAND_GET_WINDOW, "CLAY_PROGRESS_COMMAND_GET_WINDOW", "get-window" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PROGRESS_COMMAND_START, "CLAY_PROGRESS_COMMAND_START", NULL },
    { CLAY_PROGRESS_COMMAND_END, "CLAY_PROGRESS_COMMAND_END", NULL },
    { CLAY_PROGRESS_COMMAND_SET_TEXT, "CLAY_PROGRESS_COMMAND_SET_TEXT", NULL },
    { CLAY_PROGRESS_COMMAND_SET_VALUE, "CLAY_PROGRESS_COMMAND_SET_VALUE", NULL },
    { CLAY_PROGRESS_COMMAND_PULSE, "CLAY_PROGRESS_COMMAND_PULSE", NULL },
    { CLAY_PROGRESS_COMMAND_GET_WINDOW, "CLAY_PROGRESS_COMMAND_GET_WINDOW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayProgressCommand", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "progress-command");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_repeat_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_REPEAT_NONE, "CLAY_REPEAT_NONE", "none" },
    { CLAY_REPEAT_TRUNCATE, "CLAY_REPEAT_TRUNCATE", "truncate" },
    { CLAY_REPEAT_SAWTOOTH, "CLAY_REPEAT_SAWTOOTH", "sawtooth" },
    { CLAY_REPEAT_TRIANGULAR, "CLAY_REPEAT_TRIANGULAR", "triangular" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_REPEAT_NONE, NC_("repeat-mode", "None (extend)"), NULL },
    { CLAY_REPEAT_TRUNCATE, NC_("repeat-mode", "None (truncate)"), NULL },
    { CLAY_REPEAT_SAWTOOTH, NC_("repeat-mode", "Sawtooth wave"), NULL },
    { CLAY_REPEAT_TRIANGULAR, NC_("repeat-mode", "Triangular wave"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRepeatMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "repeat-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_rotation_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ROTATE_DEGREES90, "CLAY_ROTATE_DEGREES90", "degrees90" },
    { CLAY_ROTATE_DEGREES180, "CLAY_ROTATE_DEGREES180", "degrees180" },
    { CLAY_ROTATE_DEGREES270, "CLAY_ROTATE_DEGREES270", "degrees270" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ROTATE_DEGREES90, "CLAY_ROTATE_DEGREES90", NULL },
    { CLAY_ROTATE_DEGREES180, "CLAY_ROTATE_DEGREES180", NULL },
    { CLAY_ROTATE_DEGREES270, "CLAY_ROTATE_DEGREES270", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRotationType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "rotation-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_run_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RUN_INTERACTIVE, "CLAY_RUN_INTERACTIVE", "interactive" },
    { CLAY_RUN_NONINTERACTIVE, "CLAY_RUN_NONINTERACTIVE", "noninteractive" },
    { CLAY_RUN_WITH_LAST_VALS, "CLAY_RUN_WITH_LAST_VALS", "with-last-vals" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RUN_INTERACTIVE, NC_("run-mode", "Run interactively"), NULL },
    { CLAY_RUN_NONINTERACTIVE, NC_("run-mode", "Run non-interactively"), NULL },
    { CLAY_RUN_WITH_LAST_VALS, NC_("run-mode", "Run with last used values"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRunMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "run-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_select_criterion_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_SELECT_CRITERION_COMPOSITE, "CLAY_SELECT_CRITERION_COMPOSITE", "composite" },
    { CLAY_SELECT_CRITERION_RGB_RED, "CLAY_SELECT_CRITERION_RGB_RED", "rgb-red" },
    { CLAY_SELECT_CRITERION_RGB_GREEN, "CLAY_SELECT_CRITERION_RGB_GREEN", "rgb-green" },
    { CLAY_SELECT_CRITERION_RGB_BLUE, "CLAY_SELECT_CRITERION_RGB_BLUE", "rgb-blue" },
    { CLAY_SELECT_CRITERION_HSV_HUE, "CLAY_SELECT_CRITERION_HSV_HUE", "hsv-hue" },
    { CLAY_SELECT_CRITERION_HSV_SATURATION, "CLAY_SELECT_CRITERION_HSV_SATURATION", "hsv-saturation" },
    { CLAY_SELECT_CRITERION_HSV_VALUE, "CLAY_SELECT_CRITERION_HSV_VALUE", "hsv-value" },
    { CLAY_SELECT_CRITERION_LCH_LIGHTNESS, "CLAY_SELECT_CRITERION_LCH_LIGHTNESS", "lch-lightness" },
    { CLAY_SELECT_CRITERION_LCH_CHROMA, "CLAY_SELECT_CRITERION_LCH_CHROMA", "lch-chroma" },
    { CLAY_SELECT_CRITERION_LCH_HUE, "CLAY_SELECT_CRITERION_LCH_HUE", "lch-hue" },
    { CLAY_SELECT_CRITERION_ALPHA, "CLAY_SELECT_CRITERION_ALPHA", "alpha" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_SELECT_CRITERION_COMPOSITE, NC_("select-criterion", "Composite"), NULL },
    { CLAY_SELECT_CRITERION_RGB_RED, NC_("select-criterion", "Red"), NULL },
    { CLAY_SELECT_CRITERION_RGB_GREEN, NC_("select-criterion", "Green"), NULL },
    { CLAY_SELECT_CRITERION_RGB_BLUE, NC_("select-criterion", "Blue"), NULL },
    { CLAY_SELECT_CRITERION_HSV_HUE, NC_("select-criterion", "HSV Hue"), NULL },
    { CLAY_SELECT_CRITERION_HSV_SATURATION, NC_("select-criterion", "HSV Saturation"), NULL },
    { CLAY_SELECT_CRITERION_HSV_VALUE, NC_("select-criterion", "HSV Value"), NULL },
    { CLAY_SELECT_CRITERION_LCH_LIGHTNESS, NC_("select-criterion", "LCh Lightness"), NULL },
    { CLAY_SELECT_CRITERION_LCH_CHROMA, NC_("select-criterion", "LCh Chroma"), NULL },
    { CLAY_SELECT_CRITERION_LCH_HUE, NC_("select-criterion", "LCh Hue"), NULL },
    { CLAY_SELECT_CRITERION_ALPHA, NC_("select-criterion", "Alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClaySelectCriterion", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "select-criterion");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_size_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PIXELS, "CLAY_PIXELS", "pixels" },
    { CLAY_POINTS, "CLAY_POINTS", "points" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PIXELS, NC_("size-type", "Pixels"), NULL },
    { CLAY_POINTS, NC_("size-type", "Points"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClaySizeType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "size-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_stack_trace_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_STACK_TRACE_NEVER, "CLAY_STACK_TRACE_NEVER", "never" },
    { CLAY_STACK_TRACE_QUERY, "CLAY_STACK_TRACE_QUERY", "query" },
    { CLAY_STACK_TRACE_ALWAYS, "CLAY_STACK_TRACE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_STACK_TRACE_NEVER, "CLAY_STACK_TRACE_NEVER", NULL },
    { CLAY_STACK_TRACE_QUERY, "CLAY_STACK_TRACE_QUERY", NULL },
    { CLAY_STACK_TRACE_ALWAYS, "CLAY_STACK_TRACE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayStackTraceMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "stack-trace-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_stroke_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_STROKE_LINE, "CLAY_STROKE_LINE", "line" },
    { CLAY_STROKE_PAINT_METHOD, "CLAY_STROKE_PAINT_METHOD", "paint-method" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_STROKE_LINE, NC_("stroke-method", "Stroke line"), NULL },
    { CLAY_STROKE_PAINT_METHOD, NC_("stroke-method", "Stroke with a paint tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayStrokeMethod", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "stroke-method");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_text_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TEXT_DIRECTION_LTR, "CLAY_TEXT_DIRECTION_LTR", "ltr" },
    { CLAY_TEXT_DIRECTION_RTL, "CLAY_TEXT_DIRECTION_RTL", "rtl" },
    { CLAY_TEXT_DIRECTION_TTB_RTL, "CLAY_TEXT_DIRECTION_TTB_RTL", "ttb-rtl" },
    { CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT, "CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT", "ttb-rtl-upright" },
    { CLAY_TEXT_DIRECTION_TTB_LTR, "CLAY_TEXT_DIRECTION_TTB_LTR", "ttb-ltr" },
    { CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT, "CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT", "ttb-ltr-upright" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TEXT_DIRECTION_LTR, NC_("text-direction", "From left to right"), NULL },
    { CLAY_TEXT_DIRECTION_RTL, NC_("text-direction", "From right to left"), NULL },
    { CLAY_TEXT_DIRECTION_TTB_RTL, NC_("text-direction", "Vertical, right to left (mixed orientation)"), NULL },
    { CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT, NC_("text-direction", "Vertical, right to left (upright orientation)"), NULL },
    { CLAY_TEXT_DIRECTION_TTB_LTR, NC_("text-direction", "Vertical, left to right (mixed orientation)"), NULL },
    { CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT, NC_("text-direction", "Vertical, left to right (upright orientation)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTextDirection", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "text-direction");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_text_hint_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TEXT_HINT_STYLE_NONE, "CLAY_TEXT_HINT_STYLE_NONE", "none" },
    { CLAY_TEXT_HINT_STYLE_SLIGHT, "CLAY_TEXT_HINT_STYLE_SLIGHT", "slight" },
    { CLAY_TEXT_HINT_STYLE_MEDIUM, "CLAY_TEXT_HINT_STYLE_MEDIUM", "medium" },
    { CLAY_TEXT_HINT_STYLE_FULL, "CLAY_TEXT_HINT_STYLE_FULL", "full" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TEXT_HINT_STYLE_NONE, NC_("text-hint-style", "None"), NULL },
    { CLAY_TEXT_HINT_STYLE_SLIGHT, NC_("text-hint-style", "Slight"), NULL },
    { CLAY_TEXT_HINT_STYLE_MEDIUM, NC_("text-hint-style", "Medium"), NULL },
    { CLAY_TEXT_HINT_STYLE_FULL, NC_("text-hint-style", "Full"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTextHintStyle", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "text-hint-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_text_justification_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TEXT_JUSTIFY_LEFT, "CLAY_TEXT_JUSTIFY_LEFT", "left" },
    { CLAY_TEXT_JUSTIFY_RIGHT, "CLAY_TEXT_JUSTIFY_RIGHT", "right" },
    { CLAY_TEXT_JUSTIFY_CENTER, "CLAY_TEXT_JUSTIFY_CENTER", "center" },
    { CLAY_TEXT_JUSTIFY_FILL, "CLAY_TEXT_JUSTIFY_FILL", "fill" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TEXT_JUSTIFY_LEFT, NC_("text-justification", "Left justified"), NULL },
    { CLAY_TEXT_JUSTIFY_RIGHT, NC_("text-justification", "Right justified"), NULL },
    { CLAY_TEXT_JUSTIFY_CENTER, NC_("text-justification", "Centered"), NULL },
    { CLAY_TEXT_JUSTIFY_FILL, NC_("text-justification", "Filled"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTextJustification", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "text-justification");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_text_outline_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TEXT_OUTLINE_NONE, "CLAY_TEXT_OUTLINE_NONE", "none" },
    { CLAY_TEXT_OUTLINE_STROKE_ONLY, "CLAY_TEXT_OUTLINE_STROKE_ONLY", "stroke-only" },
    { CLAY_TEXT_OUTLINE_STROKE_FILL, "CLAY_TEXT_OUTLINE_STROKE_FILL", "stroke-fill" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TEXT_OUTLINE_NONE, NC_("text-outline", "Filled"), NULL },
    { CLAY_TEXT_OUTLINE_STROKE_ONLY, NC_("text-outline", "Outlined"), NULL },
    { CLAY_TEXT_OUTLINE_STROKE_FILL, NC_("text-outline", "Outlined and filled"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTextOutline", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "text-outline");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_text_outline_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TEXT_OUTLINE_DIRECTION_OUTER, "CLAY_TEXT_OUTLINE_DIRECTION_OUTER", "outer" },
    { CLAY_TEXT_OUTLINE_DIRECTION_INNER, "CLAY_TEXT_OUTLINE_DIRECTION_INNER", "inner" },
    { CLAY_TEXT_OUTLINE_DIRECTION_CENTERED, "CLAY_TEXT_OUTLINE_DIRECTION_CENTERED", "centered" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TEXT_OUTLINE_DIRECTION_OUTER, NC_("text-outline-direction", "Outer"), NULL },
    { CLAY_TEXT_OUTLINE_DIRECTION_INNER, NC_("text-outline-direction", "Inner"), NULL },
    { CLAY_TEXT_OUTLINE_DIRECTION_CENTERED, NC_("text-outline-direction", "Centered"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTextOutlineDirection", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "text-outline-direction");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transfer_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFER_SHADOWS, "CLAY_TRANSFER_SHADOWS", "shadows" },
    { CLAY_TRANSFER_MIDTONES, "CLAY_TRANSFER_MIDTONES", "midtones" },
    { CLAY_TRANSFER_HIGHLIGHTS, "CLAY_TRANSFER_HIGHLIGHTS", "highlights" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFER_SHADOWS, NC_("transfer-mode", "Shadows"), NULL },
    { CLAY_TRANSFER_MIDTONES, NC_("transfer-mode", "Midtones"), NULL },
    { CLAY_TRANSFER_HIGHLIGHTS, NC_("transfer-mode", "Highlights"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransferMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "transfer-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_direction_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFORM_FORWARD, "CLAY_TRANSFORM_FORWARD", "forward" },
    { CLAY_TRANSFORM_BACKWARD, "CLAY_TRANSFORM_BACKWARD", "backward" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFORM_FORWARD, NC_("transform-direction", "Normal (Forward)"), NULL },
    { CLAY_TRANSFORM_BACKWARD, NC_("transform-direction", "Corrective (Backward)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransformDirection", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "transform-direction");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_resize_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFORM_RESIZE_ADJUST, "CLAY_TRANSFORM_RESIZE_ADJUST", "adjust" },
    { CLAY_TRANSFORM_RESIZE_CLIP, "CLAY_TRANSFORM_RESIZE_CLIP", "clip" },
    { CLAY_TRANSFORM_RESIZE_CROP, "CLAY_TRANSFORM_RESIZE_CROP", "crop" },
    { CLAY_TRANSFORM_RESIZE_CROP_WITH_ASPECT, "CLAY_TRANSFORM_RESIZE_CROP_WITH_ASPECT", "crop-with-aspect" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFORM_RESIZE_ADJUST, NC_("transform-resize", "Adjust"), NULL },
    { CLAY_TRANSFORM_RESIZE_CLIP, NC_("transform-resize", "Clip"), NULL },
    { CLAY_TRANSFORM_RESIZE_CROP, NC_("transform-resize", "Crop to result"), NULL },
    { CLAY_TRANSFORM_RESIZE_CROP_WITH_ASPECT, NC_("transform-resize", "Crop with aspect"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransformResize", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "transform-resize");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_unit_id_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_UNIT_PIXEL, "CLAY_UNIT_PIXEL", "pixel" },
    { CLAY_UNIT_INCH, "CLAY_UNIT_INCH", "inch" },
    { CLAY_UNIT_MM, "CLAY_UNIT_MM", "mm" },
    { CLAY_UNIT_POINT, "CLAY_UNIT_POINT", "point" },
    { CLAY_UNIT_PICA, "CLAY_UNIT_PICA", "pica" },
    { CLAY_UNIT_END, "CLAY_UNIT_END", "end" },
    { CLAY_UNIT_PERCENT, "CLAY_UNIT_PERCENT", "percent" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_UNIT_PIXEL, "CLAY_UNIT_PIXEL", NULL },
    { CLAY_UNIT_INCH, "CLAY_UNIT_INCH", NULL },
    { CLAY_UNIT_MM, "CLAY_UNIT_MM", NULL },
    { CLAY_UNIT_POINT, "CLAY_UNIT_POINT", NULL },
    { CLAY_UNIT_PICA, "CLAY_UNIT_PICA", NULL },
    { CLAY_UNIT_END, "CLAY_UNIT_END", NULL },
    { CLAY_UNIT_PERCENT, "CLAY_UNIT_PERCENT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayUnitID", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "unit-id");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_path_stroke_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PATH_STROKE_TYPE_BEZIER, "CLAY_PATH_STROKE_TYPE_BEZIER", "bezier" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PATH_STROKE_TYPE_BEZIER, "CLAY_PATH_STROKE_TYPE_BEZIER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPathStrokeType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "path-stroke-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_export_capabilities_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { CLAY_EXPORT_CAN_HANDLE_RGB, "CLAY_EXPORT_CAN_HANDLE_RGB", "can-handle-rgb" },
    { CLAY_EXPORT_CAN_HANDLE_GRAY, "CLAY_EXPORT_CAN_HANDLE_GRAY", "can-handle-gray" },
    { CLAY_EXPORT_CAN_HANDLE_INDEXED, "CLAY_EXPORT_CAN_HANDLE_INDEXED", "can-handle-indexed" },
    { CLAY_EXPORT_CAN_HANDLE_BITMAP, "CLAY_EXPORT_CAN_HANDLE_BITMAP", "can-handle-bitmap" },
    { CLAY_EXPORT_CAN_HANDLE_ALPHA, "CLAY_EXPORT_CAN_HANDLE_ALPHA", "can-handle-alpha" },
    { CLAY_EXPORT_CAN_HANDLE_LAYERS, "CLAY_EXPORT_CAN_HANDLE_LAYERS", "can-handle-layers" },
    { CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION, "CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION", "can-handle-layers-as-animation" },
    { CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS, "CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS", "can-handle-layer-masks" },
    { CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS, "CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS", "can-handle-layer-effects" },
    { CLAY_EXPORT_NEEDS_ALPHA, "CLAY_EXPORT_NEEDS_ALPHA", "needs-alpha" },
    { CLAY_EXPORT_NEEDS_CROP, "CLAY_EXPORT_NEEDS_CROP", "needs-crop" },
    { 0, NULL, NULL }
  };

  static const ClayFlagsDesc descs[] =
  {
    { CLAY_EXPORT_CAN_HANDLE_RGB, "CLAY_EXPORT_CAN_HANDLE_RGB", NULL },
    { CLAY_EXPORT_CAN_HANDLE_GRAY, "CLAY_EXPORT_CAN_HANDLE_GRAY", NULL },
    { CLAY_EXPORT_CAN_HANDLE_INDEXED, "CLAY_EXPORT_CAN_HANDLE_INDEXED", NULL },
    { CLAY_EXPORT_CAN_HANDLE_BITMAP, "CLAY_EXPORT_CAN_HANDLE_BITMAP", NULL },
    { CLAY_EXPORT_CAN_HANDLE_ALPHA, "CLAY_EXPORT_CAN_HANDLE_ALPHA", NULL },
    { CLAY_EXPORT_CAN_HANDLE_LAYERS, "CLAY_EXPORT_CAN_HANDLE_LAYERS", NULL },
    { CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION, "CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION", NULL },
    { CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS, "CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS", NULL },
    { CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS, "CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS", NULL },
    { CLAY_EXPORT_NEEDS_ALPHA, "CLAY_EXPORT_NEEDS_ALPHA", NULL },
    { CLAY_EXPORT_NEEDS_CROP, "CLAY_EXPORT_NEEDS_CROP", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("ClayExportCapabilities", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "export-capabilities");
      clay_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_file_chooser_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_FILE_CHOOSER_ACTION_ANY, "CLAY_FILE_CHOOSER_ACTION_ANY", "any" },
    { CLAY_FILE_CHOOSER_ACTION_OPEN, "CLAY_FILE_CHOOSER_ACTION_OPEN", "open" },
    { CLAY_FILE_CHOOSER_ACTION_SAVE, "CLAY_FILE_CHOOSER_ACTION_SAVE", "save" },
    { CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER, "CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER", "select-folder" },
    { CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER, "CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER", "create-folder" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_FILE_CHOOSER_ACTION_ANY, "CLAY_FILE_CHOOSER_ACTION_ANY", NULL },
    { CLAY_FILE_CHOOSER_ACTION_OPEN, "CLAY_FILE_CHOOSER_ACTION_OPEN", NULL },
    { CLAY_FILE_CHOOSER_ACTION_SAVE, "CLAY_FILE_CHOOSER_ACTION_SAVE", NULL },
    { CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER, "CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER", NULL },
    { CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER, "CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayFileChooserAction", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "file-chooser-action");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

